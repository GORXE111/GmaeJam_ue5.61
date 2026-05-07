// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GsLevelStateGameState.generated.h"

class AGsRespawnPoint;

/**
 * 保存当前关卡运行时复活进度。
 */
UCLASS()
class UEGAMEJAM_API AGsLevelStateGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGsLevelStateGameState();

protected:
	/** 当前已激活的复活点编号，越大表示越靠后的关卡进度 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Level State|Respawn")
	int32 CurrentCheckpointIndex = INDEX_NONE;

	/** 当前死亡后使用的复活位置与朝向 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Level State|Respawn")
	FTransform CurrentRespawnTransform = FTransform::Identity;

	/** 是否已经记录可用的复活位置 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Level State|Respawn")
	bool bHasRespawnTransform = false;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AGsRespawnPoint>> RespawnPointsByIndex;

protected:
	virtual void BeginPlay() override;

public:
	/** 激活指定编号的复活点，仅允许推进到更靠后的编号 */
	UFUNCTION(BlueprintCallable, Category="Level State|Respawn")
	bool ActivateCheckpointByIndex(int32 CheckpointIndex);

	/** 获取当前记录的复活位置，如果还没有记录则返回 false */
	UFUNCTION(BlueprintPure, Category="Level State|Respawn")
	bool GetCurrentRespawnTransform(FTransform& OutRespawnTransform) const;

	/** 在没有场景复活点时，使用传入位置作为兜底复活点 */
	UFUNCTION(BlueprintCallable, Category="Level State|Respawn")
	void EnsureFallbackRespawnTransform(const FTransform& FallbackTransform);

	/** 获取当前已激活的复活点编号 */
	UFUNCTION(BlueprintPure, Category="Level State|Respawn")
	int32 GetCurrentCheckpointIndex() const { return CurrentCheckpointIndex; }

	/** 当前是否已经记录可用的复活位置 */
	UFUNCTION(BlueprintPure, Category="Level State|Respawn")
	bool HasRespawnTransform() const { return bHasRespawnTransform; }

private:
	void CacheRespawnPoints();
	void ActivateInitialCheckpoint();
};
