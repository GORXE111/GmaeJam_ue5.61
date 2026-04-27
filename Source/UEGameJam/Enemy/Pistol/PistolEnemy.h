// ===================================================
// 文件：PistolEnemy.h
// 说明：表世界手枪敌人。Aim → Fire → Cooldown 循环。
//       Aim 阶段激活红色激光 Niagara 指向玩家，接近开火时闪烁。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "PistolEnemy.generated.h"

class USceneComponent;
class UNiagaraComponent;
class UPistolEnemyDataAsset;
class AEnemyProjectile;

UCLASS()
class UEGAMEJAM_API APistolEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	APistolEnemy();

	virtual void ApplyDataAsset() override;

	/** 激活/关闭激光（持续指向 AimTarget） */
	UFUNCTION(BlueprintCallable, Category="Enemy|Pistol")
	void SetLaserActive(bool bActive, AActor* AimTarget);

	/** 设置激光闪烁状态（Niagara 用户参数） */
	UFUNCTION(BlueprintCallable, Category="Enemy|Pistol")
	void SetLaserFlicker(bool bFlicker);

	/** 发射一发子弹朝 AimLocation 方向 */
	UFUNCTION(BlueprintCallable, Category="Enemy|Pistol")
	void FireProjectile(const FVector& AimLocation);

	UFUNCTION(BlueprintPure, Category="Enemy|Pistol")
	FVector GetMuzzleLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Pistol")
	TObjectPtr<USceneComponent> MuzzleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Pistol")
	TObjectPtr<UNiagaraComponent> LaserFX;

	/** 当前被激光指向的目标；每帧把 LaserFX 的 BeamEnd 刷到它 */
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentAimTarget;

	UPROPERTY()
	bool bLaserActive = false;
};
