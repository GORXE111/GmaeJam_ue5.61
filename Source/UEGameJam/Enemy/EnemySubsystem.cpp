// ===================================================
// 文件：EnemySubsystem.cpp
// ===================================================

#include "EnemySubsystem.h"
#include "EnemyCharacter.h"
#include "EnemyHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UEnemySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AliveEnemies.Reset();
	bAnnouncedAllDead = false;
	bHadAnyRegistration = false;
}

void UEnemySubsystem::Deinitialize()
{
	AliveEnemies.Reset();
	Super::Deinitialize();
}

UEnemySubsystem* UEnemySubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}
	UWorld* World = WorldContext->GetWorld();
	return World ? World->GetSubsystem<UEnemySubsystem>() : nullptr;
}

void UEnemySubsystem::RegisterEnemy(AEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	// 去重
	for (const TWeakObjectPtr<AEnemyCharacter>& W : AliveEnemies)
	{
		if (W.Get() == Enemy)
		{
			return;
		}
	}

	AliveEnemies.Add(Enemy);
	bHadAnyRegistration = true;
	bAnnouncedAllDead = false;

	OnEnemyRegistered.Broadcast(Enemy);
}

void UEnemySubsystem::UnregisterEnemy(AEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	int32 Removed = 0;
	for (int32 i = AliveEnemies.Num() - 1; i >= 0; --i)
	{
		if (!AliveEnemies[i].IsValid() || AliveEnemies[i].Get() == Enemy)
		{
			AliveEnemies.RemoveAtSwap(i);
			++Removed;
		}
	}

	if (Removed > 0)
	{
		OnEnemyUnregistered.Broadcast(Enemy);
		MaybeBroadcastAllDead();
	}
}

int32 UEnemySubsystem::GetAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyCharacter>& W : AliveEnemies)
	{
		AEnemyCharacter* E = W.Get();
		if (E && !E->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

int32 UEnemySubsystem::GetAliveCountByRealm(ERealmType Realm) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyCharacter>& W : AliveEnemies)
	{
		AEnemyCharacter* E = W.Get();
		if (E && !E->IsDead() && E->GetEnemyRealmType() == Realm)
		{
			++Count;
		}
	}
	return Count;
}

int32 UEnemySubsystem::GetAliveCountByClass(TSubclassOf<AEnemyCharacter> Class) const
{
	if (!Class)
	{
		return 0;
	}
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyCharacter>& W : AliveEnemies)
	{
		AEnemyCharacter* E = W.Get();
		if (E && !E->IsDead() && E->IsA(Class))
		{
			++Count;
		}
	}
	return Count;
}

bool UEnemySubsystem::AreAllEnemiesDead() const
{
	return GetAliveCount() == 0;
}

void UEnemySubsystem::GetAliveEnemies(TArray<AEnemyCharacter*>& OutEnemies) const
{
	OutEnemies.Reset();
	for (const TWeakObjectPtr<AEnemyCharacter>& W : AliveEnemies)
	{
		AEnemyCharacter* E = W.Get();
		if (E && !E->IsDead())
		{
			OutEnemies.Add(E);
		}
	}
}

void UEnemySubsystem::EnemyKillAll()
{
	TArray<AEnemyCharacter*> Snap;
	GetAliveEnemies(Snap);
	for (AEnemyCharacter* E : Snap)
	{
		if (E && !E->IsDead() && E->GetHealth())
		{
			E->GetHealth()->ApplyDamage(E->GetHealth()->MaxHP + 1.f, nullptr);
		}
	}
}

void UEnemySubsystem::EnemyDump() const
{
	const int32 Alive   = GetAliveCount();
	const int32 Surface = GetAliveCountByRealm(ERealmType::Surface);
	const int32 Realm   = GetAliveCountByRealm(ERealmType::Realm);
	UE_LOG(LogTemp, Log, TEXT("[EnemySubsystem] Alive=%d Surface=%d Realm=%d"), Alive, Surface, Realm);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Alive=%d Surface=%d Realm=%d"), Alive, Surface, Realm));
	}
}

void UEnemySubsystem::CompactAliveList()
{
	for (int32 i = AliveEnemies.Num() - 1; i >= 0; --i)
	{
		if (!AliveEnemies[i].IsValid())
		{
			AliveEnemies.RemoveAtSwap(i);
		}
	}
}

void UEnemySubsystem::MaybeBroadcastAllDead()
{
	if (bAnnouncedAllDead || !bHadAnyRegistration)
	{
		return;
	}

	if (GetAliveCount() == 0)
	{
		bAnnouncedAllDead = true;
		OnAllEnemiesDead.Broadcast();
	}
}
