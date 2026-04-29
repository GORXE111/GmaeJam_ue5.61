// ===================================================
// 文件：EnemyRangedTasks.h
// 说明：远程敌人（手枪、机枪）专用 StateTree Tasks。
//
//       任务 6 交付：FEST_TelegraphAim、FEST_SpawnProjectile
//       任务 7 交付：FEST_BurstFire（连发）
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "EnemyRangedTasks.generated.h"

class AEnemyCharacter;
class AEnemyProjectile;
class AActor;
class USkeletalMeshComponent;

// ---------- FEST_TelegraphAim ----------

USTRUCT()
struct UEGAMEJAM_API FEST_TelegraphAimInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 锁定时长（秒）。手枪 0.8-1.2s，机枪 1.0-1.5s（策划参数区分） */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float LockSeconds = 1.0f;

	UPROPERTY()
	float Elapsed = 0.0f;
};

/**
 *  瞄准（锁定）阶段。进入时广播威胁开始，Tick 计时到 LockSeconds 返回 Succeeded。
 *  离开状态（包括被打断）时广播威胁结束，确保 UI 指示器同步消失。
 */
USTRUCT(meta = (DisplayName = "Telegraph Aim", Category = "Enemy"))
struct UEGAMEJAM_API FEST_TelegraphAim : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_TelegraphAimInstanceData;

	FEST_TelegraphAim();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEST_SpawnProjectile ----------

USTRUCT()
struct UEGAMEJAM_API FEST_SpawnProjectileInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 要生成的投射物类 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	/** 使用敌人骨骼上的枪口插槽位置。留空则用胶囊前方 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName MuzzleSocket = NAME_None;

	/** 胶囊前方的水平偏移（厘米），仅在 MuzzleSocket 为空时使用 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ForwardOffset = 60.0f;

	/** 胶囊垂直偏移（厘米），仅在 MuzzleSocket 为空时使用 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float VerticalOffset = 40.0f;
};

/**
 *  开火瞬时。EnterState 生成一颗投射物并立即 Succeeded。
 */
USTRUCT(meta = (DisplayName = "Spawn Projectile", Category = "Enemy"))
struct UEGAMEJAM_API FEST_SpawnProjectile : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_SpawnProjectileInstanceData;

	FEST_SpawnProjectile();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------- FEST_BurstFire ----------

USTRUCT()
struct UEGAMEJAM_API FEST_BurstFireInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName MuzzleSocket = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ForwardOffset = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float VerticalOffset = 40.0f;

	/** 发射频率（发/秒）。策划：机枪 5-8 发/秒 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.01f))
	float FireRate = 7.0f;

	/** 连发总时长（秒）。策划：机枪 1.2-1.5s */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float Duration = 1.3f;

	/** 散射半角（度）。越大子弹越散 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, ClampMax = 45))
	float SpreadHalfAngle = 3.0f;

	UPROPERTY()
	float Elapsed = 0.0f;

	UPROPERTY()
	float TimeSinceLastShot = 1.0f;

	UPROPERTY()
	int32 ShotsFired = 0;
};

/**
 *  连发 Task。Tick 内按 FireRate 发射扇形散射投射物，持续 Duration 秒。
 *  适合机枪兵的 BurstFire 阶段。
 */
USTRUCT(meta = (DisplayName = "Burst Fire", Category = "Enemy"))
struct UEGAMEJAM_API FEST_BurstFire : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEST_BurstFireInstanceData;

	FEST_BurstFire();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
