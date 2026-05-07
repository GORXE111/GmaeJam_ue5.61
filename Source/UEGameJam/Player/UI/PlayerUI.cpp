// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerUI.h"
#include "Components/TextBlock.h"
#include "Player/Character/GsPlayer.h"

void UPlayerUI::BindPlayer(AGsPlayer* InPlayer)
{
	if (BoundPlayer)
	{
		BoundPlayer->OnDeath.RemoveDynamic(this, &UPlayerUI::HandlePlayerDeath);
		BoundPlayer->OnRespawn.RemoveDynamic(this, &UPlayerUI::HandlePlayerRespawn);
	}

	BoundPlayer = InPlayer;
	SetDieTextVisible(false);

	if (!BoundPlayer)
	{
		return;
	}

	BoundPlayer->OnDeath.AddDynamic(this, &UPlayerUI::HandlePlayerDeath);
	BoundPlayer->OnRespawn.AddDynamic(this, &UPlayerUI::HandlePlayerRespawn);
	if (BoundPlayer->IsDead())
	{
		HandlePlayerDeath();
	}
}

void UPlayerUI::NativeDestruct()
{
	if (BoundPlayer)
	{
		BoundPlayer->OnDeath.RemoveDynamic(this, &UPlayerUI::HandlePlayerDeath);
		BoundPlayer->OnRespawn.RemoveDynamic(this, &UPlayerUI::HandlePlayerRespawn);
		BoundPlayer = nullptr;
	}

	Super::NativeDestruct();
}

void UPlayerUI::HandlePlayerDeath()
{
	SetDieTextVisible(true);
}

void UPlayerUI::HandlePlayerRespawn()
{
	SetDieTextVisible(false);
}

void UPlayerUI::SetDieTextVisible(bool bVisible)
{
	if (DieText)
	{
		DieText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
