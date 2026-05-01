// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsSkillBall.generated.h"

class URealmRevealerComponent;
class USphereComponent;

/**
 * 玩家技能球的基础实现，负责飞行、碰撞与销毁。
 */
UCLASS()
class UEGAMEJAM_API AGsSkillBall : public AActor
{
	GENERATED_BODY()

	/** 技能球的碰撞体，用于阻挡移动并停在命中位置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URealmRevealerComponent> RealmRevealerComponent;
protected:
	/** 技能球的直线飞行速度，数值越大每秒飞得越远 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Ball", meta = (ClampMin = 0, Units = "cm/s"))
	float MoveSpeed = 3000.0f;

	/** 技能球存在多久后自动销毁，0表示不自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Ball", meta = (ClampMin = 0, Units = "s"))
	float DestroyDelay = 30.0f;

	/** 技能球要飞向的目标点 */
	FVector TargetLocation = FVector::ZeroVector;

	/** 是否已经设置了有效的目标点 */
	bool bHasTarget = false;

	/** 是否已经停止移动 */
	bool bStopped = false;

public:

	AGsSkillBall();

	/** 设置技能球要飞向的目标点，并允许其开始移动 */
	void InitializeSkillBall(const FVector& InTargetLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
