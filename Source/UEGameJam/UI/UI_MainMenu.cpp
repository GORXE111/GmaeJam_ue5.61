// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_MainMenu.h"

#include "UI_SettingsMenu.h"
#include "Components/Button.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UUI_MainMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartBtn)
	{
		StartBtn->OnClicked.RemoveDynamic(this, &UUI_MainMenu::HandleStartClicked);
		StartBtn->OnClicked.AddDynamic(this, &UUI_MainMenu::HandleStartClicked);
	}

	if (SetBtn)
	{
		SetBtn->OnClicked.RemoveDynamic(this, &UUI_MainMenu::HandleSettingsClicked);
		SetBtn->OnClicked.AddDynamic(this, &UUI_MainMenu::HandleSettingsClicked);
	}

	if (ExitBtn)
	{
		ExitBtn->OnClicked.RemoveDynamic(this, &UUI_MainMenu::HandleExitClicked);
		ExitBtn->OnClicked.AddDynamic(this, &UUI_MainMenu::HandleExitClicked);
	}
}

void UUI_MainMenu::HandleStartClicked()
{
	if (!StartLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, StartLevelName);
	}
}

void UUI_MainMenu::HandleSettingsClicked()
{
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Visible);

		if (!SettingsWidget->IsInViewport())
		{
			SettingsWidget->AddToViewport(10);
		}

		return;
	}

	if (!SettingsWidgetClass)
	{
		return;
	}

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SettingsWidget = CreateWidget<UUI_SettingsMenu>(OwningPlayer, SettingsWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		SettingsWidget = CreateWidget<UUI_SettingsMenu>(World, SettingsWidgetClass);
	}

	if (SettingsWidget)
	{
		SettingsWidget->AddToViewport(10);
	}
}

void UUI_MainMenu::HandleExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
