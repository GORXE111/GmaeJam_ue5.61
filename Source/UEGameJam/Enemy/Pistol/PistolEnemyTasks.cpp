// ===================================================
// 文件：PistolEnemyTasks.cpp
// 说明：手枪敌人专属 Task 实现（PistolAim / PistolFire）。
//       声明位于 Enemy/StateTree/EnemyStateTreeTasks.h。
// ===================================================

#include "EnemyStateTreeTasks.h"
#include "PistolEnemy.h"
#include "StateTreeExecutionContext.h"

////////////////////////////////////////////////////////////////////
// PistolAim

FEnemyPistolAimTask::FEnemyPistolAimTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyPistolAimTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	Data.bFlickerStarted = false;

	if (!IsValid(Data.PistolEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.PistolEnemy->SetLaserActive(true, Data.Target);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyPistolAimTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.PistolEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedTime += DeltaTime;

	if (!Data.bFlickerStarted && Data.Duration > 0.f)
	{
		const float Ratio = Data.ElapsedTime / Data.Duration;
		if (Ratio >= Data.FlickerStartRatio)
		{
			Data.PistolEnemy->SetLaserFlicker(true);
			Data.bFlickerStarted = true;
		}
	}

	return (Data.ElapsedTime >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEnemyPistolAimTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.PistolEnemy))
	{
		Data.PistolEnemy->SetLaserFlicker(false);
		Data.PistolEnemy->SetLaserActive(false, nullptr);
	}
}

#if WITH_EDITOR
FText FEnemyPistolAimTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Pistol Aim</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// PistolFire

EStateTreeRunStatus FEnemyPistolFireTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.PistolEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector AimLoc = FVector::ZeroVector;
	if (IsValid(Data.Target))
	{
		AimLoc = Data.Target->GetActorLocation();
	}
	else
	{
		AimLoc = Data.PistolEnemy->GetMuzzleLocation() + Data.PistolEnemy->GetActorForwardVector() * 10000.f;
	}

	Data.PistolEnemy->FireProjectile(AimLoc);
	return EStateTreeRunStatus::Succeeded; // 单帧任务
}

#if WITH_EDITOR
FText FEnemyPistolFireTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Pistol Fire</b>"));
}
#endif
