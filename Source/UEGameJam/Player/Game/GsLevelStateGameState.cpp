// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Game/GsLevelStateGameState.h"

#include "Kismet/GameplayStatics.h"
#include "Player/Scene/GsRespawnPoint.h"
#include "UEGameJam.h"

AGsLevelStateGameState::AGsLevelStateGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGsLevelStateGameState::BeginPlay()
{
	Super::BeginPlay();

	CacheRespawnPoints();
	ActivateInitialCheckpoint();
}

bool AGsLevelStateGameState::ActivateCheckpointByIndex(int32 CheckpointIndex)
{
	if (bHasRespawnTransform && CheckpointIndex <= CurrentCheckpointIndex)
	{
		return false;
	}

	TObjectPtr<AGsRespawnPoint>* RespawnPointPtr = RespawnPointsByIndex.Find(CheckpointIndex);
	AGsRespawnPoint* RespawnPoint = RespawnPointPtr ? RespawnPointPtr->Get() : nullptr;
	if (!IsValid(RespawnPoint))
	{
		UE_LOG(LogUEGameJam, Warning, TEXT("找不到编号为 %d 的复活点。"), CheckpointIndex);
		return false;
	}

	CurrentCheckpointIndex = CheckpointIndex;
	CurrentRespawnTransform = RespawnPoint->GetActorTransform();
	bHasRespawnTransform = true;
	return true;
}

bool AGsLevelStateGameState::GetCurrentRespawnTransform(FTransform& OutRespawnTransform) const
{
	if (!bHasRespawnTransform)
	{
		return false;
	}

	OutRespawnTransform = CurrentRespawnTransform;
	return true;
}

void AGsLevelStateGameState::EnsureFallbackRespawnTransform(const FTransform& FallbackTransform)
{
	if (bHasRespawnTransform)
	{
		return;
	}

	CurrentCheckpointIndex = INDEX_NONE;
	CurrentRespawnTransform = FallbackTransform;
	bHasRespawnTransform = true;
}

void AGsLevelStateGameState::CacheRespawnPoints()
{
	RespawnPointsByIndex.Reset();

	TArray<AActor*> RespawnPointActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGsRespawnPoint::StaticClass(), RespawnPointActors);

	for (AActor* RespawnPointActor : RespawnPointActors)
	{
		AGsRespawnPoint* RespawnPoint = Cast<AGsRespawnPoint>(RespawnPointActor);
		if (!RespawnPoint)
		{
			continue;
		}

		const int32 CheckpointIndex = RespawnPoint->GetCheckpointIndex();
		if (RespawnPointsByIndex.Contains(CheckpointIndex))
		{
			UE_LOG(
				LogUEGameJam,
				Warning,
				TEXT("复活点编号 %d 重复，保留第一个复活点 '%s'，忽略 '%s'。"),
				CheckpointIndex,
				*GetNameSafe(RespawnPointsByIndex[CheckpointIndex].Get()),
				*GetNameSafe(RespawnPoint));
			continue;
		}

		RespawnPointsByIndex.Add(CheckpointIndex, RespawnPoint);
	}
}

void AGsLevelStateGameState::ActivateInitialCheckpoint()
{
	int32 FirstCheckpointIndex = INDEX_NONE;
	bool bHasCheckpoint = false;

	for (const auto& RespawnPointPair : RespawnPointsByIndex)
	{
		if (!IsValid(RespawnPointPair.Value.Get()))
		{
			continue;
		}

		if (!bHasCheckpoint || RespawnPointPair.Key < FirstCheckpointIndex)
		{
			FirstCheckpointIndex = RespawnPointPair.Key;
			bHasCheckpoint = true;
		}
	}

	if (bHasCheckpoint)
	{
		ActivateCheckpointByIndex(FirstCheckpointIndex);
	}
}
