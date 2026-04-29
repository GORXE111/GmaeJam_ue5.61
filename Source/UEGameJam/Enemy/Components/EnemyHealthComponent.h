// ===================================================
// 文件：EnemyHealthComponent.h
// 说明：敌人血量与受击规则组件。封装"一击必杀 vs 多段血量"
//       的差异，并在需要时查询 UEnemyCoreExposureComponent
//       以实现重装敌人"核心暴露时一刀秒"的判定。
//
//       组件不直接处理死亡动画或销毁，只维护血量数字；血量
//       归零后由 AEnemyCharacter::TakeDamage 调用 HandleDeath。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHealthComponent.generated.h"

class UEnemyCoreExposureComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEnemyHealthChanged, float, NewHP, float, MaxHP);

UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class UEGAMEJAM_API UEnemyHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyHealthComponent();

	/** 最大血量。多段血量敌人（重装）可设大；一击必杀敌人保持 1 即可 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|HP", meta = (ClampMin = 1))
	float MaxHP = 1.0f;

	/** 是否启用"一击必杀"规则：任意正值伤害都直接清零血量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|HP")
	bool bOneShotKillRule = true;

protected:
	/** 当前血量，仅运行时读取 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|HP")
	float CurrentHP = 1.0f;

public:
	/** 血量变化广播 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|HP")
	FEnemyHealthChanged OnHealthChanged;

	/** 读取当前血量 */
	UFUNCTION(BlueprintPure, Category = "Enemy|HP")
	float GetCurrentHP() const { return CurrentHP; }

	/** 读取最大血量 */
	UFUNCTION(BlueprintPure, Category = "Enemy|HP")
	float GetMaxHP() const { return MaxHP; }

	/** 是否已经死亡（血量 <= 0） */
	UFUNCTION(BlueprintPure, Category = "Enemy|HP")
	bool IsDepleted() const { return CurrentHP <= 0.0f; }

	/**
	 *  施加伤害。返回真实扣血数（用于外部日志）。
	 *  若启用一击必杀且 InDamage > 0，则血量直接清零。
	 *  否则根据核心暴露查询决定一刀秒或扣 1 点。
	 */
	float ApplyDamage(float InDamage, AActor* Causer);

protected:
	virtual void BeginPlay() override;

private:
	/** 缓存的 CoreExposure（可能为空——非重装敌人不挂此组件） */
	UPROPERTY(Transient)
	TObjectPtr<UEnemyCoreExposureComponent> CachedCoreExposure;
};
