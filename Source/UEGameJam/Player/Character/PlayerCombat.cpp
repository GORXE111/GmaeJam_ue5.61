// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/GsPlayer.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Enemy/EnemyCharacter.h"
#include "Enemy/EnemySubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Character/GsPlayerResourceDataAsset.h"
#include "Player/Scene/GsSkillAimAssistPoint.h"
#include "Player/Skill/GsSkillBall.h"
#include "TimerManager.h"

void AGsPlayer::DoSkill()
{
	BP_OnSkillInput();
	StartSkillCast();
}

bool AGsPlayer::StartMeleeAttack()
{
	if (bIsDead)
	{
		return false;
	}

	const FGsPlayerTuningRow& PlayerTuning = GetPlayerTuning();
	float ActionDuration = PlayerTuning.MeleeFallbackDuration;

	if (USkeletalMeshComponent* PlayerMesh = GetFirstPersonArmsMeshComponent())
	{
		if (UAnimInstance* AnimInstance = PlayerMesh->GetAnimInstance())
		{
			const float MontageDuration = AnimInstance->Montage_Play(PlayerResourceData->MeleeAttackMontage);
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

	if (PlayerTuning.MeleeHitDelay <= 0.0f)
	{
		PerformMeleeHit();
	}
	else
	{
		World->GetTimerManager().SetTimer(MeleeHitTimer, this, &AGsPlayer::PerformMeleeHit, PlayerTuning.MeleeHitDelay, false);
	}

	return true;
}

FVector AGsPlayer::GetSkillAimTarget(const FVector& ViewLocation, const FVector& ViewDirection) const
{
	const FGsPlayerTuningRow& PlayerTuning = GetPlayerTuning();
	const FVector TraceEnd = ViewLocation + (ViewDirection * PlayerTuning.SkillAimTraceDistance);

	UWorld* World = GetWorld();
	if (!World)
	{
		return TraceEnd;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerSkillAim), false, this);
	QueryParams.AddIgnoredActor(this);

	if (PlayerTuning.SkillEnemyAimAssistAngle > 0.0f)
	{
		TArray<AEnemyCharacter*> AliveEnemies;
		if (const UEnemySubsystem* EnemySubsystem = UEnemySubsystem::Get(this))
		{
			EnemySubsystem->GetAliveEnemies(AliveEnemies);
		}

		const float MaxDistanceSquared = FMath::Square(PlayerTuning.SkillAimTraceDistance);
		const float MinAimDot = FMath::Cos(FMath::DegreesToRadians(PlayerTuning.SkillEnemyAimAssistAngle));
		bool bHasBestAimAssistTarget = false;
		FVector BestAimAssistTargetLocation = FVector::ZeroVector;
		float BestAimDot = MinAimDot;
		float BestDistanceSquared = MaxDistanceSquared;

		const auto TryUseAimAssistTarget = [&](AActor* TargetActor, const FVector& TargetLocation)
		{
			if (!IsValid(TargetActor))
			{
				return;
			}

			const FVector ToTarget = TargetLocation - ViewLocation;
			const float DistanceSquared = ToTarget.SizeSquared();
			if (DistanceSquared <= KINDA_SMALL_NUMBER || DistanceSquared > MaxDistanceSquared)
			{
				return;
			}

			const FVector TargetDirection = ToTarget.GetSafeNormal();
			const float AimDot = FVector::DotProduct(ViewDirection, TargetDirection);
			if (AimDot < MinAimDot)
			{
				return;
			}

			FHitResult SightHit;
			FCollisionQueryParams SightQueryParams = QueryParams;
			SightQueryParams.AddIgnoredActor(TargetActor);
			const bool bHasBlocker = World->LineTraceSingleByChannel(SightHit, ViewLocation, TargetLocation, ECC_Visibility, SightQueryParams);
			if (bHasBlocker)
			{
				return;
			}

			if (!bHasBestAimAssistTarget || AimDot > BestAimDot || (FMath::IsNearlyEqual(AimDot, BestAimDot) && DistanceSquared < BestDistanceSquared))
			{
				bHasBestAimAssistTarget = true;
				BestAimAssistTargetLocation = TargetLocation;
				BestAimDot = AimDot;
				BestDistanceSquared = DistanceSquared;
			}
		};

		for (AEnemyCharacter* Enemy : AliveEnemies)
		{
			if (!IsValid(Enemy) || Enemy->IsDead())
			{
				continue;
			}

			TryUseAimAssistTarget(Enemy, Enemy->GetActorLocation());
		}

		for (TActorIterator<AGsSkillAimAssistPoint> AimAssistPointIt(World); AimAssistPointIt; ++AimAssistPointIt)
		{
			AGsSkillAimAssistPoint* AimAssistPoint = *AimAssistPointIt;
			if (!IsValid(AimAssistPoint) || !AimAssistPoint->IsSkillAimAssistEnabled())
			{
				continue;
			}

			TryUseAimAssistTarget(AimAssistPoint, AimAssistPoint->GetSkillAimTargetLocation());
		}

		if (bHasBestAimAssistTarget)
		{
			return BestAimAssistTargetLocation;
		}
	}

	FHitResult OutHit;
	World->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

bool AGsPlayer::StartSkillCast()
{
	if (bIsDead)
	{
		return false;
	}

	// 上一次技能（飞行小球或落地大球）尚未结束 → 禁止再丢
	if (AGsSkillBall::IsAnySkillActive())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const bool bShouldBypassSkillAction = IsSliding() || IsWallRunning();
	const bool bStartedSkillAction = !bShouldBypassSkillAction;
	if (bStartedSkillAction && !TryStartCharacterAction(EUEGameJamPlayerAction::Skill, GetPlayerTuning().SkillActionDuration))
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	
	const FVector ViewDirection = ViewRotation.Vector();
	const FVector AimTarget = GetSkillAimTarget(ViewLocation, ViewDirection);
	const FVector SpawnLocation = GetActorLocation();
	const FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, AimTarget);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGsSkillBall* SpawnedSkillBall = World->SpawnActor<AGsSkillBall>(PlayerResourceData->SkillProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!SpawnedSkillBall)
	{
		if (bStartedSkillAction)
		{
			FinishCharacterAction();
		}
		return false;
	}

	SpawnedSkillBall->InitializeSkillBall(AimTarget);

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

	const float MeleeDamage = GetPlayerTuning().MeleeDamage;
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("击中=======%s"), *HitActor->GetName()));
		}
		UGameplayStatics::ApplyDamage(HitActor, MeleeDamage, GetController(), this, UDamageType::StaticClass());
	}
}
