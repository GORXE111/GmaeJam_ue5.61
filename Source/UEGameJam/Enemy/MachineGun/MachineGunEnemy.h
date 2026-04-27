// ===================================================
// 文件：MachineGunEnemy.h
// 说明：表世界机枪敌人。Warmup(多激光) → Burst(扇形+跟踪) → Cooldown。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "MachineGunEnemy.generated.h"

class USceneComponent;
class UNiagaraComponent;
class UMachineGunEnemyDataAsset;

UCLASS()
class UEGAMEJAM_API AMachineGunEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMachineGunEnemy();

	virtual void ApplyDataAsset() override;

	/** 激活/关闭预警激光 */
	UFUNCTION(BlueprintCallable, Category="Enemy|MG")
	void SetWarningLasersActive(bool bActive, AActor* AimTarget);

	/** 发射一发子弹（带扇形散射） */
	UFUNCTION(BlueprintCallable, Category="Enemy|MG")
	void FireOneBullet(const FVector& AimLocation);

	/** 允许/禁止连发期间缓慢跟踪玩家 */
	UFUNCTION(BlueprintCallable, Category="Enemy|MG")
	void SetTrackingEnabled(bool bEnabled, AActor* TrackTarget);

	UFUNCTION(BlueprintPure, Category="Enemy|MG")
	FVector GetMuzzleLocation() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|MG")
	TObjectPtr<USceneComponent> MuzzleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|MG")
	TObjectPtr<UNiagaraComponent> WarningLasersFX;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentWarningTarget;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTrackTarget;

	UPROPERTY()
	bool bWarningActive = false;

	UPROPERTY()
	bool bTracking = false;
};
