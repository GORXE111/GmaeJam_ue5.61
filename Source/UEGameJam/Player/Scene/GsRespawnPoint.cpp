// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Scene/GsRespawnPoint.h"

#include "Components/SceneComponent.h"

AGsRespawnPoint::AGsRespawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}
