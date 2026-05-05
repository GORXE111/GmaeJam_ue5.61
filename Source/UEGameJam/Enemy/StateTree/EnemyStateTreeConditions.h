// ===================================================
// 文件：EnemyStateTreeConditions.h
// 说明：敌人 StateTree 条件集合。
//   - HasPlayerTarget：AIController 有缓存玩家目标
//   - PlayerInRadius  ：目标在指定距离内
//   - PlayerInRange   ：目标距离在 [Min,Max] 之间
//   - HasLineOfSight  ：LineTrace 视线检查
//   - IsDead          ：敌人已死亡
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "EnemyStateTreeConditions.generated.h"

class AEnemyCharacter;
class AEnemyAIController;

////////////////////////////////////////////////////////////////////
// HasPlayerTarget

USTRUCT()
struct FEnemyHasPlayerTargetConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyAIController> Controller;

	/** 勾选后对判定结果取反（UE 5.6 StateTree 编辑器无原生 Invert 选项，由本参数代替） */
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="Enemy: Has Player Target", Category="Enemy"))
struct UEGAMEJAM_API FEnemyHasPlayerTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyHasPlayerTargetConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// PlayerInRadius

USTRUCT()
struct FEnemyPlayerInRadiusConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Radius = 1500.f;

	/** 勾选后对判定结果取反 */
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="Enemy: Player In Radius", Category="Enemy"))
struct UEGAMEJAM_API FEnemyPlayerInRadiusCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyPlayerInRadiusConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// PlayerInRange

USTRUCT()
struct FEnemyPlayerInRangeConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float MaxRange = 1500.f;

	/** 勾选后对判定结果取反 */
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="Enemy: Player In Range", Category="Enemy"))
struct UEGAMEJAM_API FEnemyPlayerInRangeCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyPlayerInRangeConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// HasLineOfSight

USTRUCT()
struct FEnemyHasLineOfSightConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	/** 眼部偏移（局部坐标，一般 Z 取 60-80） */
	UPROPERTY(EditAnywhere, Category="Parameter")
	FVector EyeOffset = FVector(0.f, 0.f, 60.f);

	/** 勾选后对判定结果取反 */
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="Enemy: Has Line of Sight", Category="Enemy"))
struct UEGAMEJAM_API FEnemyHasLineOfSightCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyHasLineOfSightConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// IsDead

USTRUCT()
struct FEnemyIsDeadConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	/** 勾选后对判定结果取反（等效"Is Alive"） */
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="Enemy: Is Dead", Category="Enemy"))
struct UEGAMEJAM_API FEnemyIsDeadCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyIsDeadConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
