// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Player/Scene/GsGrapplePoint.h"

static constexpr float GrappleFinishDistance = 80.0f;
static constexpr float GrappleExitSpeedScale = 0.25f;
static constexpr float GrappleEaseInStrength = 0.65f;

void AGsPlayer::DoFalcula()
{
	if (bIsDead)
	{
		return;
	}

	if (bIsFalculaLaunching || IsCharacterActionActive())
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
	if (TargetDistance <= GrappleFinishDistance)
	{
		return;
	}

	const FVector DirectDirection = ToTarget.GetSafeNormal();
	if (DirectDirection.IsNearlyZero())
	{
		return;
	}

	const float GrappleSpeed = PlayerTuning.GrappleDirectSpeed;
	if (GrappleSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	GrappleDirection = DirectDirection;
	GrappleStartLocation = GetActorLocation();
	GrappleTargetLocation = TargetLocation;
	CurrentGrappleElapsedTime = 0.0f;
	CurrentGrappleDuration = (TargetDistance) / GrappleSpeed;
	PreGrappleMovementMode = PlayerMovementComponent->MovementMode;
	PreGrappleCustomMovementMode = PlayerMovementComponent->CustomMovementMode;
	LastFalculaTime = CurrentWorldTime;
	bIsFalculaLaunching = true;

	PlayerMovementComponent->StopMovementImmediately();
	PlayerMovementComponent->StopActiveMovement();
	PlayerMovementComponent->DisableMovement();

	if (CurrentGrappleDuration <= KINDA_SMALL_NUMBER)
	{
		FinishGrapple();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("FalculaAction: Grapple point reachable"));
	}
}

void AGsPlayer::UpdateGrapple(float DeltaSeconds)
{
	if (!bIsFalculaLaunching)
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent)
	{
		AbortGrapple();
		return;
	}

	CurrentGrappleElapsedTime += DeltaSeconds;
	const float GrappleAlpha = CurrentGrappleDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentGrappleElapsedTime / CurrentGrappleDuration, 0.0f, 1.0f)
		: 1.0f;
	const float EaseInAlpha = FMath::Lerp(GrappleAlpha, GrappleAlpha * GrappleAlpha, GrappleEaseInStrength);
	const bool bTimeFinished = GrappleAlpha >= 1.0f;

	const FVector DesiredLocation = FMath::Lerp(GrappleStartLocation, GrappleTargetLocation, EaseInAlpha);

	FHitResult SweepHit;
	SetActorLocation(DesiredLocation, true, &SweepHit, ETeleportType::None);

	const bool bReachedTargetNearby = FVector::DistSquared(GetActorLocation(), GrappleTargetLocation) <= FMath::Square(GrappleFinishDistance);
	if (SweepHit.bBlockingHit || bReachedTargetNearby || bTimeFinished)
	{
		FinishGrapple();
	}
}

void AGsPlayer::FinishGrapple()
{
	if (!bIsFalculaLaunching)
	{
		ClearGrappleState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(PreGrappleMovementMode, PreGrappleCustomMovementMode);
		PlayerMovementComponent->Velocity = GrappleDirection * (GetPlayerTuning().GrappleDirectSpeed * GrappleExitSpeedScale);
	}

	ClearGrappleState();
}

void AGsPlayer::AbortGrapple()
{
	if (!bIsFalculaLaunching)
	{
		ClearGrappleState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(PreGrappleMovementMode, PreGrappleCustomMovementMode);
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->StopActiveMovement();
	}

	ClearGrappleState();
}

void AGsPlayer::ClearGrappleState()
{
	bIsFalculaLaunching = false;
	GrappleDirection = FVector::ForwardVector;
	GrappleStartLocation = FVector::ZeroVector;
	GrappleTargetLocation = FVector::ZeroVector;
	CurrentGrappleElapsedTime = 0.0f;
	CurrentGrappleDuration = 0.0f;
	PreGrappleMovementMode = MOVE_Falling;
	PreGrappleCustomMovementMode = 0;
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
