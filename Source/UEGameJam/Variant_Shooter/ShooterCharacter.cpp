// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterPickupBase.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "ShooterWeapon.h"

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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
	LastSafeLocation = GetActorLocation();
	LastSafeRotation = GetActorRotation();
	bHasSafeLocation = true;
	LastFallRecoveryTime = -SafeLandingMinInterval;

	CacheDefaultUnarmedAnimInstances();
	if (!IsValid(CurrentWeapon))
	{
		ApplyUnarmedAnimInstances();
	}

	// initialize camera FOV after Blueprint overrides have been applied
	GetFirstPersonCameraComponent()->SetFieldOfView(DefaultCameraFOV);

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSlide(DeltaSeconds);
	UpdateWallJumpContact();

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent
		&& MovementComponent->IsFalling()
		&& bHasSafeLocation
		&& !bIsRecoveringFromFall
		&& FallResetDepth > 0.0f)
	{
		UWorld* World = GetWorld();
		const float CurrentWorldTime = World ? World->GetTimeSeconds() : 0.0f;
		if ((CurrentWorldTime - LastFallRecoveryTime) >= SafeLandingMinInterval
			&& GetActorLocation().Z <= (LastSafeLocation.Z - FallResetDepth))
		{
			RecoverFromDeepFall();
		}
	}

	UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent();
	if (!FirstPersonCamera)
	{
		return;
	}

	const float TargetFOV = GetVelocity().Size2D() >= RunFOVSpeedThreshold ? RunningCameraFOV : DefaultCameraFOV;
	const float NewFOV = FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaSeconds, CameraFOVInterpSpeed);
	FirstPersonCamera->SetFieldOfView(NewFOV);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopSlide(true);

	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(ActionTimer);
}

void AShooterCharacter::UpdateSafeLandingTransform()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if ((World->GetTimeSeconds() - LastFallRecoveryTime) < SafeLandingMinInterval)
	{
		return;
	}

	LastSafeLocation = GetActorLocation();
	LastSafeRotation = GetActorRotation();
	bHasSafeLocation = true;
}

void AShooterCharacter::RecoverFromDeepFall()
{
	if (bIsRecoveringFromFall || !bHasSafeLocation)
	{
		return;
	}

	bIsRecoveringFromFall = true;

	if (UWorld* World = GetWorld())
	{
		LastFallRecoveryTime = World->GetTimeSeconds();
	}

	StopSlide(true);
	FinishCharacterAction();
	ClearWallJumpContact();
	bHasWallJumpedSinceLanded = false;
	LastWallJumpNormal = FVector::ZeroVector;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->StopActiveMovement();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	SetActorLocationAndRotation(LastSafeLocation, LastSafeRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	bIsRecoveringFromFall = false;
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AShooterCharacter::OnMoveInputCompleted);
		}

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

		// Slide
		if (SlideAction)
		{
			EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AShooterCharacter::DoSlide);
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
	StopSlide(true);

	ClearCurrentWeapon(false);

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

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
