// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Scene/GsLedgeClimbBox.h"

#include "Components/BoxComponent.h"

AGsLedgeClimbBox::AGsLedgeClimbBox()
{
	PrimaryActorTick.bCanEverTick = false;

	LedgeClimbCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LedgeClimbCollision"));
	RootComponent = LedgeClimbCollision;
	LedgeClimbCollision->InitBoxExtent(FVector(100.0f, 40.0f, 40.0f));
	LedgeClimbCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LedgeClimbCollision->SetCollisionObjectType(ECC_WorldDynamic);
	LedgeClimbCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	LedgeClimbCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	LedgeClimbCollision->SetGenerateOverlapEvents(true);

	SetActorHiddenInGame(true);
}

float AGsLedgeClimbBox::GetLedgeTopWorldZ() const
{
	return LedgeClimbCollision->Bounds.Origin.Z + LedgeClimbCollision->Bounds.BoxExtent.Z;
}
