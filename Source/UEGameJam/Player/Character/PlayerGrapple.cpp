// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Player/Scene/GsGrapplePoint.h"

void AGsPlayer::DoFalcula()
{
	if (bIsDead)
	{
		return;
	}

	AGsGrapplePoint* GrapplePoint = FindReachableGrapplePoint();
	if (!GrapplePoint)
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent)
	{
		return;
	}

	const FVector StartLocation = GetActorLocation();
	const FVector TargetLocation = GrapplePoint->GetGrappleTargetLocation();
	const float HeightDelta = TargetLocation.Z - StartLocation.Z;
	if (HeightDelta <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Gravity = FMath::Abs(PlayerMovementComponent->GetGravityZ());
	if (Gravity <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float TimeToApex = FMath::Sqrt((2.0f * HeightDelta) / Gravity);
	if (TimeToApex <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	constexpr float GrappleForwardCarryScale = 1.0f;

	FVector ToTargetHorizontal = TargetLocation - StartLocation;
	ToTargetHorizontal.Z = 0.0f;
	const float HorizontalDistance = ToTargetHorizontal.Size();
	if (HorizontalDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector DirectionToTarget = ToTargetHorizontal / HorizontalDistance;
	const float CarryDistance = HorizontalDistance * GrappleForwardCarryScale;
	const FVector ForwardTarget = TargetLocation + (DirectionToTarget * CarryDistance);

	FVector HorizontalVelocity = ForwardTarget - StartLocation;
	HorizontalVelocity.Z = 0.0f;
	HorizontalVelocity /= TimeToApex;

	const float VerticalSpeed = Gravity * TimeToApex;
	const FVector LaunchVelocity = HorizontalVelocity + (FVector::UpVector * VerticalSpeed);
	LaunchCharacter(LaunchVelocity, true, true);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("FalculaAction: Grapple point reachable"));
	}
}

AGsGrapplePoint* AGsPlayer::FindReachableGrapplePoint() const
{
	UWorld* World = GetWorld();
	AController* PlayerController = GetController();
	if (!World || !PlayerController)
	{
		return nullptr;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector ViewDirection = ViewRotation.Vector();
	constexpr float MaxAimAngleDegrees = 40.0f;
	const float MinAimDot = FMath::Cos(FMath::DegreesToRadians(MaxAimAngleDegrees));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerFalculaVisibility), false, this);
	QueryParams.AddIgnoredActor(this);

	AGsGrapplePoint* BestGrapplePoint = nullptr;
	float BestAimDot = MinAimDot;

	for (TActorIterator<AGsGrapplePoint> GrapplePointIt(World); GrapplePointIt; ++GrapplePointIt)
	{
		AGsGrapplePoint* GrapplePoint = *GrapplePointIt;
		if (!IsValid(GrapplePoint) || !GrapplePoint->IsPlayerNearbyFor(this))
		{
			continue;
		}

		const FVector TargetLocation = GrapplePoint->GetGrappleTargetLocation();
		const FVector ToTarget = TargetLocation - ViewLocation;
		const float TargetDistance = ToTarget.Size();
		if (TargetDistance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector TargetDirection = ToTarget / TargetDistance;
		const float AimDot = FVector::DotProduct(ViewDirection, TargetDirection);
		if (AimDot < BestAimDot)
		{
			continue;
		}

		FHitResult VisibilityHit;
		const bool bBlocked = World->LineTraceSingleByChannel(
			VisibilityHit,
			ViewLocation,
			TargetLocation,
			ECC_Visibility,
			QueryParams);
		if (bBlocked && VisibilityHit.GetActor() != GrapplePoint)
		{
			continue;
		}

		BestAimDot = AimDot;
		BestGrapplePoint = GrapplePoint;
	}

	return BestGrapplePoint;
}
