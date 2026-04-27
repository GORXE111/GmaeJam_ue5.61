// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerUI.generated.h"

/**
 *  纯玩家侧角色 UI 基类
 */
UCLASS(Abstract)
class UEGAMEJAM_API UPlayerUI : public UUserWidget
{
	GENERATED_BODY()

public:

	/** 允许蓝图根据当前生命百分比更新血条和受伤反馈 */
	UFUNCTION(BlueprintImplementableEvent, Category="Player", meta = (DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);
};
