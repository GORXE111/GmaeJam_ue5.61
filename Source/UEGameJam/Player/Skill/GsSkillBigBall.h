// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsSkillBigBall.generated.h"

class URealmRevealerComponent;
class USphereComponent;

/**
 * 技能命中后生成的大球，负责变大表现与里世界揭示。
 */
UCLASS()
class UEGAMEJAM_API AGsSkillBigBall : public AActor
{
	GENERATED_BODY()

	/** 大球的碰撞体，用于命中后停留在场景中形成阻挡范围 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	/** 大球的里世界揭示组件，用于在大球范围内切换里世界表现 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URealmRevealerComponent> RealmRevealerComponent;

protected:
	/** 大球刚生成时的整体缩放，用于表现从小球开始变大 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0))
	FVector InitialActorScale = FVector(0.25f);

	/** 大球变大完成后的整体缩放，用于控制最终视觉大小 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0))
	FVector TargetActorScale = FVector(1.0f);

	/** 大球从初始缩放变到目标缩放所需时间，0表示立即变大 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "s"))
	float GrowDuration = 0.25f;

	/** 大球的碰撞半径，用于决定停留后的阻挡范围 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "cm"))
	float CollisionRadius = 64.0f;

	/** 大球存在多久后自动销毁，0表示不自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "s"))
	float DestroyDelay = 30.0f;

	/** 当前已经变大的时间 */
	float CurrentGrowTime = 0.0f;

public:
	AGsSkillBigBall();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
