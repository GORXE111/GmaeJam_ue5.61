// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsRespawnTriggerBox.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/**
 * 玩家进入后推进到指定复活点编号的碰撞盒。
 */
UCLASS(Blueprintable)
class UEGAMEJAM_API AGsRespawnTriggerBox : public AActor
{
	GENERATED_BODY()

	/** 复活点推进检测盒，玩家进入后尝试激活目标复活点编号 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerCollision;

protected:
	/** 玩家进入碰撞盒后要激活的复活点编号，越大表示越靠后的关卡进度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn Trigger", meta = (ClampMin = 0))
	int32 TargetCheckpointIndex = 0;

public:
	AGsRespawnTriggerBox();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
