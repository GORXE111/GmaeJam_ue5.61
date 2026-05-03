// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GsGrapplePointUI.generated.h"

class UImage;

/**
 * 场景中钩爪点使用的状态 UI
 */
UCLASS(Abstract, Blueprintable)
class UEGAMEJAM_API UGsGrapplePointUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Grapple Point")
	void SetPlayerNearby(bool bNearby);

protected:
	virtual void NativeConstruct() override;

	/** 玩家未靠近钩爪点时显示的 Image，需要在 Widget 蓝图中命名为 FarImage */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> FarImage;

	/** 玩家靠近钩爪点时显示的 Image，需要在 Widget 蓝图中命名为 NearImage */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> NearImage;

	/** 玩家靠近状态变化时触发，蓝图可在这里播放动画或补充表现 */
	UFUNCTION(BlueprintImplementableEvent, Category="Grapple Point", meta = (DisplayName = "On Nearby State Changed"))
	void BP_OnNearbyStateChanged(bool bNearby);

private:
	bool bIsPlayerNearby = false;

	void ApplyImageVisibility();
};
