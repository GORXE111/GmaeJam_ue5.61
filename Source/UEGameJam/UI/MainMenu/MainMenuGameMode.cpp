// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/UI_MainMenu.h"

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController || !MainMenuWidgetClass)
	{
		return;
	}

	MainMenuWidget = CreateWidget<UUI_MainMenu>(PlayerController, MainMenuWidgetClass);
	if (!MainMenuWidget)
	{
		return;
	}

	MainMenuWidget->AddToViewport(0);

	PlayerController->bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	PlayerController->SetInputMode(InputMode);
}
