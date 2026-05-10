// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Scene/GsSkillAimAssistPoint.h"

#include "Components/SceneComponent.h"

AGsSkillAimAssistPoint::AGsSkillAimAssistPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

FVector AGsSkillAimAssistPoint::GetSkillAimTargetLocation() const
{
	return GetActorLocation();
}
