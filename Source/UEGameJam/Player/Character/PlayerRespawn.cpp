// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/Game/GsLevelStateGameState.h"
#include "TimerManager.h"

void AGsPlayer::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bResetFirstPersonCameraLocationOnNextUpdate = true;

	StopSlide(true);
	if (IsDashing())
	{
		AbortDash();
	}
	AbortGrapple();
	if (IsLedgeClimbing())
	{
		AbortLedgeClimb();
	}
	if (IsWallRunning())
	{
		StopWallRun();
	}
	else
	{
		ClearDashState();
		FinishCharacterAction();
	}
	bHasDashedSinceLanded = false;
	ClearGrappleState();
	ResetWallRunDetection();

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
	OnDeath.Broadcast();
	BP_OnDeath();

	const float RespawnDelay = GetPlayerTuning().DeferredDestructionTime;
	if (RespawnDelay <= 0.0f)
	{
		RespawnFromCheckpoint();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RespawnTimer, this, &AGsPlayer::OnRespawnTimerElapsed, RespawnDelay, false);
	}
}

void AGsPlayer::OnRespawnTimerElapsed()
{
	RespawnFromCheckpoint();
}

void AGsPlayer::RespawnFromCheckpoint()
{
	FTransform RespawnTransform = bHasSafeLocation
		? FTransform(LastSafeRotation, LastSafeLocation)
		: GetActorTransform();

	if (UWorld* World = GetWorld())
	{
		if (AGsLevelStateGameState* LevelState = World->GetGameState<AGsLevelStateGameState>())
		{
			LevelState->EnsureFallbackRespawnTransform(RespawnTransform);
			LevelState->GetCurrentRespawnTransform(RespawnTransform);
		}
	}

	ResetForRespawn(RespawnTransform);
}

void AGsPlayer::ResetForRespawn(const FTransform& RespawnTransform)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimer);
		World->GetTimerManager().ClearTimer(MeleeHitTimer);
		World->GetTimerManager().ClearTimer(ActionTimer);
	}

	const FGsPlayerTuningRow& PlayerTuning = GetPlayerTuning();
	CurrentHP = PlayerTuning.MaxHP;
	bIsDead = false;
	CurrentAction = EUEGameJamPlayerAction::None;
	CachedMoveInput = FVector2D::ZeroVector;
	bIsSlideInputHeld = false;
	bIsWaitingToStopSlideWhenCanStand = false;
	CurrentSlideSpeed = 0.0f;
	SlideDirection = FVector::ForwardVector;
	bHasDashedSinceLanded = false;
	bIsFalculaLaunching = false;
	bIsRecoveringFromFall = false;
	bHasSafeLocation = true;
	LastSafeLocation = RespawnTransform.GetLocation();
	LastSafeRotation = RespawnTransform.GetRotation().Rotator();
	bResetFirstPersonCameraLocationOnNextUpdate = true;
	TargetWallRunCameraRoll = 0.0f;
	CurrentWallRunCameraRoll = 0.0f;
	SetWallRunCameraTiltTarget(0.0f);
	ClearDashState();
	ClearGrappleState();
	ClearLedgeClimbState();
	ResetWallRunDetection();

	if (UWorld* World = GetWorld())
	{
		const float CurrentWorldTime = World->GetTimeSeconds();
		LastDashTime = CurrentWorldTime - PlayerTuning.DashCooldown;
		LastFalculaTime = CurrentWorldTime - PlayerTuning.GrappleCooldown;
		LastFallRecoveryTime = CurrentWorldTime - PlayerTuning.SafeLandingMinInterval;
	}
	else
	{
		LastDashTime = -PlayerTuning.DashCooldown;
		LastFalculaTime = -PlayerTuning.GrappleCooldown;
		LastFallRecoveryTime = -PlayerTuning.SafeLandingMinInterval;
	}

	const FRotator RespawnRotation = RespawnTransform.GetRotation().Rotator();
	SetActorLocationAndRotation(RespawnTransform.GetLocation(), RespawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* PlayerMovementComponent = GetCharacterMovement())
	{
		PlayerMovementComponent->StopMovementImmediately();
		PlayerMovementComponent->StopActiveMovement();
		PlayerMovementComponent->Velocity = FVector::ZeroVector;
		PlayerMovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (AController* PlayerController = GetController())
	{
		PlayerController->SetControlRotation(RespawnRotation);
	}

	EnableInput(Cast<APlayerController>(GetController()));
	OnDamaged.Broadcast(GetLifePercent());
	OnRespawn.Broadcast();
}
