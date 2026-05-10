// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsSkillAimAssistPoint.generated.h"

class USceneComponent;

/**
 * 可放置在场景中的技能吸附点
 */
UCLASS(Blueprintable)
class UEGAMEJAM_API AGsSkillAimAssistPoint : public AActor
{
	GENERATED_BODY()

	/** 技能吸附点根组件，用于决定技能吸附的目标位置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

protected:
	/** 是否允许玩家发射技能时吸附到这个场景点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill Aim Assist", meta = (AllowPrivateAccess = "true"))
	bool bEnabled = true;

public:
	AGsSkillAimAssistPoint();

	UFUNCTION(BlueprintPure, Category="Skill Aim Assist")
	bool IsSkillAimAssistEnabled() const { return bEnabled; }

	UFUNCTION(BlueprintPure, Category="Skill Aim Assist")
	FVector GetSkillAimTargetLocation() const;
};
