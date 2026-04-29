// ===================================================
// 文件：EnemyHealthComponent.h
// 说明：敌人专属血量组件。独立于玩家血量系统，仅服务敌人模块。
//       负责：HP 维护、伤害处理、死亡广播；不处理 Ragdoll / Destroy
//       等外部效果，这些由 AEnemyCharacter 在收到 OnDepleted 时处理。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHealthComponent.generated.h"

class UEnemyHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyHealthDepletedSignature, UEnemyHealthComponent*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEnemyHealthChangedSignature, UEnemyHealthComponent*, Source, float, NewHP, float, Delta);

UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class UEGAMEJAM_API UEnemyHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyHealthComponent();

	/** 最大血量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin=1))
	float MaxHP = 1.0f;

	/** 当前血量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHP = 1.0f;

	/** 无敌开关 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	bool bInvulnerable = false;

	/** HP 耗尽广播 */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FEnemyHealthDepletedSignature OnDepleted;

	/** HP 变化广播 */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FEnemyHealthChangedSignature OnHealthChanged;

	/** 应用伤害；返回实际扣除的血量 */
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDamage(float Amount, AActor* Instigator);

	/** 重置最大血量；bHealFully=true 时同步把 CurrentHP 拉满 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMaxHP(float NewMax, bool bHealFully);

	/** 当前血量比例 [0,1] */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHPRatio() const;

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return CurrentHP <= 0.f; }

protected:
	virtual void BeginPlay() override;
};
