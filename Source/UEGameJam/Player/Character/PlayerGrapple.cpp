// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"

#include "Components/CapsuleComponent.h"
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

	UWorld* World = GetWorld();
	const float CurrentWorldTime = World ? World->GetTimeSeconds() : 0.0f;
	const FGsPlayerTuningRow& PlayerTuning = GetPlayerTuning();
	if ((CurrentWorldTime - LastFalculaTime) < PlayerTuning.GrappleCooldown)
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

	const UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();
	const FVector StartLocation = PlayerCapsuleComponent ? PlayerCapsuleComponent->Bounds.Origin : GetActorLocation();
	const FVector TargetLocation = GrapplePoint->GetGrappleTargetLocation();

	const FVector ToTarget = TargetLocation - StartLocation;
	const float TargetDistance = ToTarget.Size();
	const float MaxGrappleDistance = GrapplePoint->GetGrappleProximityRadius();
	if (MaxGrappleDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector DirectDirection = ToTarget.GetSafeNormal();
	if (DirectDirection.IsNearlyZero())
	{
		return;
	}

	const float DistanceAlpha = FMath::Clamp(TargetDistance / MaxGrappleDistance, 0.0f, 1.0f);
	const float LaunchSpeed = PlayerTuning.GrappleDirectSpeed * DistanceAlpha;
	const FVector LaunchVelocity = DirectDirection * LaunchSpeed;
	LaunchCharacter(LaunchVelocity, true, true);
	LastFalculaTime = CurrentWorldTime;
	bIsFalculaLaunching = true;

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
