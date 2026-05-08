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

public:
	/** 大球生命周期阶段：生长 → 停留 → 收缩 → 销毁 */
	enum class EPhase : uint8
	{
		Growing,
		Holding,
		Shrinking,
	};

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

	/** 大球达到最大尺寸后维持的时间，期间持续揭示里世界 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "s"))
	float HoldDuration = 2.0f;

	/** 大球从最大尺寸缩回到 0 所需时间，0表示立即消失 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "s"))
	float ShrinkDuration = 0.5f;

	/** 大球的碰撞半径，用于决定停留后的阻挡范围 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill Big Ball", meta = (ClampMin = 0, Units = "cm"))
	float CollisionRadius = 64.0f;

	/** 当前所处阶段 */
	EPhase Phase = EPhase::Growing;

	/** 当前阶段已经过去的时间 */
	float PhaseElapsed = 0.0f;

	/** BeginPlay 时缓存的揭示半径基准值（来自 RealmRevealerComponent 的蓝图配置）。
	 *  对应"球完全长大"时的揭示范围，每帧按 actor scale 比例缩放写回。 */
	float BaseRevealRadius = 0.0f;

public:
	AGsSkillBigBall();

	/** 球完全长大时的揭示半径（== BeginPlay 捕获的 RealmRevealerComponent 蓝图默认值）。
	 *  外部（例如敌人子弹的跨界拦截）用这个值当作"稳定边界"，忽略 Growing/Shrinking
	 *  动画期的半径抖动。 */
	float GetMaxRevealRadius() const { return BaseRevealRadius; }

	/** 当前场上活跃的大球（同一时刻最多一只，由 AGsSkillBall::SetActiveSkill 约束）。
	 *  BeginPlay 时登记，EndPlay/Destroy 时清空。没有时返回 nullptr。 */
	static AGsSkillBigBall* GetActiveInstance() { return ActiveInstance.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 进入下一阶段，重置计时 */
	void EnterPhase(EPhase NewPhase);

private:
	static TWeakObjectPtr<AGsSkillBigBall> ActiveInstance;
};
