// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterPickupBase.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// create the kick damage check volume
	KickDamageCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Kick Damage Collision"));
	KickDamageCollision->SetupAttachment(GetRootComponent());
	KickDamageCollision->SetSphereRadius(100.0f);
	KickDamageCollision->SetRelativeLocation(FVector(100.0f, 0.0f, 0.0f));
	KickDamageCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	KickDamageCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	KickDamageCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	KickDamageCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	KickDamageCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	KickDamageCollision->SetGenerateOverlapEvents(true);

	KickDamageType = UDamageType::StaticClass();

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(ActionTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);

		// Pickup
		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &AShooterCharacter::DoPickup);
		}

		// Kick
		if (KickAction)
		{
			EnhancedInputComponent->BindAction(KickAction, ETriggerEvent::Started, this, &AShooterCharacter::DoKick);
		}
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1)
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::DoPickup()
{
	if (AShooterPickupBase* Pickup = FindBestPickupCandidate())
	{
		Pickup->TryPickup(this, true);
	}
}

void AShooterCharacter::RegisterPickupCandidate(AShooterPickupBase* Pickup)
{
	if (!IsValid(Pickup))
	{
		return;
	}

	PickupCandidates.AddUnique(TWeakObjectPtr<AShooterPickupBase>(Pickup));

	if (Pickup->CanAutoPickup(this))
	{
		Pickup->TryPickup(this, false);
	}
}

void AShooterCharacter::UnregisterPickupCandidate(AShooterPickupBase* Pickup)
{
	PickupCandidates.RemoveAllSwap([Pickup](const TWeakObjectPtr<AShooterPickupBase>& PickupCandidate)
	{
		return PickupCandidate.Get() == Pickup;
	});
}

bool AShooterCharacter::ShouldAutoPickupWeapon() const
{
	return !IsValid(CurrentWeapon) || CurrentWeapon->GetBulletCount() <= 0;
}

bool AShooterCharacter::ReplaceCurrentWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass, TSubclassOf<AShooterWeapon>& OutReplacedWeaponClass)
{
	OutReplacedWeaponClass = nullptr;

	if (!WeaponClass)
	{
		return false;
	}

	AShooterWeapon* ReplacedWeapon = IsValid(CurrentWeapon) ? CurrentWeapon.Get() : nullptr;
	if (!ReplacedWeapon)
	{
		CurrentWeapon = nullptr;
	}

	if (ReplacedWeapon)
	{
		OutReplacedWeaponClass = ReplacedWeapon->GetClass();
	}

	if (AShooterWeapon* ExistingWeapon = FindWeaponOfType(WeaponClass))
	{
		if (ExistingWeapon != ReplacedWeapon)
		{
			if (ReplacedWeapon)
			{
				ReplacedWeapon->DeactivateWeapon();
				OwnedWeapons.Remove(ReplacedWeapon);
				ReplacedWeapon->Destroy();
			}

			CurrentWeapon = ExistingWeapon;
			CurrentWeapon->ActivateWeapon();
			return true;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

	AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);
	if (!AddedWeapon)
	{
		OutReplacedWeaponClass = nullptr;
		return false;
	}

	OwnedWeapons.Add(AddedWeapon);

	if (ReplacedWeapon)
	{
		ReplacedWeapon->DeactivateWeapon();
		OwnedWeapons.Remove(ReplacedWeapon);
		ReplacedWeapon->Destroy();
	}

	CurrentWeapon = AddedWeapon;
	CurrentWeapon->ActivateWeapon();

	return true;
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (IsValid(Weapon) && Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

AShooterPickupBase* AShooterCharacter::FindBestPickupCandidate()
{
	CleanPickupCandidates();

	AShooterPickupBase* BestPickup = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AShooterPickupBase>& PickupCandidate : PickupCandidates)
	{
		AShooterPickupBase* Pickup = PickupCandidate.Get();
		if (!Pickup || !Pickup->CanManualPickup(this))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(GetActorLocation(), Pickup->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestPickup = Pickup;
			BestDistanceSq = DistanceSq;
		}
	}

	return BestPickup;
}

void AShooterCharacter::CleanPickupCandidates()
{
	PickupCandidates.RemoveAllSwap([](const TWeakObjectPtr<AShooterPickupBase>& PickupCandidate)
	{
		return !PickupCandidate.IsValid() || !PickupCandidate->IsPickupEnabled();
	});
}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}
