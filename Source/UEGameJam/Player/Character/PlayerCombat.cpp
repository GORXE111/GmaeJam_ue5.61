// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
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

	UBoxComponent* AttackDamageCollision = GetMeleeDamageCollision();
	if (!AttackDamageCollision)
	{
		return;
	}

	AttackDamageCollision->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	AttackDamageCollision->GetOverlappingActors(OverlappingActors);
	if (OverlappingActors.IsEmpty())
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (AActor* HitActor : OverlappingActors)
	{
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
