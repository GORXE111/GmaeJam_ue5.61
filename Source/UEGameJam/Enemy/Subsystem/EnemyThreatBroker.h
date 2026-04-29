// ===================================================
// 文件：EnemyThreatBroker.h
// 说明：威胁指示器中枢。UWorldSubsystem。敌人瞄准玩家时调用
//       BeginThreat，结束时调用 EndThreat。UI（玩家 HUD 的屏幕
//       边缘方向指示器）从这里订阅广播或轮询当前活跃威胁列表。
//
//       本模块不直接画 UI，只提供数据层；UI 由别的模块消费。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyThreatBroker.generated.h"

class AEnemyCharacter;

/** 一次活跃的威胁（敌人 A 正在瞄准玩家 B） */
USTRUCT(BlueprintType)
struct UEGAMEJAM_API FEnemyThreatEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Threat")
	TWeakObjectPtr<AEnemyCharacter> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Threat")
	TWeakObjectPtr<AActor> Target;

	/** 预计锁定总时长（秒），供 UI 做进度条/倒计时 */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Threat")
	float LockSeconds = 0.0f;

	/** 开始时的 World 时间（秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Threat")
	float StartWorldTime = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FThreatBegun, const FEnemyThreatEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FThreatEnded, AEnemyCharacter*, Source);

UCLASS()
class UEGAMEJAM_API UEnemyThreatBroker : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End USubsystem interface

	/** 敌人开始瞄准。同一 Source 重复调用会覆盖之前未结束的条目 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Threat")
	void BeginThreat(AEnemyCharacter* Source, AActor* Target, float LockSeconds);

	/** 敌人结束瞄准（开火、被打断、死亡） */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Threat")
	void EndThreat(AEnemyCharacter* Source);

	/** 查询当前所有活跃威胁 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Threat")
	const TArray<FEnemyThreatEntry>& GetActiveThreats() const { return Active; }

	/** 威胁新增广播 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Threat")
	FThreatBegun OnThreatBegun;

	/** 威胁结束广播 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Threat")
	FThreatEnded OnThreatEnded;

private:
	/** 活跃威胁列表 */
	UPROPERTY(Transient)
	TArray<FEnemyThreatEntry> Active;

	/** 找到对应 Source 的 entry 下标，找不到返回 INDEX_NONE */
	int32 FindEntryIndex(AEnemyCharacter* Source) const;
};
