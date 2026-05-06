// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class UUI_MainMenu;

/**
 * 
 */
UCLASS()
class UEGAMEJAM_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** 主菜单界面蓝图类，进入主菜单关卡时创建并显示 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu")
	TSubclassOf<UUI_MainMenu> MainMenuWidgetClass;

	/** 当前创建的主菜单界面实例 */
	UPROPERTY(Transient)
	TObjectPtr<UUI_MainMenu> MainMenuWidget;
};
