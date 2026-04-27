// ===================================================
// 文件：MeleeEnemyTasks.cpp
// 说明：近战敌人专属 StateTree Task 实现（MeleeDashTask / MeleeSwingTask）。
//       Task 声明在 Enemy/StateTree/EnemyStateTreeTasks.h。
// ===================================================

#include "EnemyStateTreeTasks.h"
#include "MeleeEnemy.h"
#include "StateTreeExecutionContext.h"

////////////////////////////////////////////////////////////////////
// MeleeDash

FEnemyMeleeDashTask::FEnemyMeleeDashTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyMeleeDashTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;

	if (!IsValid(Data.MeleeEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector Dir = FVector::ZeroVector;
	if (IsValid(Data.Target))
	{
		Dir = Data.Target->GetActorLocation() - Data.MeleeEnemy->GetActorLocation();
		Dir.Z = 0.f;
	}
	if (Dir.IsNearlyZero())
	{
		Dir = Data.MeleeEnemy->GetActorForwardVector();
	}

	// 使用传入的冲量参数覆盖 DataAsset 的默认值（Task 节点可细化每个实例的突进）
	const FVector Impulse = Dir.GetSafeNormal() * Data.Impulse;
	Data.MeleeEnemy->LaunchCharacter(Impulse, true, false);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMeleeDashTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;
	return (Data.ElapsedTime >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FEnemyMeleeDashTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Melee Dash</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// MeleeSwing

FEnemyMeleeSwingTask::FEnemyMeleeSwingTask()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEnemyMeleeSwingTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	Data.bHitboxActive = false;

	if (!IsValid(Data.MeleeEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.MeleeEnemy->SetMeleeHitboxActive(true);
	Data.bHitboxActive = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMeleeSwingTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.MeleeEnemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedTime += DeltaTime;

	// Hitbox 激活窗口结束后关闭
	if (Data.bHitboxActive && Data.ElapsedTime >= Data.HitboxActiveWindow)
	{
		Data.MeleeEnemy->SetMeleeHitboxActive(false);
		Data.bHitboxActive = false;
	}

	return (Data.ElapsedTime >= Data.TotalDuration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEnemyMeleeSwingTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.MeleeEnemy))
	{
		Data.MeleeEnemy->SetMeleeHitboxActive(false);
	}
	Data.bHitboxActive = false;
}

#if WITH_EDITOR
FText FEnemyMeleeSwingTask::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Melee Swing</b>"));
}
#endif
