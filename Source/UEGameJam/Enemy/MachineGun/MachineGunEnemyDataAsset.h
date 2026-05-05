// ===================================================
// 文件：MachineGunEnemyDataAsset.h
// 说明：表世界机枪敌人数值配置。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyDataAsset.h"
#include "MachineGunEnemyDataAsset.generated.h"

class AEnemyProjectile;
class UNiagaraSystem;

UCLASS(BlueprintType)
class UEGAMEJAM_API UMachineGunEnemyDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()

public:
	/** 预警阶段时长（1.0-1.5s） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float WarmupDuration = 1.25f;

	/** 连发阶段时长（1.2-1.5s） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float BurstDuration = 1.35f;

	/** 冷却阶段时长（1.5-2.0s） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float Cooldown = 1.7f;

	/** 射速：每秒发射数（5-8） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=1))
	float FireRate = 6.f;

	/** 扇形散射半角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float SpreadHalfAngleDeg = 4.f;

	/** 连发期间缓慢跟踪玩家的偏航速率（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float TrackingYawSpeed = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float BulletDamage = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG", meta=(ClampMin=0))
	float BulletSpeed = 5500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	/** 预警激光 Niagara（多束汇聚效果，资产层面做） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG|FX")
	TObjectPtr<UNiagaraSystem> WarningLasersNiagara;

	/** 开火闪光 Niagara */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MG|FX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;
};
