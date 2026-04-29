// ===================================================
// 文件：PistolEnemyDataAsset.h
// 说明：表世界手枪敌人数值配置。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyDataAsset.h"
#include "PistolEnemyDataAsset.generated.h"

class AEnemyProjectile;
class UNiagaraSystem;

UCLASS(BlueprintType)
class UEGAMEJAM_API UPistolEnemyDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()

public:
	/** 瞄准阶段时长（策划案 0.8-1.2s） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol", meta=(ClampMin=0))
	float AimDuration = 1.f;

	/** 闪烁开始比例（后 30% 闪烁 = 0.7） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol", meta=(ClampMin=0, ClampMax=1))
	float AimFlickerStartRatio = 0.7f;

	/** 冷却阶段时长（策划案 1.5-1.8s） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol", meta=(ClampMin=0))
	float Cooldown = 1.6f;

	/** 子弹速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol", meta=(ClampMin=0))
	float ProjectileSpeed = 4000.f;

	/** 子弹伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol", meta=(ClampMin=0))
	float ProjectileDamage = 15.f;

	/** 子弹类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	/** 瞄准激光 Niagara 资产 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol|FX")
	TObjectPtr<UNiagaraSystem> LaserNiagara;

	/** 开火闪光 Niagara */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pistol|FX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;
};
