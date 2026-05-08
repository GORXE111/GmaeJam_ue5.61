// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsSkillBall.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class AGsSkillBigBall;

/**
 * 玩家技能球的基础实现，负责飞行、碰撞与销毁。
 */
UCLASS()
class UEGAMEJAM_API AGsSkillBall : public AActor
{
	GENERATED_BODY()

	/** 技能球的碰撞体，用于重叠触发命中 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

protected:
	/** 技能球的直线飞行速度，数值越大每秒飞得越远 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Ball", meta = (ClampMin = 0, Units = "cm/s"))
	float MoveSpeed = 3000.0f;

	/** 小球从生成点飞出超过这个距离后自动消失，0表示不按距离自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Ball", meta = (ClampMin = 0, Units = "cm"))
	float MaxFlightDistance = 10000.0f;

	/** 小球命中后生成的大球类，可在蓝图中替换具体表现 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Ball")
	TSubclassOf<AGsSkillBigBall> ImpactBallClass;

	/** 技能球开始飞行的位置，用于计算最大飞行距离 */
	FVector StartLocation = FVector::ZeroVector;

	/** 技能球初始化时确定的固定飞行方向 */
	FVector FlightDirection = FVector::ForwardVector;

	/** 是否已经初始化固定飞行方向 */
	bool bHasFlightDirection = false;

	/** 是否已经停止移动 */
	bool bStopped = false;

public:

	AGsSkillBall();

	/** 设置技能球要飞向的目标点，并允许其开始移动 */
	void InitializeSkillBall(const FVector& InTargetLocation);

	/** 当前场景里是否还有技能（小球或其生成的大球）尚未结束。蓝图可调用，用于禁用UI按钮等。 */
	UFUNCTION(BlueprintPure, Category="Skill Ball")
	static bool IsAnySkillActive();

	/** 注册自己为当前活跃技能。小球 BeginPlay 调用一次，命中后大球 BeginPlay 再次接管覆盖。 */
	static void SetActiveSkill(AActor* InActor);

	/** 仅当 ActiveSkillPtr 仍指向 InActor 时才清空，避免大球已接管后小球的 EndPlay 把它清掉。 */
	static void ClearActiveSkillIf(AActor* InActor);
public:
	virtual void Tick(float DeltaSeconds) override;
protected:
	/** 当前活跃的技能 actor（小球或大球，二者接力共享同一个槽） */
	static TWeakObjectPtr<AActor> ActiveSkillPtr;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnCollisionComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 处理小球命中，生成大球并销毁自己 */
	void HandleImpact(const FVector& ImpactLocation);
};
