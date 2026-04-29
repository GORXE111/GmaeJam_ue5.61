// ===================================================
// 文件：EnemyStateTreeTasks.h
// 说明：敌人 StateTree 通用 + 各类专属任务集合。
// 通用：
//   - AcquireTarget    ：扫描带 Tag 的玩家，缓存到 AIController
//   - MoveToTarget     ：包装 MoveToActor，持续跟踪移动中的玩家
//   - Patrol           ：在半径内随机找点 MoveTo
//   - FacePlayer       ：每帧 RInterp 旋转朝向玩家
//   - WaitPhase        ：等待若干秒并设置 CurrentPhase（仅用于调试/动画）
//   - SetMovementSpeed ：切换 MaxWalkSpeed
// 近战专属：MeleeDash / MeleeSwing
// 手枪专属：PistolAim / PistolFire
// 机枪专属：MGWarmup / MGBurst
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "EnemyStateTreeTypes.h"
#include "EnemyStateTreeTasks.generated.h"

class AEnemyCharacter;
class AEnemyAIController;
class AMeleeEnemy;
class APistolEnemy;
class AMachineGunEnemy;

////////////////////////////////////////////////////////////////////
// AcquireTarget

USTRUCT()
struct FEnemyAcquireTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float DetectionRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bRequireLineOfSight = false;

	UPROPERTY(EditAnywhere, Category="Output")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category="Output")
	bool bFound = false;
};

USTRUCT(meta=(DisplayName="Enemy: Acquire Target", Category="Enemy"))
struct UEGAMEJAM_API FEnemyAcquireTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyAcquireTargetTask();

	using FInstanceDataType = FEnemyAcquireTargetTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// MoveToTarget

USTRUCT()
struct FEnemyMoveToTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float AcceptRadius = 150.f;

	/** 每隔多久重发一次 MoveTo 请求（保证跟踪移动目标） */
	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0.05))
	float RepathInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category="Output")
	bool bArrived = false;

	UPROPERTY()
	float TimeSinceRepath = 0.f;

	UPROPERTY()
	float TimeSinceDebug = 0.f;
};

USTRUCT(meta=(DisplayName="Enemy: Move To Target", Category="Enemy"))
struct UEGAMEJAM_API FEnemyMoveToTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyMoveToTargetTask();

	using FInstanceDataType = FEnemyMoveToTargetTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// Patrol

USTRUCT()
struct FEnemyPatrolTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyAIController> Controller;

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float PatrolRadius = 600.f;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float IdleBetweenPoints = 1.5f;

	UPROPERTY()
	FVector Origin = FVector::ZeroVector;

	UPROPERTY()
	FVector CurrentTarget = FVector::ZeroVector;

	UPROPERTY()
	float IdleTimer = 0.f;

	UPROPERTY()
	bool bMoving = false;
};

USTRUCT(meta=(DisplayName="Enemy: Patrol", Category="Enemy"))
struct UEGAMEJAM_API FEnemyPatrolTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyPatrolTask();

	using FInstanceDataType = FEnemyPatrolTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// FacePlayer

USTRUCT()
struct FEnemyFacePlayerTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	/** 插值旋转速率（度/秒） */
	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float YawRateDeg = 540.f;
};

USTRUCT(meta=(DisplayName="Enemy: Face Player", Category="Enemy"))
struct UEGAMEJAM_API FEnemyFacePlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyFacePlayerTask();

	using FInstanceDataType = FEnemyFacePlayerTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// WaitPhase

USTRUCT()
struct FEnemyWaitPhaseTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Duration = 1.f;

	UPROPERTY(EditAnywhere, Category="Parameter")
	EEnemyAttackPhase Phase = EEnemyAttackPhase::Idle;

	UPROPERTY()
	float ElapsedTime = 0.f;
};

USTRUCT(meta=(DisplayName="Enemy: Wait Phase", Category="Enemy"))
struct UEGAMEJAM_API FEnemyWaitPhaseTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyWaitPhaseTask();

	using FInstanceDataType = FEnemyWaitPhaseTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// SetMovementSpeed

USTRUCT()
struct FEnemySetMovementSpeedTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AEnemyCharacter> Enemy;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Speed = 300.f;
};

USTRUCT(meta=(DisplayName="Enemy: Set Movement Speed", Category="Enemy"))
struct UEGAMEJAM_API FEnemySetMovementSpeedTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemySetMovementSpeedTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// MeleeDash

USTRUCT()
struct FEnemyMeleeDashTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AMeleeEnemy> MeleeEnemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Impulse = 900.f;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Duration = 0.35f;

	UPROPERTY()
	float ElapsedTime = 0.f;
};

USTRUCT(meta=(DisplayName="Enemy: Melee Dash", Category="Enemy|Melee"))
struct UEGAMEJAM_API FEnemyMeleeDashTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyMeleeDashTask();

	using FInstanceDataType = FEnemyMeleeDashTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// MeleeSwing

USTRUCT()
struct FEnemyMeleeSwingTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AMeleeEnemy> MeleeEnemy;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float HitboxActiveWindow = 0.2f;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float TotalDuration = 0.4f;

	UPROPERTY()
	float ElapsedTime = 0.f;

	UPROPERTY()
	bool bHitboxActive = false;
};

USTRUCT(meta=(DisplayName="Enemy: Melee Swing", Category="Enemy|Melee"))
struct UEGAMEJAM_API FEnemyMeleeSwingTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyMeleeSwingTask();

	using FInstanceDataType = FEnemyMeleeSwingTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// PistolAim

USTRUCT()
struct FEnemyPistolAimTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<APistolEnemy> PistolEnemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Duration = 1.f;

	/** 闪烁开始比例 (0-1)，例如 0.7 表示在 70% 时开始闪 */
	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0, ClampMax=1))
	float FlickerStartRatio = 0.7f;

	UPROPERTY()
	float ElapsedTime = 0.f;

	UPROPERTY()
	bool bFlickerStarted = false;
};

USTRUCT(meta=(DisplayName="Enemy: Pistol Aim", Category="Enemy|Pistol"))
struct UEGAMEJAM_API FEnemyPistolAimTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyPistolAimTask();

	using FInstanceDataType = FEnemyPistolAimTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// PistolFire

USTRUCT()
struct FEnemyPistolFireTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<APistolEnemy> PistolEnemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;
};

USTRUCT(meta=(DisplayName="Enemy: Pistol Fire", Category="Enemy|Pistol"))
struct UEGAMEJAM_API FEnemyPistolFireTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyPistolFireTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// MGWarmup

USTRUCT()
struct FEnemyMGWarmupTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AMachineGunEnemy> MGEnemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Duration = 1.25f;

	UPROPERTY()
	float ElapsedTime = 0.f;
};

USTRUCT(meta=(DisplayName="Enemy: MG Warmup", Category="Enemy|MachineGun"))
struct UEGAMEJAM_API FEnemyMGWarmupTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyMGWarmupTask();

	using FInstanceDataType = FEnemyMGWarmupTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// MGBurst

USTRUCT()
struct FEnemyMGBurstTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AMachineGunEnemy> MGEnemy;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=0))
	float Duration = 1.35f;

	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin=1))
	float FireRate = 6.f;

	UPROPERTY()
	float ElapsedTime = 0.f;

	UPROPERTY()
	float ShotAccumulator = 0.f;
};

USTRUCT(meta=(DisplayName="Enemy: MG Burst", Category="Enemy|MachineGun"))
struct UEGAMEJAM_API FEnemyMGBurstTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyMGBurstTask();

	using FInstanceDataType = FEnemyMGBurstTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
