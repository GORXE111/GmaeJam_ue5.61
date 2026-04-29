// ===================================================
// 文件：EnemyStateTreeConditions.cpp
// ===================================================

#include "EnemyStateTreeConditions.h"
#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "StateTreeExecutionContext.h"
#include "Engine/World.h"

////////////////////////////////////////////////////////////////////
// HasPlayerTarget

bool FEnemyHasPlayerTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Controller))
	{
		return false;
	}
	return IsValid(Data.Controller->GetCachedPlayer());
}

#if WITH_EDITOR
FText FEnemyHasPlayerTargetCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Has Player Target</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// PlayerInRadius

bool FEnemyPlayerInRadiusCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Target))
	{
		return false;
	}
	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Data.Target->GetActorLocation());
	return DistSq <= (Data.Radius * Data.Radius);
}

#if WITH_EDITOR
FText FEnemyPlayerInRadiusCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Player In Radius</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// PlayerInRange

bool FEnemyPlayerInRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Target))
	{
		return false;
	}
	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Data.Target->GetActorLocation());
	return DistSq >= (Data.MinRange * Data.MinRange) && DistSq <= (Data.MaxRange * Data.MaxRange);
}

#if WITH_EDITOR
FText FEnemyPlayerInRangeCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Player In Range</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// HasLineOfSight

bool FEnemyHasLineOfSightCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.Enemy) || !IsValid(Data.Target))
	{
		return false;
	}

	UWorld* World = Data.Enemy->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start = Data.Enemy->GetActorLocation() + Data.EyeOffset;
	const FVector End   = Data.Target->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Data.Enemy);
	Params.AddIgnoredActor(Data.Target);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked;
}

#if WITH_EDITOR
FText FEnemyHasLineOfSightCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Has Line of Sight</b>"));
}
#endif

////////////////////////////////////////////////////////////////////
// IsDead

bool FEnemyIsDeadCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return IsValid(Data.Enemy) && Data.Enemy->IsDead();
}

#if WITH_EDITOR
FText FEnemyIsDeadCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Is Dead</b>"));
}
#endif
