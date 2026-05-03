// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Controller/PlayerCharacterController.h"
#include "Player/Character/GsPlayer.h"
#include "Player/UI/PlayerUI.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UEGameJam.h"
#include "UEGameJamCameraManager.h"

APlayerCharacterController::APlayerCharacterController()
{
	PlayerCameraManagerClass = AUEGameJamCameraManager::StaticClass();
}

void APlayerCharacterController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	PlayerUI = CreateWidget<UPlayerUI>(this, PlayerUIClass);

	if (PlayerUI)
	{
		PlayerUI->AddToPlayerScreen(0);
	}
	else if (PlayerUIClass)
	{
		UE_LOG(LogUEGameJam, Error, TEXT("Could not spawn player UI widget."));
	}

	InitializePlayerUI(GetPawn());
}

void APlayerCharacterController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	InitializePlayerUI(InPawn);
}

void APlayerCharacterController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			if (CurrentContext)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

void APlayerCharacterController::InitializePlayerUI(APawn* InPawn)
{
	if (!IsLocalPlayerController() || !PlayerUI)
	{
		return;
	}

	if (AGsPlayer* PlayerCharacter = Cast<AGsPlayer>(InPawn))
	{
		PlayerUI->BindPlayer(PlayerCharacter);
	}
	else
	{
		PlayerUI->BindPlayer(nullptr);
	}
}
