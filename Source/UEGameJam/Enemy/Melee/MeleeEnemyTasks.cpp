// ===================================================
// 文件：MeleeEnemyTasks.cpp
// 说明：近战敌人专属 StateTree Task 实现（MeleeDashTask / MeleeSwingTask）。
//       Task 声明在 Enemy/StateTree/EnemyStateTreeTasks.h。
// ===================================================

#include "EnemyStateTreeTasks.h"
#include "MeleeEnemy.h"
#include "StateTreeExecutionContext.h"
#include "Engine/Engine.h"

namespace
{
	static void PrintEnemyTaskDebug(const FString& Msg, const FColor Color)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color, Msg);
		}
		UE_LOG(LogTemp, Log, TEXT("[EnemyTask] %s"), *Msg);
	}
}

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
		PrintEnemyTaskDebug(TEXT("MeleeDash: FAIL (MeleeEnemy is null - Context binding missing?)"), FColor::Red);
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

	const FVector Impulse = Dir.GetSafeNormal() * Data.Impulse;
	Data.MeleeEnemy->LaunchCharacter(Impulse, true, false);

	PrintEnemyTaskDebug(FString::Printf(TEXT("MeleeDash: ENTER (impulse=%.0f)"), Data.Impulse), FColor::Orange);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyMeleeDashTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;
	if (Data.ElapsedTime >= Data.Duration)
	{
		PrintEnemyTaskDebug(TEXT("MeleeDash: DONE"), FColor::Orange);
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
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
		PrintEnemyTaskDebug(TEXT("MeleeSwing: FAIL (MeleeEnemy is null)"), FColor::Red);
		return EStateTreeRunStatus::Failed;
	}

	Data.MeleeEnemy->SetMeleeHitboxActive(true);
	Data.bHitboxActive = true;
	PrintEnemyTaskDebug(TEXT("MeleeSwing: ENTER (hitbox ON)"), FColor::Yellow);
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

	if (Data.bHitboxActive && Data.ElapsedTime >= Data.HitboxActiveWindow)
	{
		Data.MeleeEnemy->SetMeleeHitboxActive(false);
		Data.bHitboxActive = false;
		PrintEnemyTaskDebug(TEXT("MeleeSwing: hitbox OFF"), FColor::Yellow);
	}

	if (Data.ElapsedTime >= Data.TotalDuration)
	{
		PrintEnemyTaskDebug(TEXT("MeleeSwing: DONE"), FColor::Yellow);
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
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
