// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsRespawnPoint.generated.h"

class USceneComponent;

/**
 * 可放置在场景中的玩家复活点。
 */
UCLASS(Blueprintable)
class UEGAMEJAM_API AGsRespawnPoint : public AActor
{
	GENERATED_BODY()

	/** 复活点根组件，使用 Actor 位置和朝向作为玩家复活 Transform */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

protected:
	/** 复活点编号，越大表示越靠后的关卡进度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn Point", meta = (ClampMin = 0))
	int32 CheckpointIndex = 0;

public:
	AGsRespawnPoint();

	UFUNCTION(BlueprintPure, Category="Respawn Point")
	int32 GetCheckpointIndex() const { return CheckpointIndex; }
};
