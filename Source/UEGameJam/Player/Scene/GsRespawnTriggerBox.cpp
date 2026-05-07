// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Scene/GsRespawnTriggerBox.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Player/Character/GsPlayer.h"
#include "Player/Game/GsLevelStateGameState.h"

AGsRespawnTriggerBox::AGsRespawnTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerCollision"));
	RootComponent = TriggerCollision;
	TriggerCollision->InitBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerCollision->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerCollision->SetGenerateOverlapEvents(true);

	SetActorHiddenInGame(true);
}

void AGsRespawnTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerCollision)
	{
		TriggerCollision->OnComponentBeginOverlap.AddDynamic(this, &AGsRespawnTriggerBox::HandleTriggerBeginOverlap);
	}
}

void AGsRespawnTriggerBox::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	const AGsPlayer* Player = Cast<AGsPlayer>(OtherActor);
	if (!Player || Player->IsDead())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AGsLevelStateGameState* LevelState = World->GetGameState<AGsLevelStateGameState>())
	{
		LevelState->ActivateCheckpointByIndex(TargetCheckpointIndex);
	}
}
