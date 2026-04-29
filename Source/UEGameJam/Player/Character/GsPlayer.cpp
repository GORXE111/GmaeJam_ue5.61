// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "UEGameJam.h"

AGsPlayer::AGsPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	MeleeDamageCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("MeleeDamageCollision"));
	MeleeDamageCollision->SetupAttachment(GetRootComponent());
	MeleeDamageCollision->SetRelativeLocation(FVector(140.0f, 0.0f, 0.0f));
	MeleeDamageCollision->InitBoxExtent(FVector(70.0f, 50.0f, 50.0f));
	MeleeDamageCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeleeDamageCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeleeDamageCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeleeDamageCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	MeleeDamageCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	MeleeDamageCollision->SetGenerateOverlapEvents(true);

	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	PlayerMovementComponent->BrakingDecelerationFalling = 1500.0f;
	PlayerMovementComponent->AirControl = 0.5f;
	PlayerMovementComponent->RotationRate = FRotator(0.0f, 600.0f, 0.0f);

	MeleeDamageType = UDamageType::StaticClass();
}

void AGsPlayer::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	bIsDead = false;
	bHasDashedSinceLanded = false;
	PreDashVelocity = FVector::ZeroVector;
	PreDashMovementMode = MOVE_Walking;
	PreDashCustomMovementMode = 0;
	DashStartLocation = FVector::ZeroVector;
	DashTargetLocation = FVector::ZeroVector;
	CurrentDashElapsedTime = 0.0f;
	LastSafeLocation = GetActorLocation();
	LastSafeRotation = GetActorRotation();
	bHasSafeLocation = true;
	LastFallRecoveryTime = -SafeLandingMinInterval;
	LastDashTime = -DashCooldown;

	if (UWorld* World = GetWorld())
	{
		LastDashTime = World->GetTimeSeconds() - DashCooldown;
	}

	if (FirstPersonCameraComponent)
	{
		FirstPersonCameraComponent->SetFieldOfView(DefaultCameraFOV);
	}

	OnDamaged.Broadcast(GetLifePercent());
}

void AGsPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	UpdateSlide(DeltaSeconds);
	UpdateDash(DeltaSeconds);
	UpdateWallJumpContact();

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (PlayerMovementComponent
		&& PlayerMovementComponent->IsFalling()
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

	if (!FirstPersonCameraComponent)
	{
		return;
	}

	const float TargetFOV = IsDashing()
		? DashCameraFOV
		: (GetVelocity().Size2D() >= RunFOVSpeedThreshold ? RunningCameraFOV : DefaultCameraFOV);
	const float NewFOV = FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView, TargetFOV, DeltaSeconds, CameraFOVInterpSpeed);
	FirstPersonCameraComponent->SetFieldOfView(NewFOV);
}

void AGsPlayer::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopSlide(true);
	if (IsDashing())
	{
		AbortDash();
	}
	else
	{
		ClearDashState();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActionTimer);
		World->GetTimerManager().ClearTimer(MeleeHitTimer);
		World->GetTimerManager().ClearTimer(DeferredDestroyTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void AGsPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGsPlayer::DoJumpStart);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGsPlayer::DoJumpEnd);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGsPlayer::MoveInput);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AGsPlayer::OnMoveInputCompleted);
		}

		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGsPlayer::LookInput);
		}

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AGsPlayer::DoStartFiring);
		}

		if (SlideAction)
		{
			EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AGsPlayer::DoSlide);
			EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AGsPlayer::DoSlideEnd);
		}

		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AGsPlayer::DoDash);
		}
	}
	else
	{
		UE_LOG(LogUEGameJam, Error, TEXT("'%s' 找不到 Enhanced Input Component。"), *GetNameSafe(this));
	}
}

void AGsPlayer::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bHasDashedSinceLanded = false;
	ClearWallJumpContact();
	bHasWallJumpedSinceLanded = false;
	LastWallJumpNormal = FVector::ZeroVector;
	UpdateSafeLandingTransform();
}

void AGsPlayer::MoveInput(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AGsPlayer::LookInput(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoAim(LookAxisVector.X, LookAxisVector.Y);
}

float AGsPlayer::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	(void)DamageEvent;
	(void)EventInstigator;
	(void)DamageCauser;

	if (bIsDead || Damage <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHP, Damage);
	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	OnDamaged.Broadcast(GetLifePercent());

	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	return AppliedDamage;
}

void AGsPlayer::DoAim(float Yaw, float Pitch)
{
	if (bIsDead || !GetController())
	{
		return;
	}

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AGsPlayer::DoStartFiring()
{
	StartMeleeAttack();
}

void AGsPlayer::DoSlide()
{
	if (bIsDead)
	{
		return;
	}

	if (!IsSliding())
	{
		StartSlide();
	}
}

void AGsPlayer::DoSlideEnd()
{
	if (bIsDead || !IsSliding())
	{
		return;
	}

	StopSlide(false);
}

void AGsPlayer::DoDash()
{
	if (bIsDead)
	{
		return;
	}

	StartDash();
}

bool AGsPlayer::IsCharacterActionActive() const
{
	return CurrentAction != EUEGameJamPlayerAction::None;
}

bool AGsPlayer::IsSliding() const
{
	return CurrentAction == EUEGameJamPlayerAction::Slide;
}

bool AGsPlayer::IsDashing() const
{
	return CurrentAction == EUEGameJamPlayerAction::Dash;
}

float AGsPlayer::GetLifePercent() const
{
	return MaxHP > 0.0f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0f;
}

bool AGsPlayer::IsDead() const
{
	return bIsDead;
}

bool AGsPlayer::TryStartCharacterAction(EUEGameJamPlayerAction Action, float Duration)
{
	if (IsCharacterActionActive())
	{
		return false;
	}

	CurrentAction = Action;

	if (!GetWorld())
	{
		return true;
	}

	if (Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ActionTimer, this, &AGsPlayer::FinishCharacterAction, Duration, false);
	}
	else if (FMath::IsNearlyZero(Duration))
	{
		FinishCharacterAction();
	}

	return true;
}

void AGsPlayer::FinishCharacterAction()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActionTimer);
	}

	CurrentAction = EUEGameJamPlayerAction::None;
}

void AGsPlayer::FinishDash()
{
	if (!IsDashing())
	{
		ClearDashState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(PreDashMovementMode, PreDashCustomMovementMode);

		FVector PreDashHorizontalVelocity = PreDashVelocity;
		PreDashHorizontalVelocity.Z = 0.0f;

		const float ForwardSpeed = FMath::Max(0.0f, FVector::DotProduct(PreDashHorizontalVelocity, DashDirection));
		FVector RestoredVelocity = DashDirection * ForwardSpeed;
		RestoredVelocity.Z = PreDashVelocity.Z;

		PlayerMovementComponent->Velocity = RestoredVelocity;
	}

	ClearDashState();
	FinishCharacterAction();
}

void AGsPlayer::AbortDash()
{
	if (!IsDashing())
	{
		ClearDashState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(PreDashMovementMode, PreDashCustomMovementMode);
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->StopActiveMovement();
	}

	ClearDashState();
	FinishCharacterAction();
}

void AGsPlayer::UpdateSafeLandingTransform()
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

void AGsPlayer::RecoverFromDeepFall()
{
	if (bIsDead || bIsRecoveringFromFall || !bHasSafeLocation)
	{
		return;
	}

	bIsRecoveringFromFall = true;

	if (UWorld* World = GetWorld())
	{
		LastFallRecoveryTime = World->GetTimeSeconds();
	}

	StopSlide(true);
	if (IsDashing())
	{
		AbortDash();
	}
	else
	{
		ClearDashState();
		FinishCharacterAction();
	}
	bHasDashedSinceLanded = false;
	ClearWallJumpContact();
	bHasWallJumpedSinceLanded = false;
	LastWallJumpNormal = FVector::ZeroVector;

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->StopActiveMovement();
		PlayerMovementComponent->Velocity = FVector::ZeroVector;
		PlayerMovementComponent->SetMovementMode(MOVE_Walking);
	}

	SetActorLocationAndRotation(LastSafeLocation, LastSafeRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->Velocity = FVector::ZeroVector;
	}

	bIsRecoveringFromFall = false;
}

void AGsPlayer::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	StopSlide(true);
	if (IsDashing())
	{
		AbortDash();
	}
	else
	{
		ClearDashState();
		FinishCharacterAction();
	}
	bHasDashedSinceLanded = false;
	ClearWallJumpContact();
	bHasWallJumpedSinceLanded = false;
	LastWallJumpNormal = FVector::ZeroVector;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MeleeHitTimer);
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->StopActiveMovement();
		PlayerMovementComponent->DisableMovement();
	}

	DisableInput(Cast<APlayerController>(GetController()));
	OnDamaged.Broadcast(0.0f);
	BP_OnDeath();

	if (DeferredDestructionTime <= 0.0f)
	{
		Destroy();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DeferredDestroyTimer, this, &AGsPlayer::OnDeferredDestroy, DeferredDestructionTime, false);
	}
}

void AGsPlayer::OnDeferredDestroy()
{
	Destroy();
}
