// ===================================================
// 文件：EnemyManagerSubsystem.cpp
// 说明：UEnemyManagerSubsystem 的实现。
// ===================================================

#include "EnemyManagerSubsystem.h"
#include "EnemyCharacter.h"
#include "EnemyDebug.h"
#include "Engine/World.h"

bool UEnemyManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 只在实际 Game/PIE 世界创建；编辑器预览/清单世界等跳过
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Alive.Reset();
	bAnyEverRegistered = false;
}

void UEnemyManagerSubsystem::Deinitialize()
{
	Alive.Reset();
	bAnyEverRegistered = false;
	Super::Deinitialize();
}

void UEnemyManagerSubsystem::RegisterEnemy(AEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy) || Enemy->IsDead())
	{
		return;
	}

	Alive.AddUnique(Enemy);
	bAnyEverRegistered = true;

	// 绑定死亡回调；使用 AddUniqueDynamic 防重复
	Enemy->OnEnemyDied.AddUniqueDynamic(this, &UEnemyManagerSubsystem::HandleEnemyDied);

	UE_LOG(LogEnemyModule, Verbose, TEXT("RegisterEnemy(%s), AliveCount=%d"), *Enemy->GetName(), GetAliveCount());

	OnEnemyRosterChanged.Broadcast(GetAliveCount());
}

void UEnemyManagerSubsystem::UnregisterEnemy(AEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	const int32 Removed = Alive.RemoveAll([Enemy](const TWeakObjectPtr<AEnemyCharacter>& W)
	{
		return !W.IsValid() || W.Get() == Enemy;
	});

	if (Removed > 0)
	{
		OnEnemyRosterChanged.Broadcast(GetAliveCount());

		if (bAnyEverRegistered && GetAliveCount() == 0)
		{
			OnAllEnemiesCleared.Broadcast();
		}
	}

	if (IsValid(Enemy))
	{
		Enemy->OnEnemyDied.RemoveDynamic(this, &UEnemyManagerSubsystem::HandleEnemyDied);
	}
}

int32 UEnemyManagerSubsystem::GetAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyCharacter>& W : Alive)
	{
		if (W.IsValid() && !W->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

int32 UEnemyManagerSubsystem::GetAliveCountByArchetype(EEnemyArchetype ArchetypeFilter) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyCharacter>& W : Alive)
	{
		if (W.IsValid() && !W->IsDead() && W->GetArchetype() == ArchetypeFilter)
		{
			++Count;
		}
	}
	return Count;
}

TArray<AEnemyCharacter*> UEnemyManagerSubsystem::GetAllAlive() const
{
	TArray<AEnemyCharacter*> Result;
	Result.Reserve(Alive.Num());
	for (const TWeakObjectPtr<AEnemyCharacter>& W : Alive)
	{
		if (W.IsValid() && !W->IsDead())
		{
			Result.Add(W.Get());
		}
	}
	return Result;
}

void UEnemyManagerSubsystem::HandleEnemyDied(AEnemyCharacter* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	UE_LOG(LogEnemyModule, Log, TEXT("EnemyDied(%s), remaining before unregister=%d"), *DeadEnemy->GetName(), GetAliveCount());

	OnAnyEnemyDied.Broadcast(DeadEnemy);

	// 立刻把死者从花名册移除，让外部能在"最后一刀落下"时立即感知
	// 胜利，而不是等 ragdoll 消失后 3s 才广播。UnregisterEnemy 会负责
	// 触发 RosterChanged / AllCleared 并解绑委托。
	UnregisterEnemy(DeadEnemy);
}
