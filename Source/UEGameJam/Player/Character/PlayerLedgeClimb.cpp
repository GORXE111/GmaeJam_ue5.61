// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Scene/GsLedgeClimbBox.h"

void AGsPlayer::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	const AGsLedgeClimbBox* LedgeClimbBox = Cast<AGsLedgeClimbBox>(OtherActor);
	if (!LedgeClimbBox || !CanTriggerLedgeClimb())
	{
		return;
	}

	StartLedgeClimb(*LedgeClimbBox);
}

bool AGsPlayer::CanTriggerLedgeClimb() const
{
	const UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	return !bIsDead
		&& PlayerMovementComponent
		&& PlayerMovementComponent->IsFalling()
		&& CurrentAction == EUEGameJamPlayerAction::None
		&& CachedMoveInput.Y > 0.1f
		&& !bIsFalculaLaunching;
}

bool AGsPlayer::StartLedgeClimb(const AGsLedgeClimbBox& LedgeClimbBox)
{
	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	const UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent();
	if (!PlayerMovementComponent || !PlayerCapsuleComponent || !CanTriggerLedgeClimb())
	{
		return false;
	}

	LedgeClimbStartLocation = GetActorLocation();
	LedgeClimbTargetLocation = LedgeClimbStartLocation;
	LedgeClimbTargetLocation.Z = LedgeClimbBox.GetLedgeTopWorldZ()
		+ PlayerCapsuleComponent->GetScaledCapsuleHalfHeight()
		+ GetPlayerTuning().LedgeClimbFloorClearance;
	if (LedgeClimbTargetLocation.Z <= LedgeClimbStartLocation.Z + KINDA_SMALL_NUMBER)
	{
		ClearLedgeClimbState();
		return false;
	}

	if (!TryStartCharacterAction(EUEGameJamPlayerAction::LedgeClimb, -1.0f))
	{
		ClearLedgeClimbState();
		return false;
	}

	PreLedgeClimbMovementMode = PlayerMovementComponent->MovementMode;
	PreLedgeClimbCustomMovementMode = PlayerMovementComponent->CustomMovementMode;
	CurrentLedgeClimbElapsedTime = 0.0f;

	PlayerMovementComponent->StopMovementImmediately();
	PlayerMovementComponent->StopActiveMovement();
	PlayerMovementComponent->DisableMovement();

	if (GetPlayerTuning().LedgeClimbDuration <= KINDA_SMALL_NUMBER)
	{
		FHitResult SweepHit;
		SetActorLocation(LedgeClimbTargetLocation, true, &SweepHit, ETeleportType::None);
		if (SweepHit.bBlockingHit)
		{
			AbortLedgeClimb();
			return false;
		}

		FinishLedgeClimb();
	}

	return true;
}

void AGsPlayer::UpdateLedgeClimb(float DeltaSeconds)
{
	if (!IsLedgeClimbing())
	{
		return;
	}

	UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement();
	if (!PlayerMovementComponent)
	{
		AbortLedgeClimb();
		return;
	}

	CurrentLedgeClimbElapsedTime += DeltaSeconds;
	const float LedgeClimbDuration = GetPlayerTuning().LedgeClimbDuration;
	const float LedgeClimbAlpha = LedgeClimbDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentLedgeClimbElapsedTime / LedgeClimbDuration, 0.0f, 1.0f)
		: 1.0f;
	const bool bReachedDestination = LedgeClimbAlpha >= 1.0f;

	const FVector DesiredLocation = FMath::Lerp(LedgeClimbStartLocation, LedgeClimbTargetLocation, LedgeClimbAlpha);

	FHitResult SweepHit;
	SetActorLocation(DesiredLocation, true, &SweepHit, ETeleportType::None);

	if (SweepHit.bBlockingHit)
	{
		AbortLedgeClimb();
		return;
	}

	if (bReachedDestination)
	{
		FinishLedgeClimb();
	}
}

void AGsPlayer::FinishLedgeClimb()
{
	if (!IsLedgeClimbing())
	{
		ClearLedgeClimbState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(MOVE_Walking);
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->Velocity = FVector::ZeroVector;
	}

	ClearLedgeClimbState();
	FinishCharacterAction();
	UpdateSafeLandingTransform();
}

void AGsPlayer::AbortLedgeClimb()
{
	if (!IsLedgeClimbing())
	{
		ClearLedgeClimbState();
		return;
	}

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->SetMovementMode(PreLedgeClimbMovementMode, PreLedgeClimbCustomMovementMode);
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->Velocity = FVector::ZeroVector;
	}

	ClearLedgeClimbState();
	FinishCharacterAction();
}

void AGsPlayer::ClearLedgeClimbState()
{
	LedgeClimbStartLocation = FVector::ZeroVector;
	LedgeClimbTargetLocation = FVector::ZeroVector;
	CurrentLedgeClimbElapsedTime = 0.0f;
	PreLedgeClimbMovementMode = MOVE_Falling;
	PreLedgeClimbCustomMovementMode = 0;
}
