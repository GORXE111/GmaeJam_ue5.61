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
	const bool bRaw = IsValid(Data.Controller) && IsValid(Data.Controller->GetCachedPlayer());
	return Data.bInvert ? !bRaw : bRaw;
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
		// Target 无效时：原始判定为 false（不在半径内）；Invert 后就是 true
		return Data.bInvert;
	}
	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Data.Target->GetActorLocation());
	const bool bRaw = DistSq <= (Data.Radius * Data.Radius);
	return Data.bInvert ? !bRaw : bRaw;
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
		return Data.bInvert;
	}
	const float DistSq = FVector::DistSquared(Data.Enemy->GetActorLocation(), Data.Target->GetActorLocation());
	const bool bRaw = DistSq >= (Data.MinRange * Data.MinRange) && DistSq <= (Data.MaxRange * Data.MaxRange);
	return Data.bInvert ? !bRaw : bRaw;
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
		return Data.bInvert;
	}

	UWorld* World = Data.Enemy->GetWorld();
	if (!World)
	{
		return Data.bInvert;
	}

	const FVector Start = Data.Enemy->GetActorLocation() + Data.EyeOffset;
	const FVector End   = Data.Target->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Data.Enemy);
	Params.AddIgnoredActor(Data.Target);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const bool bRaw = !bBlocked;
	return Data.bInvert ? !bRaw : bRaw;
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
	const bool bRaw = IsValid(Data.Enemy) && Data.Enemy->IsDead();
	return Data.bInvert ? !bRaw : bRaw;
}

#if WITH_EDITOR
FText FEnemyIsDeadCondition::GetDescription(const FGuid&, FStateTreeDataView, const IStateTreeBindingLookup&, EStateTreeNodeFormatting) const
{
	return FText::FromString(TEXT("<b>Is Dead</b>"));
}
#endif
