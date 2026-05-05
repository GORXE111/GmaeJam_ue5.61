// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UEGameJamGameMode.generated.h"

class UAudioDataAsset;
/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AUEGameJamGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AUEGameJamGameMode();
};



