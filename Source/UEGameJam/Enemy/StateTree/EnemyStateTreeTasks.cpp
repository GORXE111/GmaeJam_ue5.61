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
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"

namespace
{
	static void PrintEnemyCommonDebug(const FString& Msg, const FColor Color)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color, Msg);
		}
		UE_LOG(LogTemp, Log, TEXT("[EnemyTask] %s"), *Msg);
	}

	static const TCHAR* MoveRequestResultToString(const EPathFollowingRequestResult::Type Result)
	{
		switch (Result)
		{
		case EPathFollowingRequestResult::Failed:
			return TEXT("Failed");
		case EPathFollowingRequestResult::AlreadyAtGoal:
			return TEXT("AlreadyAtGoal");
		case EPathFollowingRequestResult::RequestSuccessful:
			return TEXT("RequestSuccessful");
		default:
			return TEXT("Unknown");
		}
	}
}

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

	const bool bWasFound = Data.bFound;

	AActor* Player = Data.Controller->FindPlayerByTag();
	if (!Player)
	{
		Data.bFound = false;
		Data.TargetActor = nullptr;
		Data.Controller->ClearCachedPlayer();
		if (bWasFound)
		{
			PrintEnemyCommonDebug(TEXT("AcquireTarget: LOST (no player pawn)"), FColor::Silver);
		}
		return EStateTreeRunStatus::Running;
	}

	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Player->GetActorLocation());
	if (DistSq > Data.DetectionRadius * Data.DetectionRadius)
	{
		Data.bFound = false;
		Data.TargetActor = nullptr;
		Data.Controller->ClearCachedPlayer();
		if (bWasFound)
		{
			PrintEnemyCommonDebug(TEXT("AcquireTarget: LOST (out of radius)"), FColor::Silver);
		}
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
			Data.Controller->ClearCachedPlayer();
			return EStateTreeRunStatus::Running;
		}
	}

	Data.TargetActor = Player;
	Data.bFound = true;
	if (!bWasFound)
	{
		PrintEnemyCommonDebug(FString::Printf(TEXT("AcquireTarget: FOUND dist=%.0f"), FMath::Sqrt(DistSq)), FColor::Green);
	}
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
	Data.TimeSinceRepath = Data.RepathInterval;
	Data.TimeSinceDebug = 999.f;

	if (!IsValid(Data.Controller) || !IsValid(Data.Target))
	{
		PrintEnemyCommonDebug(
			FString::Printf(TEXT("MoveToTarget: FAIL (Ctrl=%s, Target=%s)"),
				IsValid(Data.Controller) ? TEXT("OK") : TEXT("NULL"),
				IsValid(Data.Target) ? TEXT("OK") : TEXT("NULL")),
			FColor::Red);
		return EStateTreeRunStatus::Failed;
	}
	PrintEnemyCommonDebug(TEXT("MoveToTarget: ENTER"), FColor::Blue);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMoveToTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.TimeSinceDebug += DeltaTime;

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
		PrintEnemyCommonDebug(FString::Printf(TEXT("MoveToTarget: ARRIVED (dist=%.0f)"), FMath::Sqrt(DistSq)), FColor::Blue);
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
		const FPathFollowingRequestResult MoveResult = Data.Controller->MoveTo(Req);
		if (MoveResult.Code == EPathFollowingRequestResult::Failed)
		{
			if (Data.TimeSinceDebug >= 1.f)
			{
				Data.TimeSinceDebug = 0.f;
				PrintEnemyCommonDebug(
					FString::Printf(TEXT("MoveToTarget: MOVE REQUEST FAILED (dist=%.0f, AcceptRadius=%.0f)"),
						FMath::Sqrt(DistSq),
						Data.AcceptRadius),
					FColor::Red);
			}
			return EStateTreeRunStatus::Running;
		}
		if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			Data.bArrived = true;
			PrintEnemyCommonDebug(TEXT("MoveToTarget: ARRIVED (MoveTo already at goal)"), FColor::Blue);
			return EStateTreeRunStatus::Succeeded;
		}
		if (Data.TimeSinceDebug >= 1.f)
		{
			Data.TimeSinceDebug = 0.f;
			PrintEnemyCommonDebug(
				FString::Printf(TEXT("MoveToTarget: MoveTo %s (dist=%.0f)"),
					MoveRequestResultToString(MoveResult.Code),
					FMath::Sqrt(DistSq)),
				FColor::Blue);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyMoveToTargetTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	PrintEnemyCommonDebug("Exit Move To Target", FColor::Red);
	
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
		PrintEnemyCommonDebug(TEXT("FacePlayer: FAIL (Enemy is null)"), FColor::Red);
		return EStateTreeRunStatus::Failed;
	}
	PrintEnemyCommonDebug(TEXT("FacePlayer: ENTER"), FColor::Purple);
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

	const UEnum* E = StaticEnum<EEnemyAttackPhase>();
	const FString PhaseName = E ? E->GetNameStringByValue(static_cast<int64>(Data.Phase)) : TEXT("?");
	PrintEnemyCommonDebug(FString::Printf(TEXT("Phase: %s (%.2fs)"), *PhaseName, Data.Duration), FColor::Cyan);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyWaitPhaseTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;
	if (Data.ElapsedTime >= Data.Duration)
	{
		PrintEnemyCommonDebug(TEXT("WaitPhase: DONE"), FColor::Cyan);
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
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
		PrintEnemyCommonDebug(TEXT("SetMovementSpeed: FAIL (Enemy is null)"), FColor::Red);
		return EStateTreeRunStatus::Failed;
	}
	if (UCharacterMovementComponent* Move = Data.Enemy->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Data.Speed;
	}
	PrintEnemyCommonDebug(FString::Printf(TEXT("SetMovementSpeed: %.0f"), Data.Speed), FColor::Silver);
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemySetMovementSpeedTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Set Movement Speed</b>"));
}
#endif
