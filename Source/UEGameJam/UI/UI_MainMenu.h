// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_MainMenu.generated.h"

class UButton;
class UWidget;
class UUI_SettingsMenu;

/**
 * 主界面
 */
UCLASS()
class UEGAMEJAM_API UUI_MainMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 开始游戏按钮 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartBtn;

	/** 设置按钮 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SetBtn;

	/** 退出游戏按钮 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ExitBtn;

	/** 点击开始游戏后进入的关卡名 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu")
	FName StartLevelName = TEXT("LEVEL_01");

	/** 设置界面蓝图类，点击设置按钮时创建并显示 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu")
	TSubclassOf<UUI_SettingsMenu> SettingsWidgetClass;

	/** 当前创建的设置界面实例 */
	UPROPERTY(Transient)
	TObjectPtr<UUI_SettingsMenu> SettingsWidget;

private:
	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleExitClicked();
};
