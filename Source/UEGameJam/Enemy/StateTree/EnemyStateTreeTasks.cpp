// ===================================================
// 文件：EnemyStateTreeTasks.cpp
// 说明：通用敌人 Task 实现。专属 Task (Melee/Pistol/MG) 的实现
//       位于对应子模块的 cpp 里。
// ===================================================

#include "EnemyStateTreeTasks.h"
#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"

////////////////////////////////////////////////////////////////////
// AcquireTarget

FEnemyAcquireTargetTask::FEnemyAcquireTargetTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyAcquireTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bFound = false;
	Data.TargetActor = nullptr;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyAcquireTargetTask::Tick(FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Controller) || !IsValid(Data.Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Player = Data.Controller->FindPlayerByTag();
	if (!Player)
	{
		Data.bFound = false;
		Data.TargetActor = nullptr;
		return EStateTreeRunStatus::Running;
	}

	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Player->GetActorLocation());
	if (DistSq > Data.DetectionRadius * Data.DetectionRadius)
	{
		Data.bFound = false;
		Data.TargetActor = nullptr;
		return EStateTreeRunStatus::Running;
	}

	if (Data.bRequireLineOfSight)
	{
		UWorld* World = Data.Enemy->GetWorld();
		const FVector Start = Data.Enemy->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector End   = Player->GetActorLocation();

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Data.Enemy);
		Params.AddIgnoredActor(Player);

		FHitResult Hit;
		if (World && World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			Data.bFound = false;
			Data.TargetActor = nullptr;
			return EStateTreeRunStatus::Running;
		}
	}

	Data.TargetActor = Player;
	Data.bFound = true;
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemyAcquireTargetTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Acquire Target</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// MoveToTarget

FEnemyMoveToTargetTask::FEnemyMoveToTargetTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyMoveToTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bArrived = false;
	Data.TimeSinceRepath = Data.RepathInterval; // 首帧立即请求一次

	if (!IsValid(Data.Controller) || !IsValid(Data.Target))
	{
		return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMoveToTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Controller) || !IsValid(Data.Target))
	{
		return EStateTreeRunStatus::Failed;
	}

	APawn* Pawn = Data.Controller->GetPawn();
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Data.Target->GetActorLocation());
	if (DistSq <= Data.AcceptRadius * Data.AcceptRadius)
	{
		Data.bArrived = true;
		if (UPathFollowingComponent* PathComp = Data.Controller->GetPathFollowingComponent())
		{
			PathComp->AbortMove(*Data.Controller, FPathFollowingResultFlags::UserAbort);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	Data.TimeSinceRepath += DeltaTime;
	if (Data.TimeSinceRepath >= Data.RepathInterval)
	{
		Data.TimeSinceRepath = 0.f;
		FAIMoveRequest Req(Data.Target);
		Req.SetAcceptanceRadius(Data.AcceptRadius);
		Req.SetUsePathfinding(true);
		Req.SetAllowPartialPath(true);
		Data.Controller->MoveTo(Req);
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyMoveToTargetTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Controller))
	{
		if (UPathFollowingComponent* PathComp = Data.Controller->GetPathFollowingComponent())
		{
			PathComp->AbortMove(*Data.Controller, FPathFollowingResultFlags::UserAbort);
		}
	}
}

#if WITH_EDITOR
FText FEnemyMoveToTargetTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Move To Target</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// Patrol

FEnemyPatrolTask::FEnemyPatrolTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyPatrolTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Controller))
	{
		return EStateTreeRunStatus::Failed;
	}
	Data.Origin = Data.Enemy->GetActorLocation();
	Data.bMoving = false;
	Data.IdleTimer = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyPatrolTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Controller))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!Data.bMoving)
	{
		Data.IdleTimer += DeltaTime;
		if (Data.IdleTimer < Data.IdleBetweenPoints)
		{
			return EStateTreeRunStatus::Running;
		}
		Data.IdleTimer = 0.f;

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Data.Enemy->GetWorld());
		if (!NavSys)
		{
			return EStateTreeRunStatus::Running;
		}

		FNavLocation Rand;
		if (NavSys->GetRandomReachablePointInRadius(Data.Origin, Data.PatrolRadius, Rand))
		{
			Data.CurrentTarget = Rand.Location;
			FAIMoveRequest Req(Data.CurrentTarget);
			Req.SetAcceptanceRadius(50.f);
			Req.SetUsePathfinding(true);
			Data.Controller->MoveTo(Req);
			Data.bMoving = true;
		}
	}
	else
	{
		const float DistSq = FVector::DistSquared2D(Data.Enemy->GetActorLocation(), Data.CurrentTarget);
		if (DistSq <= 60.f * 60.f)
		{
			Data.bMoving = false;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyPatrolTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Controller))
	{
		if (UPathFollowingComponent* PathComp = Data.Controller->GetPathFollowingComponent())
		{
			PathComp->AbortMove(*Data.Controller, FPathFollowingResultFlags::UserAbort);
		}
	}
}

#if WITH_EDITOR
FText FEnemyPatrolTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Patrol</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// FacePlayer

FEnemyFacePlayerTask::FEnemyFacePlayerTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyFacePlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyFacePlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Target))
	{
		return EStateTreeRunStatus::Running;
	}

	const FVector ToTarget = Data.Target->GetActorLocation() - Data.Enemy->GetActorLocation();
	if (ToTarget.IsNearlyZero())
	{
		return EStateTreeRunStatus::Running;
	}

	FRotator Cur = Data.Enemy->GetActorRotation();
	const FRotator Desired = ToTarget.Rotation();
	FRotator New = FMath::RInterpConstantTo(Cur, FRotator(0.f, Desired.Yaw, 0.f), DeltaTime, Data.YawRateDeg);
	New.Pitch = 0.f;
	New.Roll = 0.f;
	Data.Enemy->SetActorRotation(New);

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemyFacePlayerTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Face Player</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// WaitPhase

FEnemyWaitPhaseTask::FEnemyWaitPhaseTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyWaitPhaseTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyWaitPhaseTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;
	return (Data.ElapsedTime >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemyWaitPhaseTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Wait Phase</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// SetMovementSpeed

EStateTreeRunStatus FEnemySetMovementSpeedTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}
	if (UCharacterMovementComponent* Move = Data.Enemy->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Data.Speed;
	}
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemySetMovementSpeedTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Set Movement Speed</b>"));
}
#endif
