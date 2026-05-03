// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

/**
 * 主角动作蓝图
 */
UCLASS()
class UEGAMEJAM_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 是否处于移动状态，用于状态机切换到移动 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	bool bIsMoving = false;

	/** 是否处于空中下落状态，用于切换到掉落动作，墙跑和冲刺时不会触发 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	bool bIsFalling = false;

	/** 是否处于地面起跳上升阶段，用于切换到起跳动作 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	bool bIsJumpStarting = false;

	/** 是否处于滑铲状态，用于动画状态机切换到滑铲 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	bool bIsSliding = false;

private:

	bool bWasOnGroundLastFrame = false;
};
