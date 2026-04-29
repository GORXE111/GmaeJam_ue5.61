// ===================================================
// 文件：EnemyThreatBroker.cpp
// 说明：UEnemyThreatBroker 的实现。
// ===================================================

#include "EnemyThreatBroker.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"

bool UEnemyThreatBroker::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UEnemyThreatBroker::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Active.Reset();
}

void UEnemyThreatBroker::Deinitialize()
{
	Active.Reset();
	Super::Deinitialize();
}

int32 UEnemyThreatBroker::FindEntryIndex(AEnemyCharacter* Source) const
{
	return Active.IndexOfByPredicate([Source](const FEnemyThreatEntry& E)
	{
		return E.Source.Get() == Source;
	});
}

void UEnemyThreatBroker::BeginThreat(AEnemyCharacter* Source, AActor* Target, float LockSeconds)
{
	if (!IsValid(Source))
	{
		return;
	}

	FEnemyThreatEntry Entry;
	Entry.Source = Source;
	Entry.Target = Target;
	Entry.LockSeconds = FMath::Max(0.0f, LockSeconds);
	Entry.StartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	const int32 Existing = FindEntryIndex(Source);
	if (Existing != INDEX_NONE)
	{
		Active[Existing] = Entry;
	}
	else
	{
		Active.Add(Entry);
	}

	OnThreatBegun.Broadcast(Entry);
}

void UEnemyThreatBroker::EndThreat(AEnemyCharacter* Source)
{
	const int32 Index = FindEntryIndex(Source);
	if (Index == INDEX_NONE)
	{
		return;
	}

	Active.RemoveAt(Index);
	OnThreatEnded.Broadcast(Source);
}
