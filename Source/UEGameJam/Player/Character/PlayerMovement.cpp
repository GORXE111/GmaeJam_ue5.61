// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"

void AGsPlayer::DoMove(float Right, float Forward)
{
	if (bIsDead || !GetController())
	{
		return;
	}

	constexpr float SlideInputDeadZone = 0.1f;
	const FVector2D MoveVector(Right, Forward);
	CachedMoveInput = MoveVector.SizeSquared() > FMath::Square(SlideInputDeadZone) ? MoveVector : FVector2D::ZeroVector;

	if (IsSliding() || IsDashing())
	{
		return;
	}

	AddMovementInput(GetActorRightVector(), Right);
	AddMovementInput(GetActorForwardVector(), Forward);
}

void AGsPlayer::DoJumpStart()
{
	if (bIsDead)
	{
		return;
	}

	if (IsDashing())
	{
		return;
	}

	if (IsSliding())
	{
		if (!StopSlide(false))
		{
			return;
		}
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		if (PlayerMovementComponent->IsFalling() && TryWallJump())
		{
			return;
		}
	}

	Jump();
}

void AGsPlayer::DoJumpEnd()
{
	if (bIsDead)
	{
		return;
	}

	StopJumping();
}

void AGsPlayer::OnMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	CachedMoveInput = FVector2D::ZeroVector;
}

bool AGsPlayer::StartSlide()
{
	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();

	if (bIsDead || !PlayerMovementComponent || !PlayerCapsuleComponent || !PlayerMovementComponent->IsMovingOnGround())
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

	if (!TryStartCharacterAction(EUEGameJamPlayerAction::Slide, -1.0f))
	{
		return false;
	}

	OriginalSlideCapsuleHalfHeight = PlayerCapsuleComponent->GetUnscaledCapsuleHalfHeight();
	OriginalSlideMaxWalkSpeed = PlayerMovementComponent->MaxWalkSpeed;

	const float TargetCapsuleHalfHeight = FMath::Clamp(
		SlideCapsuleHalfHeight,
		PlayerCapsuleComponent->GetUnscaledCapsuleRadius(),
		OriginalSlideCapsuleHalfHeight);
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - TargetCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * PlayerCapsuleComponent->GetShapeScale();

	PlayerCapsuleComponent->SetCapsuleHalfHeight(TargetCapsuleHalfHeight, true);
	AddActorWorldOffset(FVector(0.0f, 0.0f, -WorldHalfHeightDelta), false);

	PlayerMovementComponent->MaxWalkSpeed = FMath::Max(SlideSpeed, SlideMaxSpeed);
	CurrentSlideSpeed = SlideSpeed;

	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = PlayerMovementComponent->Velocity.Z;
	PlayerMovementComponent->Velocity = NewVelocity;

	if (SlideMontage && FirstPersonMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(SlideMontage);
		}
	}

	return true;
}

bool AGsPlayer::StartDash()
{
	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (bIsDead || !PlayerMovementComponent)
	{
		return false;
	}

	if (IsCharacterActionActive())
	{
		return false;
	}

	UWorld* World = GetWorld();
	const float CurrentWorldTime = World ? World->GetTimeSeconds() : 0.0f;
	if ((CurrentWorldTime - LastDashTime) < DashCooldown)
	{
		return false;
	}

	const bool bIsAirborne = PlayerMovementComponent->IsFalling();
	if (bIsAirborne && bHasDashedSinceLanded)
	{
		return false;
	}

	FVector ForwardDirection = FVector::VectorPlaneProject(GetActorForwardVector(), FVector::UpVector);
	if (!ForwardDirection.Normalize())
	{
		return false;
	}

	if (!TryStartCharacterAction(EUEGameJamPlayerAction::Dash, -1.0f))
	{
		return false;
	}

	DashDirection = ForwardDirection;
	LastDashTime = CurrentWorldTime;
	PreDashVelocity = PlayerMovementComponent->Velocity;
	PreDashMovementMode = PlayerMovementComponent->MovementMode;
	PreDashCustomMovementMode = PlayerMovementComponent->CustomMovementMode;
	DashStartLocation = GetActorLocation();
	DashTargetLocation = DashStartLocation + (DashDirection * (DashSpeed * DashDuration));
	DashTargetLocation.Z = DashStartLocation.Z;
	CurrentDashElapsedTime = 0.0f;

	if (bIsAirborne)
	{
		bHasDashedSinceLanded = true;
	}

	PlayerMovementComponent->StopMovementImmediately();
	PlayerMovementComponent->StopActiveMovement();
	PlayerMovementComponent->DisableMovement();

	if (DashDuration <= KINDA_SMALL_NUMBER)
	{
		FinishDash();
	}

	return true;
}

bool AGsPlayer::TryGetSlideInputDirection(FVector& OutSlideDirection) const
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

	OutSlideDirection = (GetActorForwardVector() * LocalSlideDirection.X) + (GetActorRightVector() * LocalSlideDirection.Y);
	OutSlideDirection.Z = 0.0f;
	return OutSlideDirection.Normalize();
}

bool AGsPlayer::StopSlide(bool bForceRestore)
{
	if (!IsSliding())
	{
		return true;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();

	if (!PlayerMovementComponent || !PlayerCapsuleComponent)
	{
		StopSlideMontage();
		CurrentSlideSpeed = 0.0f;
		FinishCharacterAction();
		return true;
	}

	if (!bForceRestore && !CanRestoreSlideCapsule())
	{
		return false;
	}

	const float CurrentCapsuleHalfHeight = PlayerCapsuleComponent->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * PlayerCapsuleComponent->GetShapeScale();

	if (HalfHeightDelta > 0.0f)
	{
		AddActorWorldOffset(FVector(0.0f, 0.0f, WorldHalfHeightDelta), false);
		PlayerCapsuleComponent->SetCapsuleHalfHeight(OriginalSlideCapsuleHalfHeight, true);
	}

	PlayerMovementComponent->MaxWalkSpeed = OriginalSlideMaxWalkSpeed;
	CurrentSlideSpeed = 0.0f;
	StopSlideMontage();
	FinishCharacterAction();

	return true;
}

bool AGsPlayer::CanRestoreSlideCapsule() const
{
	UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (!PlayerCapsuleComponent || !World)
	{
		return true;
	}

	const float CurrentCapsuleHalfHeight = PlayerCapsuleComponent->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	if (HalfHeightDelta <= 0.0f)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SlideCapsuleRestore), false, this);
	FCollisionResponseParams ResponseParams;
	PlayerCapsuleComponent->InitSweepCollisionParams(QueryParams, ResponseParams);

	const float ShapeScale = PlayerCapsuleComponent->GetShapeScale();
	const float RestoredCapsuleRadius = PlayerCapsuleComponent->GetUnscaledCapsuleRadius() * ShapeScale;
	const float RestoredCapsuleHalfHeight = OriginalSlideCapsuleHalfHeight * ShapeScale;
	const FVector RestoreLocation = GetActorLocation() + FVector(0.0f, 0.0f, HalfHeightDelta * ShapeScale);
	const FCollisionShape RestoredCapsuleShape = FCollisionShape::MakeCapsule(RestoredCapsuleRadius, RestoredCapsuleHalfHeight);

	return !World->OverlapBlockingTestByChannel(
		RestoreLocation,
		GetActorQuat(),
		PlayerCapsuleComponent->GetCollisionObjectType(),
		RestoredCapsuleShape,
		QueryParams,
		ResponseParams);
}

void AGsPlayer::StopSlideMontage()
{
	if (!SlideMontage || !FirstPersonMesh)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = FirstPersonMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.15f, SlideMontage);
	}
}

void AGsPlayer::UpdateSlide(float DeltaSeconds)
{
	if (!IsSliding())
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent)
	{
		StopSlide(true);
		return;
	}

	if (!PlayerMovementComponent->IsMovingOnGround())
	{
		StopSlide(true);
		return;
	}

	bool bShouldTryStopForLowSpeed = true;
	const FFindFloorResult& CurrentFloor = PlayerMovementComponent->CurrentFloor;
	if (CurrentFloor.IsWalkableFloor())
	{
		FVector FloorNormal = CurrentFloor.HitResult.ImpactNormal.IsNearlyZero()
			? CurrentFloor.HitResult.Normal
			: CurrentFloor.HitResult.ImpactNormal;
		FloorNormal = FloorNormal.GetSafeNormal();

		FVector DownhillDirection = FVector::VectorPlaneProject(FVector::DownVector, FloorNormal);
		DownhillDirection = DownhillDirection.GetSafeNormal();
		const float DownhillAlignment = FVector::DotProduct(SlideDirection, DownhillDirection);

		if (DownhillAlignment > KINDA_SMALL_NUMBER && SlideSlopeAcceleration > 0.0f)
		{
			CurrentSlideSpeed = FMath::Min(SlideMaxSpeed, CurrentSlideSpeed + (SlideSlopeAcceleration * DownhillAlignment * DeltaSeconds));
			bShouldTryStopForLowSpeed = false;
		}
		else
		{
			CurrentSlideSpeed = FMath::Max(0.0f, CurrentSlideSpeed - (SlideDeceleration * DeltaSeconds));
		}
	}
	else
	{
		CurrentSlideSpeed = FMath::Max(0.0f, CurrentSlideSpeed - (SlideDeceleration * DeltaSeconds));
	}

	if (bShouldTryStopForLowSpeed && CurrentSlideSpeed < SlideStopSpeed)
	{
		if (StopSlide(false))
		{
			return;
		}

		CurrentSlideSpeed = SlideStopSpeed;
	}

	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = PlayerMovementComponent->Velocity.Z;
	PlayerMovementComponent->Velocity = NewVelocity;
}

void AGsPlayer::UpdateDash(float DeltaSeconds)
{
	if (!IsDashing())
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent)
	{
		AbortDash();
		return;
	}

	CurrentDashElapsedTime += DeltaSeconds;
	const float DashAlpha = DashDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentDashElapsedTime / DashDuration, 0.0f, 1.0f)
		: 1.0f;
	const bool bReachedDestination = DashAlpha >= 1.0f;

	FVector DesiredLocation = FMath::Lerp(DashStartLocation, DashTargetLocation, DashAlpha);
	DesiredLocation.Z = DashStartLocation.Z;

	FHitResult SweepHit;
	SetActorLocation(DesiredLocation, true, &SweepHit, ETeleportType::None);

	if (SweepHit.bBlockingHit || bReachedDestination)
	{
		FinishDash();
	}
}

void AGsPlayer::ClearDashState()
{
	DashDirection = FVector::ForwardVector;
	PreDashVelocity = FVector::ZeroVector;
	PreDashMovementMode = MOVE_Walking;
	PreDashCustomMovementMode = 0;
	DashStartLocation = FVector::ZeroVector;
	DashTargetLocation = FVector::ZeroVector;
	CurrentDashElapsedTime = 0.0f;
}

void AGsPlayer::UpdateWallJumpContact()
{
	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();

	if (!PlayerMovementComponent || !PlayerMovementComponent->IsFalling())
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

void AGsPlayer::ClearWallJumpContact()
{
	LastWallContactNormal = FVector::ZeroVector;
	bHasRecentWallContact = false;
}

bool AGsPlayer::TryWallJump()
{
	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent || !PlayerMovementComponent->IsFalling())
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

bool AGsPlayer::FindWallJumpSurface(FVector& OutWallNormal) const
{
	OutWallNormal = FVector::ZeroVector;

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (!PlayerMovementComponent || !PlayerCapsuleComponent || !World || WallJumpTraceDistance <= 0.0f)
	{
		return false;
	}

	FVector HorizontalVelocity = PlayerMovementComponent->Velocity;
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
	PlayerCapsuleComponent->InitSweepCollisionParams(QueryParams, ResponseParams);

	const FVector Start = GetActorLocation();
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		PlayerCapsuleComponent->GetScaledCapsuleRadius(),
		PlayerCapsuleComponent->GetScaledCapsuleHalfHeight());

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
			PlayerCapsuleComponent->GetCollisionObjectType(),
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
