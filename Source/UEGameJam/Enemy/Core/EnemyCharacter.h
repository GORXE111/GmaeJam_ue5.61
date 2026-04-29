// ===================================================
// 文件：EnemyCharacter.h
// 说明：Enemy 模块的 Pawn 基类。继承 UE 原生 ACharacter，不依赖
//       项目其他 Character 子类，保证 Enemy 模块独立可迁移。
//
//       基类承载的公共职责：
//         - 血量与受击（UEnemyHealthComponent）
//         - 里/表世界碰撞切换（URealmTagComponent）
//         - 死亡时冻结 RealmTag 防碰撞覆盖（UEnemyRealmGuardComponent）
//         - 威胁视觉接口（IEnemyThreatInterface）
//         - 死亡广播（供管理器与 AI 控制器订阅）
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyTypes.h"
#include "EnemyThreatInterface.h"
#include "EnemyCharacter.generated.h"

class URealmTagComponent;
class UEnemyHealthComponent;
class UEnemyRealmGuardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyDiedSignature, class AEnemyCharacter*, DeadEnemy);

/**
 *  敌人 Pawn 基类。抽象——只能派生使用。
 *  子类通过 Archetype 自报身份，管理器据此分类计数。
 */
UCLASS(abstract)
class UEGAMEJAM_API AEnemyCharacter : public ACharacter, public IEnemyThreatInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	/** 敌人原型，决定通用行为分组 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Identity")
	EEnemyArchetype Archetype = EEnemyArchetype::None;

	/** 里/表世界碰撞切换组件；基类默认 Surface，近战里世界子类构造改 Realm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<URealmTagComponent> RealmTag;

	/** 血量与受击规则组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<UEnemyHealthComponent> HealthComponent;

	/** 死亡时冻结 RealmTag 的守卫组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<UEnemyRealmGuardComponent> RealmGuard;

	/** 死亡后多久 Destroy 本 Actor（留给 ragdoll 停稳的时间） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death", meta = (ClampMin = 0, Units = "s"))
	float DestroyDelay = 3.0f;

	/** Ragdoll 使用的碰撞 Profile 名 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death")
	FName RagdollCollisionProfile = FName(TEXT("Ragdoll"));

	bool bIsDead = false;
	FTimerHandle DeferredDestroyHandle;

public:
	/** 死亡广播（管理器 / AI 控制器订阅） */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Death")
	FEnemyDiedSignature OnEnemyDied;

	//~Begin AActor/APawn interface
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End AActor/APawn interface

	/** 读取敌人原型 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Identity")
	EEnemyArchetype GetArchetype() const { return Archetype; }

	/** 是否已死亡 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Death")
	bool IsDead() const { return bIsDead; }

	/**
	 *  施加外力推动。死亡前用 LaunchCharacter，死亡后对 ragdoll Mesh 加冲量。
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Death")
	void ApplyPush(const FVector& PushVelocity);

	/**
	 *  开始威胁（瞄准）视觉与广播。StateTree 的 Telegraph Task 进入时调用。
	 *  TODO(任务 3)：接入 UEnemyThreatBroker::BeginThreat(this, Target, LockSeconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Threat")
	void BroadcastThreatBegin(AActor* Target, float LockSeconds);

	/**
	 *  结束威胁（瞄准）视觉与广播。Telegraph Task 退出或开火后调用。
	 *  TODO(任务 3)：接入 UEnemyThreatBroker::EndThreat(this)
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Threat")
	void BroadcastThreatEnd();

	//~Begin IEnemyThreatInterface（基类提供空实现，子类/蓝图可重写）
	virtual void OnThreatVisualStart_Implementation(AActor* Target, float LockSeconds) override {}
	virtual void OnThreatVisualStop_Implementation() override {}
	//~End IEnemyThreatInterface

protected:
	/** 血量归零后的死亡处理流程 */
	virtual void HandleDeath(AActor* Killer);

	/** 延时销毁回调 */
	UFUNCTION()
	void DeferredDestroy();
};
