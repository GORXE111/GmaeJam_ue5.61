// ===================================================
// 文件：EnemyMeleeTasks.cpp
// 说明：近战敌人 StateTree Tasks 实现。
// ===================================================

#include "EnemyMeleeTasks.h"
#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "EnemyDebug.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace EnemyMeleeUtil
{
	static FVector PickRandomNavPoint(const AActor* Origin, float Radius)
	{
		if (!Origin)
		{
			return FVector::ZeroVector;
		}
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Origin->GetWorld());
		if (!NavSys)
		{
			return Origin->GetActorLocation();
		}
		FNavLocation OutLoc;
		if (NavSys->GetRandomReachablePointInRadius(Origin->GetActorLocation(), Radius, OutLoc))
		{
			return OutLoc.Location;
		}
		return Origin->GetActorLocation();
	}
}

// ---------- FEMT_PatrolOrStand ----------

FEMT_PatrolOrStand::FEMT_PatrolOrStand()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEMT_PatrolOrStand::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.WaitElapsed = 0.0f;
	Data.bMoveInProgress = false;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEMT_PatrolOrStand::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.bStandStill || !IsValid(Data.Character) || !IsValid(Data.Controller))
	{
		return EStateTreeRunStatus::Running;
	}

	if (Data.bMoveInProgress)
	{
		// 粗略判断：若速度很低则视为"已到达"
		const float Speed = Data.Character->GetVelocity().Size2D();
		if (Speed < 10.0f)
		{
			Data.bMoveInProgress = false;
			Data.WaitElapsed = 0.0f;
		}
	}
	else
	{
		Data.WaitElapsed += DeltaTime;
		if (Data.WaitElapsed >= Data.WaitSeconds)
		{
			const FVector Goal = EnemyMeleeUtil::PickRandomNavPoint(Data.Character, Data.PatrolRadius);
			Data.Controller->MoveToLocation(Goal, -1.0f, true, true, false, true);
			Data.bMoveInProgress = true;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FEMT_PatrolOrStand::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Controller))
	{
		if (UPathFollowingComponent* Path = Data.Controller->GetPathFollowingComponent())
		{
			Path->AbortMove(*Data.Controller, FPathFollowingResultFlags::UserAbort);
		}
	}
}

// ---------- FEMT_NavChase ----------

FEMT_NavChase::FEMT_NavChase()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEMT_NavChase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.SinceLastRepath = 10.0f;

	if (!IsValid(Data.Controller) || !IsValid(Data.Target))
	{
		return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEMT_NavChase::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!IsValid(Data.Controller) || !IsValid(Data.Character) || !IsValid(Data.Target))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 进入攻击距离 → 成功
	const float DistSq = FVector::DistSquared(Data.Character->GetActorLocation(), Data.Target->GetActorLocation());
	if (DistSq <= (Data.AcceptanceRadius * Data.AcceptanceRadius))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Data.SinceLastRepath += DeltaTime;
	if (Data.SinceLastRepath >= Data.RepathInterval)
	{
		Data.SinceLastRepath = 0.0f;
		Data.Controller->MoveToActor(Data.Target, Data.AcceptanceRadius * 0.8f, true, true, true);
	}

	return EStateTreeRunStatus::Running;
}

void FEMT_NavChase::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Controller))
	{
		if (UPathFollowingComponent* Path = Data.Controller->GetPathFollowingComponent())
		{
			Path->AbortMove(*Data.Controller, FPathFollowingResultFlags::UserAbort);
		}
	}
}

// ---------- FEMT_LockOnForStrike ----------

FEMT_LockOnForStrike::FEMT_LockOnForStrike()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEMT_LockOnForStrike::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;

	if (IsValid(Data.Controller) && IsValid(Data.Target))
	{
		Data.Controller->SetFocus(Data.Target);
	}
	if (IsValid(Data.Character))
	{
		if (UCharacterMovementComponent* Move = Data.Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEMT_LockOnForStrike::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	return (Data.Elapsed >= Data.WindupSeconds) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEMT_LockOnForStrike::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Controller))
	{
		Data.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

// ---------- FEMT_DashStrike ----------

FEMT_DashStrike::FEMT_DashStrike()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEMT_DashStrike::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;
	Data.bDealtDamage = false;

	if (!IsValid(Data.Character))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector Direction = Data.Character->GetActorForwardVector();
	if (IsValid(Data.Target))
	{
		Direction = (Data.Target->GetActorLocation() - Data.Character->GetActorLocation()).GetSafeNormal();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	Data.Character->LaunchCharacter(Direction * Data.DashImpulse, true, true);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEMT_DashStrike::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;

	if (!IsValid(Data.Character))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 若还没造成伤害，尝试前向 Sphere Sweep 检测 Target
	if (!Data.bDealtDamage && IsValid(Data.Target))
	{
		const FVector Start = Data.Character->GetActorLocation();
		const FVector End = Start + Data.Character->GetActorForwardVector() * (Data.HitSphereRadius * 0.5f);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Data.Character);

		FHitResult Hit;
		const bool bHit = Data.Character->GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(Data.HitSphereRadius),
			Params);

		if (bHit && Hit.GetActor() == Data.Target)
		{
			TSubclassOf<UDamageType> DTClass = Data.DamageTypeClass;
			if (!DTClass)
			{
				DTClass = UDamageType::StaticClass();
			}
			UGameplayStatics::ApplyDamage(Data.Target, Data.Damage, Data.Character->GetController(), Data.Character, DTClass);
			Data.bDealtDamage = true;

			UE_LOG(LogEnemyModule, Verbose, TEXT("DashStrike %s hit %s for %.1f"),
				*Data.Character->GetName(), *Data.Target->GetName(), Data.Damage);
		}
	}

	return (Data.Elapsed >= Data.MaxDashTime) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
