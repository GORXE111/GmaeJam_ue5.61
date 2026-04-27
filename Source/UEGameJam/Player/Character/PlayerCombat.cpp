// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

bool AGsPlayer::StartMeleeAttack()
{
	if (bIsDead)
	{
		return false;
	}

	float ActionDuration = MeleeFallbackDuration;

	if (MeleeAttackMontage && FirstPersonMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonMesh->GetAnimInstance())
		{
			const float MontageDuration = AnimInstance->Montage_Play(MeleeAttackMontage);
			if (MontageDuration > 0.0f)
			{
				ActionDuration = MontageDuration;
			}
		}
	}

	if (!TryStartCharacterAction(EUEGameJamPlayerAction::MeleeAttack, ActionDuration))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	World->GetTimerManager().ClearTimer(MeleeHitTimer);

	if (MeleeHitDelay <= 0.0f)
	{
		PerformMeleeHit();
	}
	else
	{
		World->GetTimerManager().SetTimer(MeleeHitTimer, this, &AGsPlayer::PerformMeleeHit, MeleeHitDelay, false);
	}

	return true;
}

void AGsPlayer::PerformMeleeHit()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(MeleeHitTimer);

	if (MeleeDamage <= 0.0f)
	{
		return;
	}

	const UCameraComponent* AttackCamera = GetFirstPersonCameraComponent();
	const FVector ViewLocation = AttackCamera ? AttackCamera->GetComponentLocation() : GetActorLocation();
	const FRotator ViewRotation = AttackCamera ? AttackCamera->GetComponentRotation() : GetActorRotation();
	const FVector ForwardVector = ViewRotation.Vector();
	const FVector TraceStart = ViewLocation + (ForwardVector * MeleeTraceStartOffset);
	const FVector TraceEnd = TraceStart + (ForwardVector * MeleeTraceDistance);
	const FVector TraceHalfExtent(
		FMath::Max(0.0f, MeleeTraceHalfExtent.X),
		FMath::Max(0.0f, MeleeTraceHalfExtent.Y),
		FMath::Max(0.0f, MeleeTraceHalfExtent.Z));

	if (TraceHalfExtent.IsNearlyZero())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerMeleeSweep), false, this);
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	const bool bHasHit = World->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		ViewRotation.Quaternion(),
		ECC_Visibility,
		FCollisionShape::MakeBox(TraceHalfExtent),
		QueryParams);

	if (!bHasHit)
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValid(HitActor) || HitActor == this)
		{
			continue;
		}

		if (DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);
		UGameplayStatics::ApplyDamage(HitActor, MeleeDamage, GetController(), this, MeleeDamageType);
	}
}
