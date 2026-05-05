// ===================================================
// 文件：EnemyCharacter.h
// 说明：敌人抽象基类。继承 UE 原生 ACharacter（不依赖项目其他角色）。
//       职责：
//         - 挂载 RealmTag / EnemyHealth 组件
//         - 重写 TakeDamage，转发给 Health
//         - Die() 流程：Ragdoll + 关碰撞 + 停 AI + 计时器 Destroy
//         - 向 UEnemySubsystem 注册/注销
//         - ApplyDataAsset() 把 DataAsset 数值拷到运行时
//         - 子类重写 ApplyDataAsset 以处理专属字段
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RealmTagComponent.h"
#include "EnemyCharacter.generated.h"

class URealmTagComponent;
class UEnemyHealthComponent;
class UEnemyDataAsset;
class USoundBase;
class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyDeathSignature, AEnemyCharacter*, DeadEnemy);

UCLASS(Abstract)
class UEGAMEJAM_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;

	/** 敌人数值资产 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Data")
	TObjectPtr<UEnemyDataAsset> EnemyData;

	/** 默认 RealmType（构造时应用到 RealmTag；蓝图子类可覆写） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Realm")
	ERealmType DefaultRealmType = ERealmType::Surface;

	/** 死亡后延迟销毁的时长（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Death", meta=(ClampMin=0))
	float DeferredDestructionTime = 3.0f;

	/** Ragdoll 使用的碰撞 Profile 名 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Death")
	FName RagdollCollisionProfile = FName("Ragdoll");

	/** 死亡音效（可选） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Death")
	TObjectPtr<USoundBase> DeathSound;

	/** 敌人死亡广播 */
	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FEnemyDeathSignature OnEnemyDeath;

	/** 是否已死亡 */
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsDead() const { return bIsDead; }

	/** 当前 RealmType（从 RealmTag 读取） */
	UFUNCTION(BlueprintPure, Category="Enemy|Realm")
	ERealmType GetEnemyRealmType() const;

	UFUNCTION(BlueprintPure, Category="Enemy|Components")
	URealmTagComponent* GetRealmTag() const { return RealmTag; }

	UFUNCTION(BlueprintPure, Category="Enemy|Components")
	UEnemyHealthComponent* GetHealth() const { return Health; }

	/** 把 DataAsset 的数值拷到运行时属性（基类处理 MaxHP/WalkSpeed 等），子类可扩展 */
	virtual void ApplyDataAsset();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components")
	TObjectPtr<URealmTagComponent> RealmTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components")
	TObjectPtr<UEnemyHealthComponent> Health;

	UPROPERTY()
	bool bIsDead = false;

	FTimerHandle DeathTimer;

	/** 执行死亡流程：Ragdoll、关碰撞、停 AI、计时器 */
	virtual void Die();

	/** 计时器回调 */
	virtual void DeferredDestruction();

	/** 绑定 Health 的 OnDepleted */
	UFUNCTION()
	void HandleHealthDepleted(UEnemyHealthComponent* Src);
};
