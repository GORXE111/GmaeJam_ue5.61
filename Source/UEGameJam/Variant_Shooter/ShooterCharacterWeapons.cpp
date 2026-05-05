// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

void AShooterCharacter::DoStartFiring()
{
	if (!IsValid(CurrentWeapon))
	{
		CurrentWeapon = nullptr;
		return;
	}

	if (CurrentWeapon->GetBulletCount() <= 0)
	{
		ThrowCurrentWeapon();
		return;
	}

	CurrentWeapon->StartFiring();
}

void AShooterCharacter::DoStopFiring()
{
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// Single-slot weapon character: switching is intentionally disabled.
}

bool AShooterCharacter::ShouldAutoPickupWeapon() const
{
	return !IsValid(CurrentWeapon);
}

bool AShooterCharacter::ReplaceCurrentWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass, TSubclassOf<AShooterWeapon>& OutReplacedWeaponClass)
{
	OutReplacedWeaponClass = nullptr;

	if (!WeaponClass || !GetWorld())
	{
		return false;
	}

	AShooterWeapon* ReplacedWeapon = IsValid(CurrentWeapon) ? CurrentWeapon.Get() : nullptr;
	if (!ReplacedWeapon)
	{
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

	AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);
	if (!AddedWeapon)
	{
		return false;
	}

	if (ReplacedWeapon)
	{
		OutReplacedWeaponClass = ReplacedWeapon->GetClass();
		ReplacedWeapon->DeactivateWeapon();
		ReplacedWeapon->Destroy();
	}

	CurrentWeapon = AddedWeapon;
	CurrentWeapon->ActivateWeapon();

	return true;
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	if (!IsValid(Weapon))
	{
		return;
	}

	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	Weapon->AttachToActor(this, AttachmentRule);
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, ThirdPersonWeaponSocket);
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// Firing montage playback is currently left to animation Blueprints.
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	const UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent();
	if (!FirstPersonCamera)
	{
		return GetActorLocation() + GetActorForwardVector() * MaxAimDistance;
	}

	FHitResult OutHit;
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + (FirstPersonCamera->GetForwardVector() * MaxAimDistance);

	UWorld* World = GetWorld();
	if (!World)
	{
		return End;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	TSubclassOf<AShooterWeapon> IgnoredReplacedWeaponClass;
	ReplaceCurrentWeaponClass(WeaponClass, IgnoredReplacedWeaponClass);
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	if (!IsValid(Weapon) || Weapon != CurrentWeapon.Get())
	{
		return;
	}

	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());
	SetCharacterAnimInstanceClasses(Weapon->GetFirstPersonAnimInstanceClass(), Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// The character returns to unarmed animation when the single weapon slot is cleared.
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

void AShooterCharacter::ThrowCurrentWeapon()
{
	if (!IsValid(CurrentWeapon))
	{
		CurrentWeapon = nullptr;
		ApplyUnarmedAnimInstances();
		OnBulletCountUpdated.Broadcast(0, 0);
		return;
	}

	AShooterWeapon* WeaponToThrow = CurrentWeapon.Get();
	WeaponToThrow->StopFiring();
	WeaponToThrow->SpawnThrownWeapon(GetWeaponTargetLocation(), KickDamage, KickDamageType, KickPushStrength, GetController());

	CurrentWeapon = nullptr;
	WeaponToThrow->Destroy();

	ApplyUnarmedAnimInstances();
	OnBulletCountUpdated.Broadcast(0, 0);
}

void AShooterCharacter::CacheDefaultUnarmedAnimInstances()
{
	if (USkeletalMeshComponent* SelfMesh = GetFirstPersonMesh())
	{
		DefaultFirstPersonUnarmedAnimInstanceClass = SelfMesh->GetAnimClass();
	}

	if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
	{
		DefaultThirdPersonUnarmedAnimInstanceClass = ThirdPersonMesh->GetAnimClass();
	}
}

void AShooterCharacter::ApplyUnarmedAnimInstances()
{
	const TSubclassOf<UAnimInstance> FirstPersonAnimClass = FirstPersonUnarmedAnimInstanceClass
		? FirstPersonUnarmedAnimInstanceClass
		: DefaultFirstPersonUnarmedAnimInstanceClass;
	const TSubclassOf<UAnimInstance> ThirdPersonAnimClass = ThirdPersonUnarmedAnimInstanceClass
		? ThirdPersonUnarmedAnimInstanceClass
		: DefaultThirdPersonUnarmedAnimInstanceClass;

	SetCharacterAnimInstanceClasses(FirstPersonAnimClass, ThirdPersonAnimClass);
}

void AShooterCharacter::SetCharacterAnimInstanceClasses(TSubclassOf<UAnimInstance> FirstPersonAnimClass, TSubclassOf<UAnimInstance> ThirdPersonAnimClass)
{
	if (USkeletalMeshComponent* SelfMesh = GetFirstPersonMesh())
	{
		SelfMesh->SetAnimInstanceClass(FirstPersonAnimClass.Get());
	}

	if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
	{
		ThirdPersonMesh->SetAnimInstanceClass(ThirdPersonAnimClass.Get());
	}
}

void AShooterCharacter::ClearCurrentWeapon(bool bDestroyWeapon)
{
	if (!IsValid(CurrentWeapon))
	{
		CurrentWeapon = nullptr;
		ApplyUnarmedAnimInstances();
		OnBulletCountUpdated.Broadcast(0, 0);
		return;
	}

	AShooterWeapon* WeaponToClear = CurrentWeapon.Get();
	WeaponToClear->DeactivateWeapon();
	CurrentWeapon = nullptr;

	if (bDestroyWeapon)
	{
		WeaponToClear->Destroy();
	}

	ApplyUnarmedAnimInstances();
	OnBulletCountUpdated.Broadcast(0, 0);
}
