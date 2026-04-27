// ===================================================
// 文件：EnemySubsystem.h
// 说明：全局敌人管理器（UWorldSubsystem）。敌人 BeginPlay 时注册、
//       Die() 时注销；对外提供按 Realm/Class 过滤的统计查询，以及
//       "当前关卡所有敌人死完"的一次性广播。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RealmTagComponent.h"
#include "EnemySubsystem.generated.h"

class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyRegisteredSignature,   AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyUnregisteredSignature, AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAllEnemiesDeadSignature);

UCLASS()
class UEGAMEJAM_API UEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 便捷访问器 */
	UFUNCTION(BlueprintPure, Category="Enemy|Subsystem", meta=(WorldContext="WorldContext"))
	static UEnemySubsystem* Get(const UObject* WorldContext);

	/** 注册敌人（AEnemyCharacter::BeginPlay 调用） */
	void RegisterEnemy(AEnemyCharacter* Enemy);

	/** 注销敌人（AEnemyCharacter::Die 或 EndPlay 调用；幂等） */
	void UnregisterEnemy(AEnemyCharacter* Enemy);

	UFUNCTION(BlueprintPure, Category="Enemy|Subsystem")
	int32 GetAliveCount() const;

	UFUNCTION(BlueprintPure, Category="Enemy|Subsystem")
	int32 GetAliveCountByRealm(ERealmType Realm) const;

	UFUNCTION(BlueprintPure, Category="Enemy|Subsystem")
	int32 GetAliveCountByClass(TSubclassOf<AEnemyCharacter> Class) const;

	UFUNCTION(BlueprintPure, Category="Enemy|Subsystem")
	bool AreAllEnemiesDead() const;

	UFUNCTION(BlueprintCallable, Category="Enemy|Subsystem")
	void GetAliveEnemies(TArray<AEnemyCharacter*>& OutEnemies) const;

	/** 一次性胜利广播（全部死完后触发一次） */
	UPROPERTY(BlueprintAssignable, Category="Enemy|Subsystem")
	FAllEnemiesDeadSignature OnAllEnemiesDead;

	UPROPERTY(BlueprintAssignable, Category="Enemy|Subsystem")
	FEnemyRegisteredSignature OnEnemyRegistered;

	UPROPERTY(BlueprintAssignable, Category="Enemy|Subsystem")
	FEnemyUnregisteredSignature OnEnemyUnregistered;

	/** 调试：杀光所有敌人 */
	UFUNCTION(Exec, Category="Enemy|Subsystem")
	void EnemyKillAll();

	/** 调试：打印当前敌人分组计数 */
	UFUNCTION(Exec, Category="Enemy|Subsystem")
	void EnemyDump() const;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AEnemyCharacter>> AliveEnemies;

	bool bAnnouncedAllDead = false;
	bool bHadAnyRegistration = false;

	/** 清理数组里过期的 WeakPtr */
	void CompactAliveList();

	void MaybeBroadcastAllDead();
};
