// ===================================================
// 文件：EnemyAIController.h
// 说明：敌人通用 AIController。挂载 UStateTreeAIComponent，在 OnPossess
//       时启动对应 StateTree。MVP 用 DetectionRadius + Actor Tag 查玩家，
//       不引入 UAIPerceptionComponent。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UStateTreeAIComponent;
class AEnemyCharacter;

UCLASS()
class UEGAMEJAM_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** 查找带此 Tag 的玩家 Pawn 并缓存 */
	UFUNCTION(BlueprintCallable, Category="Enemy|AI")
	AActor* RefreshPlayer();

	/** 读取缓存的玩家引用（不主动查找） */
	UFUNCTION(BlueprintPure, Category="Enemy|AI")
	AActor* GetCachedPlayer() const { return CachedPlayer.Get(); }

	/** 查找/返回带 PlayerTag 的玩家（找不到就再扫一次） */
	UFUNCTION(BlueprintCallable, Category="Enemy|AI")
	AActor* FindPlayerByTag();

	/** 清空当前缓存的玩家目标 */
	void ClearCachedPlayer();

	/** 读取 StateTree 组件 */
	UFUNCTION(BlueprintPure, Category="Enemy|AI")
	UStateTreeAIComponent* GetStateTreeAI() const { return StateTreeAI; }

protected:
	/** StateTree 组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	/** 玩家识别 Tag（玩家 Pawn 需要有此 Actor Tag） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI")
	FName PlayerTag = FName("Player");

	/** 缓存的玩家引用 */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedPlayer;

	/** Pawn 死亡时停 StateTree */
	UFUNCTION()
	void HandleOwnerDeath(AEnemyCharacter* DeadEnemy);
};
