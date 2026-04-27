// ===================================================
// 文件：MeleeEnemyDataAsset.h
// 说明：近战敌人数值配置。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyDataAsset.h"
#include "MeleeEnemyDataAsset.generated.h"

UCLASS(BlueprintType)
class UEGAMEJAM_API UMeleeEnemyDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()

public:
	/** 进入此半径触发攻击序列（单位 cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0, Units="cm"))
	float AttackRadius = 180.f;

	/** 锁定修正（强制转身）时长 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float LockOnDuration = 0.25f;

	/** 突进冲量大小 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float DashImpulse = 900.f;

	/** 突进时长 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float DashDuration = 0.35f;

	/** 挥砍时 Hitbox 激活窗口 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float HitboxActiveWindow = 0.2f;

	/** 挥砍整体时长（含前后摇） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float SwingDuration = 0.4f;

	/** 攻击收招时长 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float AttackRecovery = 0.6f;

	/** 近战伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0))
	float MeleeDamage = 20.f;

	/** 是否启用巡逻 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Patrol")
	bool bPatrol = true;

	/** 巡逻半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Patrol", meta=(ClampMin=0, Units="cm"))
	float PatrolRadius = 600.f;

	/** 巡逻间隔（到点后等多久再去下一个） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Patrol", meta=(ClampMin=0))
	float PatrolIdleTime = 1.5f;
};
