// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

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
