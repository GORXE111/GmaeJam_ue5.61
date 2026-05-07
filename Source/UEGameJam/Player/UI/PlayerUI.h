// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerUI.generated.h"

class AGsPlayer;
class UTextBlock;

/**
 *  纯玩家侧角色 UI 基类
 */
UCLASS(Abstract)
class UEGAMEJAM_API UPlayerUI : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindPlayer(AGsPlayer* InPlayer);

protected:
	virtual void NativeDestruct() override;

	/** 玩家死亡时显示的提示文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DieText;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGsPlayer> BoundPlayer;

	UFUNCTION()
	void HandlePlayerDeath();

	UFUNCTION()
	void HandlePlayerRespawn();

	void SetDieTextVisible(bool bVisible);
};
