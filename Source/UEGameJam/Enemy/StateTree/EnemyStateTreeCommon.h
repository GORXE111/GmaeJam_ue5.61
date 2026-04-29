// ===================================================
// 文件：EnemyStateTreeCommon.h
// 说明：Enemy 模块通用 StateTree Tasks 与 Conditions。全部继承
//       FStateTreeTaskCommonBase / FStateTreeConditionCommonBase
//       保证跨 Schema 最大兼容。
//
//       包含：
//         Task:  FEST_SenseTargets / FEST_FaceTarget / FEST_Idle / FEST_Cooldown
//         Cond:  FEC_HasTarget / FEC_InRange / FEC_HasLineOfSight / FEC_IsDead
//
//       所有运行时数据放独立 USTRUCT InstanceData；父级型 Task
//       （SenseTargets / FaceTarget）构造里关闭
//       bShouldStateChangeOnReselect，防止子状态切换时反复
//       EnterState/ExitState 破坏委托绑定。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "EnemyStateTreeCommon.generated.h"

class AEnemyCharacter;
class AEnemyAIController;
class AAIController;
class ACharacter;
class AActor;

// ---------- FEST_SenseTargets ----------

USTRUCT()
struct UEGAMEJAM_API FEST_SenseTargetsInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	/** 只有带此 Tag 的 Actor 才会被视为潜在目标 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName SenseTag = FName(TEXT("Player"));

	/** 正视目标时的视锥半角（超过该角度则视为"部分感知" = 调查点而非目标） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float DirectLineOfSightCone = 85.0f;

	/** 当前是否持有确定目标 */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasTarget = false;

	/** 当前持有的目标 Actor */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor;

	/** 最近一次部分感知的位置（调查用） */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector InvestigateLocation = FVector::ZeroVector;

	/** 是否有调查点可去 */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasInvestigateLocation = false;

	UPROPERTY()
	float LastStimulusStrength = 0.0f;
};

/** 父级 Task：持续监听控制器感知事件，过滤 Tag + LOS 后设置目标/调查点。 */
USTRUCT(meta = (DisplayName = "Sense Targets", Category = "Enemy"))
struct UEGAMEJAM_API FEST_SenseTargets : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_SenseTargetsInstanceData;

	FEST_SenseTargets();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEST_FaceTarget ----------

USTRUCT()
struct UEGAMEJAM_API FEST_FaceTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> Controller;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;
};

/** 父级 Task：EnterState 设 AI Focus 到 Target，ExitState 清除。 */
USTRUCT(meta = (DisplayName = "Face Target", Category = "Enemy"))
struct UEGAMEJAM_API FEST_FaceTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_FaceTargetInstanceData;

	FEST_FaceTarget();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEST_Idle ----------

USTRUCT()
struct UEGAMEJAM_API FEST_IdleInstanceData
{
	GENERATED_BODY()
};

/** 占位 Task：什么都不做，永远 Running。配合 Transition 等外部事件跳出。 */
USTRUCT(meta = (DisplayName = "Idle", Category = "Enemy"))
struct UEGAMEJAM_API FEST_Idle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_IdleInstanceData;

	FEST_Idle();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEST_Cooldown ----------

USTRUCT()
struct UEGAMEJAM_API FEST_CooldownInstanceData
{
	GENERATED_BODY()

	/** 冷却时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0))
	float Seconds = 1.0f;

	UPROPERTY()
	float Elapsed = 0.0f;
};

/** 计时 Task：Tick 累计到 Seconds 返回 Succeeded。 */
USTRUCT(meta = (DisplayName = "Cooldown", Category = "Enemy"))
struct UEGAMEJAM_API FEST_Cooldown : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_CooldownInstanceData;

	FEST_Cooldown();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ==================== Conditions ====================

// ---------- FEC_HasTarget ----------

USTRUCT()
struct UEGAMEJAM_API FEC_HasTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller;

	/** 若 true，则在本 Condition 返回前对结果取反（等价于老版本的 Invert 按钮） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvertResult = false;
};

/** 条件：AI 控制器当前是否持有目标。 */
USTRUCT(meta = (DisplayName = "Has Target", Category = "Enemy"))
struct UEGAMEJAM_API FEC_HasTarget : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEC_HasTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------- FEC_InRange ----------

USTRUCT()
struct UEGAMEJAM_API FEC_InRangeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float MaxDistance = 300.0f;

	/** 若 true，则在本 Condition 返回前对结果取反 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvertResult = false;
};

/** 条件：Character 与 Target 的水平距离是否 <= MaxDistance。 */
USTRUCT(meta = (DisplayName = "In Range", Category = "Enemy"))
struct UEGAMEJAM_API FEC_InRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEC_InRangeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------- FEC_HasLineOfSight ----------

USTRUCT()
struct UEGAMEJAM_API FEC_HasLineOfSightInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 面向容差（超过此角度视为无 LOS） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ConeHalfAngle = 35.0f;

	/** 垂直多点检测次数，越高越宽容矮掩体 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 1))
	int32 NumVerticalChecks = 5;

	/** true: 条件要求有 LOS；false: 条件要求无 LOS */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bRequireLOS = true;
};

/** 条件：Character 能否直视 Target（多点垂直 LineTrace + 视锥判定）。 */
USTRUCT(meta = (DisplayName = "Has Line Of Sight", Category = "Enemy"))
struct UEGAMEJAM_API FEC_HasLineOfSight : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEC_HasLineOfSightInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------- FEC_IsDead ----------

USTRUCT()
struct UEGAMEJAM_API FEC_IsDeadInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	/** 若 true，则在本 Condition 返回前对结果取反（可用作"未死亡"） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvertResult = false;
};

/** 条件：敌人是否已死亡。 */
USTRUCT(meta = (DisplayName = "Is Dead", Category = "Enemy"))
struct UEGAMEJAM_API FEC_IsDead : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEC_IsDeadInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------- FEC_IsCoreExposed ----------

USTRUCT()
struct UEGAMEJAM_API FEC_IsCoreExposedInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	/** 若 true，则在本 Condition 返回前对结果取反（可用作"核心未暴露"） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvertResult = false;
};

/** 条件：敌人核心是否暴露（重装敌人在里世界圈内时为 true）。 */
USTRUCT(meta = (DisplayName = "Is Core Exposed", Category = "Enemy"))
struct UEGAMEJAM_API FEC_IsCoreExposed : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEC_IsCoreExposedInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
