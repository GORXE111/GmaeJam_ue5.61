// ===================================================
// 文件：MachineGunEnemyTasks.cpp
// 说明：机枪敌人专属 Task 实现（MGWarmup / MGBurst）。
//       声明位于 Enemy/StateTree/EnemyStateTreeTasks.h。
// ===================================================

#include "EnemyStateTreeTasks.h"
#include "MachineGunEnemy.h"
#include "StateTreeExecutionContext.h"

////////////////////////////////////////////////////////////////////
// MGWarmup

FEnemyMGWarmupTask::FEnemyMGWarmupTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyMGWarmupTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;

	if (!IsValid(Data.MGEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.MGEnemy->SetWarningLasersActive(true, Data.Target);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMGWarmupTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;
	return (Data.ElapsedTime >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEnemyMGWarmupTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.MGEnemy))
	{
		Data.MGEnemy->SetWarningLasersActive(false, nullptr);
	}
}

#if WITH_EDITOR
FText FEnemyMGWarmupTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>MG Warmup</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// MGBurst

FEnemyMGBurstTask::FEnemyMGBurstTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyMGBurstTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	Data.ShotAccumulator = 0.f;

	if (!IsValid(Data.MGEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.MGEnemy->SetTrackingEnabled(true, Data.Target);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMGBurstTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.MGEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedTime += DeltaTime;
	Data.ShotAccumulator += DeltaTime * Data.FireRate;

	while (Data.ShotAccumulator >= 1.f)
	{
		Data.ShotAccumulator -= 1.f;

		FVector AimLoc = FVector::ZeroVector;
		if (IsValid(Data.Target))
		{
			AimLoc = Data.Target->GetActorLocation();
		}
		else
		{
			AimLoc = Data.MGEnemy->GetMuzzleLocation() + Data.MGEnemy->GetActorForwardVector() * 10000.f;
		}
		Data.MGEnemy->FireOneBullet(AimLoc);
	}

	return (Data.ElapsedTime >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEnemyMGBurstTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.MGEnemy))
	{
		Data.MGEnemy->SetTrackingEnabled(false, nullptr);
	}
}

#if WITH_EDITOR
FText FEnemyMGBurstTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>MG Burst</b>"));
}
#endif
