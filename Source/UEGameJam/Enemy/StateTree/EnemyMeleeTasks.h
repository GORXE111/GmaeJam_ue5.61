// ===================================================
// 文件：EnemyMeleeTasks.h
// 说明：近战敌人专用 StateTree Tasks。
//
//       FEMT_PatrolOrStand  巡逻或站桩（被触发前的闲置行为）
//       FEMT_NavChase       发现目标后 NavMesh 追击
//       FEMT_LockOnForStrike 攻击前摇（锁定目标 + 禁移动）
//       FEMT_DashStrike     前冲斩（LaunchCharacter + 扫检测 + ApplyDamage）
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "EnemyMeleeTasks.generated.h"

class AEnemyCharacter;
class AEnemyAIController;
class AActor;
class UDamageType;

// ---------- FEMT_PatrolOrStand ----------

USTRUCT()
struct UEGAMEJAM_API FEMT_PatrolOrStandInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller;

	/** 站桩不巡逻（适合被触发前完全静止的守卫） */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bStandStill = false;

	/** 巡逻半径（厘米），仅 bStandStill = false 时有效 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius = 600.0f;

	/** 到点后停留时间（秒） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float WaitSeconds = 1.5f;

	UPROPERTY()
	float WaitElapsed = 0.0f;

	UPROPERTY()
	bool bMoveInProgress = false;
};

/** 漫游型 Task。永不 Succeed，靠外部 Transition 打断（发现目标）。 */
USTRUCT(meta = (DisplayName = "Patrol Or Stand", Category = "Enemy"))
struct UEGAMEJAM_API FEMT_PatrolOrStand : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEMT_PatrolOrStandInstanceData;

	FEMT_PatrolOrStand();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEMT_NavChase ----------

USTRUCT()
struct UEGAMEJAM_API FEMT_NavChaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 视为"进入攻击距离"的阈值（厘米） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float AcceptanceRadius = 220.0f;

	/** 追击重发 MoveToActor 的最小时间间隔，避免每帧重发 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float RepathInterval = 0.5f;

	UPROPERTY()
	float SinceLastRepath = 10.0f;
};

/** NavMesh 追击。进入 AcceptanceRadius 返回 Succeeded；Target 丢失返回 Failed。 */
USTRUCT(meta = (DisplayName = "Nav Chase", Category = "Enemy"))
struct UEGAMEJAM_API FEMT_NavChase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEMT_NavChaseInstanceData;

	FEMT_NavChase();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEMT_LockOnForStrike ----------

USTRUCT()
struct UEGAMEJAM_API FEMT_LockOnForStrikeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 攻击前摇时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float WindupSeconds = 0.35f;

	UPROPERTY()
	float Elapsed = 0.0f;
};

/** 攻击前摇。设 Focus 到 Target、禁移动、计时。 */
USTRUCT(meta = (DisplayName = "Lock On For Strike", Category = "Enemy"))
struct UEGAMEJAM_API FEMT_LockOnForStrike : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEMT_LockOnForStrikeInstanceData;

	FEMT_LockOnForStrike();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEMT_DashStrike ----------

USTRUCT()
struct UEGAMEJAM_API FEMT_DashStrikeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** LaunchCharacter 冲击大小（cm/s） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0))
	float DashImpulse = 2200.0f;

	/** Dash 最长持续时间（秒），超时强制结束 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float MaxDashTime = 0.6f;

	/** 命中时对目标造成的伤害 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0))
	float Damage = 30.0f;

	/** 前向扫检测的球半径（厘米），略大于武器触及范围 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float HitSphereRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY()
	float Elapsed = 0.0f;

	UPROPERTY()
	bool bDealtDamage = false;
};

/** 突进斩。LaunchCharacter 前冲，Tick 扫检测命中玩家后 ApplyDamage。 */
USTRUCT(meta = (DisplayName = "Dash Strike", Category = "Enemy"))
struct UEGAMEJAM_API FEMT_DashStrike : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEMT_DashStrikeInstanceData;

	FEMT_DashStrike();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
