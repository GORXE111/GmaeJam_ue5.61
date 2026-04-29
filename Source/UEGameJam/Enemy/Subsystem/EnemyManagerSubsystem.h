// ===================================================
// 文件：EnemyManagerSubsystem.h
// 说明：全局敌人管理器。UWorldSubsystem，关卡开局自生自灭。
//       敌人 BeginPlay 自注册，EndPlay/死亡自注销。胜利判断
//       Actor、Boss UI、关卡脚本等外部系统通过本 Subsystem
//       查询敌人数量、订阅死亡事件。
//
//       注意：OnAllEnemiesCleared 仅在"至少注册过 1 个敌人，
//       然后数量降到 0"时触发；关卡开局没敌人不会误触发。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyTypes.h"
#include "EnemyManagerSubsystem.generated.h"

class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyManagerEnemyDied, AEnemyCharacter*, DeadEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyManagerRosterChanged, int32, NewAliveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnemyManagerAllCleared);

UCLASS()
class UEGAMEJAM_API UEnemyManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End USubsystem interface

	/** 注册一个敌人。AEnemyCharacter::BeginPlay 自动调用 */
	void RegisterEnemy(AEnemyCharacter* Enemy);

	/** 注销一个敌人。AEnemyCharacter::EndPlay 自动调用 */
	void UnregisterEnemy(AEnemyCharacter* Enemy);

	/** 当前存活敌人总数（跳过已无效的弱引用） */
	UFUNCTION(BlueprintPure, Category = "Enemy|Manager")
	int32 GetAliveCount() const;

	/** 按原型统计存活敌人数量 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Manager")
	int32 GetAliveCountByArchetype(EEnemyArchetype ArchetypeFilter) const;

	/** 获取所有存活敌人（已过滤无效弱引用） */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Manager")
	TArray<AEnemyCharacter*> GetAllAlive() const;

	/** 任意敌人死亡广播 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Manager")
	FEnemyManagerEnemyDied OnAnyEnemyDied;

	/** 花名册变化（新增注册或任一死亡都触发） */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Manager")
	FEnemyManagerRosterChanged OnEnemyRosterChanged;

	/** 所有曾注册过的敌人都死亡后触发一次 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Manager")
	FEnemyManagerAllCleared OnAllEnemiesCleared;

private:
	/** 存活敌人弱引用列表 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AEnemyCharacter>> Alive;

	/** 关卡至今是否注册过任何敌人；用于 AllCleared 的"门控" */
	bool bAnyEverRegistered = false;

	/** 敌人死亡回调（在 RegisterEnemy 时绑定到其 OnEnemyDied） */
	UFUNCTION()
	void HandleEnemyDied(AEnemyCharacter* DeadEnemy);
};
