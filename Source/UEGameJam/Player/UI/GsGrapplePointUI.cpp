// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UI/GsGrapplePointUI.h"

#include "Components/Image.h"

void UGsGrapplePointUI::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyImageVisibility();
}

void UGsGrapplePointUI::SetPlayerNearby(bool bNearby)
{
	if (bIsPlayerNearby == bNearby)
	{
		ApplyImageVisibility();
		return;
	}

	bIsPlayerNearby = bNearby;
	ApplyImageVisibility();
	BP_OnNearbyStateChanged(bIsPlayerNearby);
}

void UGsGrapplePointUI::ApplyImageVisibility()
{
	if (FarImage)
	{
		FarImage->SetVisibility(bIsPlayerNearby ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}

	if (NearImage)
	{
		NearImage->SetVisibility(bIsPlayerNearby ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
