// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "AI/ShooterNPC.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

bool AShooterCharacter::IsCharacterActionActive() const
{
	return CurrentAction != EShooterCharacterAction::None;
}

bool AShooterCharacter::IsSliding() const
{
	return CurrentAction == EShooterCharacterAction::Slide;
}

bool AShooterCharacter::TryStartCharacterAction(EShooterCharacterAction Action, float Duration)
{
	if (IsCharacterActionActive())
	{
		return false;
	}

	CurrentAction = Action;

	if (Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ActionTimer, this, &AShooterCharacter::FinishCharacterAction, Duration, false);
	}
	else if (FMath::IsNearlyZero(Duration))
	{
		FinishCharacterAction();
	}

	return true;
}

void AShooterCharacter::FinishCharacterAction()
{
	GetWorld()->GetTimerManager().ClearTimer(ActionTimer);
	CurrentAction = EShooterCharacterAction::None;
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	if (IsSliding())
	{
		return;
	}

	constexpr float SlideInputDeadZone = 0.1f;
	const FVector2D MoveInput(Right, Forward);
	CachedMoveInput = MoveInput.SizeSquared() > FMath::Square(SlideInputDeadZone) ? MoveInput : FVector2D::ZeroVector;

	Super::DoMove(Right, Forward);
}

void AShooterCharacter::OnMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	CachedMoveInput = FVector2D::ZeroVector;
}

void AShooterCharacter::DoJumpStart()
{
	if (IsSliding())
	{
		if (!StopSlide(false))
		{
			return;
		}
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MovementComponent->IsFalling() && TryWallJump())
		{
			return;
		}
	}

	Super::DoJumpStart();
}

void AShooterCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	ClearWallJumpContact();
	bHasWallJumpedSinceLanded = false;
	LastWallJumpNormal = FVector::ZeroVector;
	UpdateSafeLandingTransform();
}

void AShooterCharacter::DoSlide()
{
	if (IsSliding())
	{
		StopSlide(false);
		return;
	}

	StartSlide();
}

bool AShooterCharacter::StartSlide()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();

	if (!MovementComponent || !SelfCapsule || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	if (!TryGetSlideInputDirection(SlideDirection))
	{
		return false;
	}

	FVector ActorForward = GetActorForwardVector();
	ActorForward.Z = 0.0f;
	if (!ActorForward.Normalize() || FVector::DotProduct(SlideDirection, ActorForward) < 0.0f)
	{
		return false;
	}

	if (!TryStartCharacterAction(EShooterCharacterAction::Slide, -1.0f))
	{
		return false;
	}

	OriginalSlideCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	OriginalSlideMaxWalkSpeed = MovementComponent->MaxWalkSpeed;

	const float TargetCapsuleHalfHeight = FMath::Clamp(SlideCapsuleHalfHeight, SelfCapsule->GetUnscaledCapsuleRadius(), OriginalSlideCapsuleHalfHeight);
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - TargetCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * SelfCapsule->GetShapeScale();

	SelfCapsule->SetCapsuleHalfHeight(TargetCapsuleHalfHeight, true);
	AddActorWorldOffset(FVector(0.0f, 0.0f, -WorldHalfHeightDelta), false);

	MovementComponent->MaxWalkSpeed = SlideSpeed;
	CurrentSlideSpeed = SlideSpeed;
	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = NewVelocity;

	return true;
}

bool AShooterCharacter::TryGetSlideInputDirection(FVector& OutSlideDirection) const
{
	constexpr float SlideInputDeadZone = 0.1f;

	OutSlideDirection = FVector::ZeroVector;

	const float RightInput = CachedMoveInput.X;
	const float ForwardInput = CachedMoveInput.Y;
	if (ForwardInput < -SlideInputDeadZone)
	{
		return false;
	}

	FVector LocalSlideDirection = FVector::ZeroVector;
	if (ForwardInput > SlideInputDeadZone)
	{
		LocalSlideDirection.X = 1.0f;

		if (RightInput < -SlideInputDeadZone)
		{
			LocalSlideDirection.Y = -1.0f;
		}
		else if (RightInput > SlideInputDeadZone)
		{
			LocalSlideDirection.Y = 1.0f;
		}
	}
	else if (RightInput < -SlideInputDeadZone)
	{
		LocalSlideDirection.Y = -1.0f;
	}
	else if (RightInput > SlideInputDeadZone)
	{
		LocalSlideDirection.Y = 1.0f;
	}
	else
	{
		return false;
	}

	const FVector ActorForward = GetActorForwardVector();
	const FVector ActorRight = GetActorRightVector();
	OutSlideDirection = (ActorForward * LocalSlideDirection.X) + (ActorRight * LocalSlideDirection.Y);
	OutSlideDirection.Z = 0.0f;

	return OutSlideDirection.Normalize();
}

bool AShooterCharacter::StopSlide(bool bForceRestore)
{
	if (!IsSliding())
	{
		return true;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();

	if (!MovementComponent || !SelfCapsule)
	{
		CurrentSlideSpeed = 0.0f;
		FinishCharacterAction();
		return true;
	}

	if (!bForceRestore && !CanRestoreSlideCapsule())
	{
		return false;
	}

	const float CurrentCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * SelfCapsule->GetShapeScale();

	if (HalfHeightDelta > 0.0f)
	{
		AddActorWorldOffset(FVector(0.0f, 0.0f, WorldHalfHeightDelta), false);
		SelfCapsule->SetCapsuleHalfHeight(OriginalSlideCapsuleHalfHeight, true);
	}

	MovementComponent->MaxWalkSpeed = OriginalSlideMaxWalkSpeed;
	CurrentSlideSpeed = 0.0f;
	FinishCharacterAction();

	return true;
}

bool AShooterCharacter::CanRestoreSlideCapsule() const
{
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (!SelfCapsule || !World)
	{
		return true;
	}

	const float CurrentCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	if (HalfHeightDelta <= 0.0f)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SlideCapsuleRestore), false, this);
	FCollisionResponseParams ResponseParams;
	SelfCapsule->InitSweepCollisionParams(QueryParams, ResponseParams);

	const float ShapeScale = SelfCapsule->GetShapeScale();
	const float RestoredCapsuleRadius = SelfCapsule->GetUnscaledCapsuleRadius() * ShapeScale;
	const float RestoredCapsuleHalfHeight = OriginalSlideCapsuleHalfHeight * ShapeScale;
	const FVector RestoreLocation = GetActorLocation() + FVector(0.0f, 0.0f, HalfHeightDelta * ShapeScale);
	const FCollisionShape RestoredCapsuleShape = FCollisionShape::MakeCapsule(RestoredCapsuleRadius, RestoredCapsuleHalfHeight);

	return !World->OverlapBlockingTestByChannel(
		RestoreLocation,
		GetActorQuat(),
		SelfCapsule->GetCollisionObjectType(),
		RestoredCapsuleShape,
		QueryParams,
		ResponseParams);
}

void AShooterCharacter::UpdateSlide(float DeltaSeconds)
{
	if (!IsSliding())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopSlide(true);
		return;
	}

	if (!MovementComponent->IsMovingOnGround())
	{
		StopSlide(true);
		return;
	}

	CurrentSlideSpeed = FMath::Max(0.0f, CurrentSlideSpeed - (SlideDeceleration * DeltaSeconds));

	if (CurrentSlideSpeed < SlideStopSpeed)
	{
		if (StopSlide(false))
		{
			return;
		}

		CurrentSlideSpeed = SlideStopSpeed;
	}

	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = NewVelocity;
}

void AShooterCharacter::UpdateWallJumpContact()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		ClearWallJumpContact();
		return;
	}

	FVector WallNormal = FVector::ZeroVector;
	if (FindWallJumpSurface(WallNormal))
	{
		LastWallContactNormal = WallNormal;
		bHasRecentWallContact = true;
	}
}

void AShooterCharacter::ClearWallJumpContact()
{
	LastWallContactNormal = FVector::ZeroVector;
	bHasRecentWallContact = false;
}

bool AShooterCharacter::TryWallJump()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return false;
	}

	FVector WallNormal = FVector::ZeroVector;
	const bool bCanUseCachedWall = bHasRecentWallContact
		&& !LastWallContactNormal.IsNearlyZero()
		&& (!bHasWallJumpedSinceLanded || FVector::DotProduct(LastWallContactNormal, LastWallJumpNormal) < WallJumpSameWallDot);

	if (bCanUseCachedWall)
	{
		WallNormal = LastWallContactNormal;
	}
	else if (!FindWallJumpSurface(WallNormal))
	{
		return false;
	}

	const FVector LaunchVelocity = (WallNormal * WallJumpHorizontalStrength) + (FVector::UpVector * WallJumpVerticalStrength);
	LaunchCharacter(LaunchVelocity, true, true);

	LastWallJumpNormal = WallNormal;
	bHasWallJumpedSinceLanded = true;
	ClearWallJumpContact();

	return true;
}

bool AShooterCharacter::FindWallJumpSurface(FVector& OutWallNormal) const
{
	OutWallNormal = FVector::ZeroVector;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (!MovementComponent || !SelfCapsule || !World || WallJumpTraceDistance <= 0.0f)
	{
		return false;
	}

	FVector HorizontalVelocity = MovementComponent->Velocity;
	HorizontalVelocity.Z = 0.0f;
	const bool bUseApproachScore = HorizontalVelocity.SizeSquared() >= FMath::Square(WallJumpMinAirHorizontalSpeed) && HorizontalVelocity.Normalize();

	TArray<FVector> TraceDirections;
	TraceDirections.Reserve(9);

	const auto AddTraceDirection = [&TraceDirections](FVector Direction)
	{
		Direction.Z = 0.0f;
		if (!Direction.Normalize())
		{
			return;
		}

		for (const FVector& ExistingDirection : TraceDirections)
		{
			if (FVector::DotProduct(ExistingDirection, Direction) > 0.99f)
			{
				return;
			}
		}

		TraceDirections.Add(Direction);
	};

	if (bUseApproachScore)
	{
		AddTraceDirection(HorizontalVelocity);
	}

	const FVector ActorForward = GetActorForwardVector();
	const FVector ActorRight = GetActorRightVector();
	AddTraceDirection(ActorForward);
	AddTraceDirection(ActorRight);
	AddTraceDirection(-ActorForward);
	AddTraceDirection(-ActorRight);
	AddTraceDirection(ActorForward + ActorRight);
	AddTraceDirection(ActorForward - ActorRight);
	AddTraceDirection(-ActorForward + ActorRight);
	AddTraceDirection(-ActorForward - ActorRight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallJumpTrace), false, this);
	FCollisionResponseParams ResponseParams;
	SelfCapsule->InitSweepCollisionParams(QueryParams, ResponseParams);

	const FVector Start = GetActorLocation();
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		SelfCapsule->GetScaledCapsuleRadius(),
		SelfCapsule->GetScaledCapsuleHalfHeight());

	float BestScore = -TNumericLimits<float>::Max();

	for (const FVector& TraceDirection : TraceDirections)
	{
		FHitResult Hit;
		const FVector End = Start + (TraceDirection * WallJumpTraceDistance);

		const bool bHitWall = World->SweepSingleByChannel(
			Hit,
			Start,
			End,
			GetActorQuat(),
			SelfCapsule->GetCollisionObjectType(),
			CapsuleShape,
			QueryParams,
			ResponseParams);

		if (!bHitWall || !Hit.bBlockingHit)
		{
			continue;
		}

		FVector SurfaceNormal = Hit.ImpactNormal.IsNearlyZero() ? Hit.Normal : Hit.ImpactNormal;
		if (FMath::Abs(SurfaceNormal.Z) > WallJumpMaxWallNormalZ)
		{
			continue;
		}

		SurfaceNormal.Z = 0.0f;
		if (!SurfaceNormal.Normalize())
		{
			continue;
		}

		if (bHasWallJumpedSinceLanded && FVector::DotProduct(SurfaceNormal, LastWallJumpNormal) >= WallJumpSameWallDot)
		{
			continue;
		}

		const float ApproachDot = bUseApproachScore ? FVector::DotProduct(HorizontalVelocity, -SurfaceNormal) : 0.0f;
		const float ApproachScore = ApproachDot >= WallJumpMinApproachDot ? ApproachDot : 0.0f;
		const float TraceDirectionScore = FMath::Max(0.0f, FVector::DotProduct(TraceDirection, -SurfaceNormal));

		const float VerticalScore = 1.0f - FMath::Abs(Hit.ImpactNormal.Z);
		const float Score = ApproachScore + TraceDirectionScore + VerticalScore - Hit.Time;
		if (Score > BestScore)
		{
			BestScore = Score;
			OutWallNormal = SurfaceNormal;
		}
	}

	return !OutWallNormal.IsNearlyZero();
}

void AShooterCharacter::DoKick()
{
	if (IsCharacterActionActive())
	{
		return;
	}

	float ActionDuration = KickFallbackDuration;

	if (KickMontage)
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonMesh()->GetAnimInstance())
		{
			const float MontageDuration = AnimInstance->Montage_Play(KickMontage);
			if (MontageDuration > 0.0f)
			{
				ActionDuration = MontageDuration;
			}
		}
	}

	if (!TryStartCharacterAction(EShooterCharacterAction::Kick, ActionDuration))
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(KickDamageTimer);
	GetWorld()->GetTimerManager().SetTimer(KickDamageTimer, this, &AShooterCharacter::ExecuteKickDamage, 0.3f, false);
}

void AShooterCharacter::ExecuteKickDamage()
{
	GetWorld()->GetTimerManager().ClearTimer(KickDamageTimer);

	if (!KickDamageCollision || KickDamage <= 0.0f)
	{
		return;
	}

	KickDamageCollision->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	KickDamageCollision->GetOverlappingActors(OverlappingActors);

	const TSubclassOf<UDamageType> DamageTypeClass = KickDamageType;

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (!IsValid(OverlappingActor) || OverlappingActor == this)
		{
			continue;
		}

		AShooterNPC* HitNPC = Cast<AShooterNPC>(OverlappingActor);
		FVector PushVelocity = FVector::ZeroVector;
		if (HitNPC && KickPushStrength > 0.0f)
		{
			FVector PushDirection = HitNPC->GetActorLocation() - GetActorLocation();
			PushDirection.Z = 0.0f;

			if (!PushDirection.Normalize())
			{
				PushDirection = GetActorForwardVector();
				PushDirection.Z = 0.0f;
				PushDirection.Normalize();
			}

			PushVelocity = PushDirection * KickPushStrength;
		}
		
		UGameplayStatics::ApplyDamage(OverlappingActor, KickDamage, GetController(), this, DamageTypeClass);

		if (HitNPC && !PushVelocity.IsNearlyZero())
		{
			HitNPC->ApplyPush(PushVelocity);
		}
	}
}
