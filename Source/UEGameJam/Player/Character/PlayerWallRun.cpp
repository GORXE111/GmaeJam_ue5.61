// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "UEGameJam.h"

void AGsPlayer::StartWallRunDetectionDelay()
{
	bCanCheckWallRun = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		EnableWallRunDetection();
		return;
	}

	World->GetTimerManager().ClearTimer(WallRunDetectionDelayTimer);

	if (WallRunCheckDelay <= KINDA_SMALL_NUMBER)
	{
		EnableWallRunDetection();
		return;
	}

	World->GetTimerManager().SetTimer(
		WallRunDetectionDelayTimer,
		this,
		&AGsPlayer::EnableWallRunDetection,
		WallRunCheckDelay,
		false);
}

void AGsPlayer::EnableWallRunDetection()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WallRunDetectionDelayTimer);
	}

	bCanCheckWallRun = true;
}

void AGsPlayer::ResetWallRunDetection()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WallRunDetectionDelayTimer);
	}

	bCanCheckWallRun = false;
	bHasTriggeredWallRunThisJump = false;
}

void AGsPlayer::UpdateWallRunDetection()
{
	if (!bCanCheckWallRun || bHasTriggeredWallRunThisJump || bIsDead)
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent || !PlayerMovementComponent->IsFalling())
	{
		return;
	}

	FHitResult WallHit;
	FVector WallNormal = FVector::ZeroVector;
	if (!TryFindWallRunSurface(WallHit, WallNormal) || !CanTriggerWallRun(WallNormal))
	{
		return;
	}

	bHasTriggeredWallRunThisJump = true;
	bCanCheckWallRun = false;

	const bool bIsRightWall = FVector::DotProduct(GetActorRightVector(), -WallNormal) >= 0.0f;
	const FString DebugMessage = FString::Printf(TEXT("Wall Run Triggered: %s"), bIsRightWall ? TEXT("Right") : TEXT("Left"));

	UE_LOG(LogUEGameJam, Log, TEXT("%s"), *DebugMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, DebugMessage);
	}
}

bool AGsPlayer::TryFindWallRunSurface(FHitResult& OutWallHit, FVector& OutWallNormal) const
{
	OutWallHit = FHitResult();
	OutWallNormal = FVector::ZeroVector;

	const UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();
	UWorld* World = GetWorld();
	if (!PlayerCapsuleComponent || !World || WallRunSideTraceDistance <= 0.0f)
	{
		return false;
	}

	const FVector TraceStart = GetActorLocation();
	const FVector RightDirection = GetActorRightVector().GetSafeNormal2D();
	if (RightDirection.IsNearlyZero())
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallRunSideTrace), false, this);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FHitResult BestHit;
	FVector BestNormal = FVector::ZeroVector;
	float BestDistance = TNumericLimits<float>::Max();

	const auto TryTraceSide = [&](const FVector& TraceDirection)
	{
		FHitResult SideHit;
		const FVector TraceEnd = TraceStart + (TraceDirection * WallRunSideTraceDistance);
		if (!World->LineTraceSingleByObjectType(SideHit, TraceStart, TraceEnd, ObjectQueryParams, QueryParams))
		{
			return;
		}

		FVector WallNormal = SideHit.ImpactNormal.IsNearlyZero() ? SideHit.Normal : SideHit.ImpactNormal;
		WallNormal = FVector::VectorPlaneProject(WallNormal, FVector::UpVector);
		if (!WallNormal.Normalize())
		{
			return;
		}

		if (SideHit.Distance < BestDistance)
		{
			BestHit = SideHit;
			BestNormal = WallNormal;
			BestDistance = SideHit.Distance;
		}
	};

	TryTraceSide(RightDirection);
	TryTraceSide(-RightDirection);

	if (BestDistance == TNumericLimits<float>::Max())
	{
		return false;
	}

	OutWallHit = BestHit;
	OutWallNormal = BestNormal;
	return true;
}

bool AGsPlayer::CanTriggerWallRun(const FVector& WallNormal) const
{
	if (!FirstPersonCameraComponent || CachedMoveInput.Y <= 0.1f)
	{
		return false;
	}

	FVector CameraForward = FirstPersonCameraComponent->GetForwardVector().GetSafeNormal2D();
	FVector HorizontalWallNormal = WallNormal.GetSafeNormal2D();
	if (CameraForward.IsNearlyZero() || HorizontalWallNormal.IsNearlyZero())
	{
		return false;
	}

	const float CameraWallDot = FMath::Abs(FVector::DotProduct(CameraForward, HorizontalWallNormal));
	if (CameraWallDot > WallRunMaxCameraWallNormalDot)
	{
		return false;
	}

	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	if (!HorizontalVelocity.Normalize())
	{
		return false;
	}

	const float ForwardCameraDot = FVector::DotProduct(HorizontalVelocity, CameraForward);
	return ForwardCameraDot >= WallRunMinForwardCameraDot;
}
