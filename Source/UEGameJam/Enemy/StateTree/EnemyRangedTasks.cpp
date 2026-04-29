// ===================================================
// 文件：EnemyRangedTasks.cpp
// 说明：远程敌人 StateTree Tasks 实现。
// ===================================================

#include "EnemyRangedTasks.h"
#include "EnemyCharacter.h"
#include "EnemyProjectile.h"
#include "EnemyDebug.h"
#include "StateTreeExecutionContext.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// ---------- FEST_TelegraphAim ----------

FEST_TelegraphAim::FEST_TelegraphAim()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEST_TelegraphAim::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;

	if (IsValid(Data.Character))
	{
		Data.Character->BroadcastThreatBegin(Data.Target, Data.LockSeconds);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEST_TelegraphAim::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;

#if ENABLE_DRAW_DEBUG
	if (CVarEnemyDebugThreat.GetValueOnGameThread() > 0 && IsValid(Data.Character) && IsValid(Data.Target))
	{
		if (UWorld* World = Data.Character->GetWorld())
		{
			const FVector Start = Data.Character->GetActorLocation() + FVector(0, 0, 50);
			const FVector End = Data.Target->GetActorLocation();
			DrawDebugLine(World, Start, End, FColor::Red, false, 0.0f, 0, 2.0f);
		}
	}
#endif

	return (Data.Elapsed >= Data.LockSeconds) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEST_TelegraphAim::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (IsValid(Data.Character))
	{
		Data.Character->BroadcastThreatEnd();
	}
}

// ---------- FEST_SpawnProjectile ----------

FEST_SpawnProjectile::FEST_SpawnProjectile()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FEST_SpawnProjectile::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!IsValid(Data.Character) || !Data.ProjectileClass)
	{
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = Data.Character->GetWorld();
	if (!World)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 计算发射位置
	FVector SpawnLocation;
	if (Data.MuzzleSocket != NAME_None && Data.Character->GetMesh())
	{
		SpawnLocation = Data.Character->GetMesh()->GetSocketLocation(Data.MuzzleSocket);
	}
	else
	{
		SpawnLocation = Data.Character->GetActorLocation()
			+ Data.Character->GetActorForwardVector() * Data.ForwardOffset
			+ FVector(0.0f, 0.0f, Data.VerticalOffset);
	}

	// 计算发射方向（指向目标中心）
	FVector Direction = Data.Character->GetActorForwardVector();
	if (IsValid(Data.Target))
	{
		Direction = (Data.Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
	}

	FActorSpawnParameters Params;
	Params.Owner = Data.Character;
	Params.Instigator = Data.Character;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform(Direction.Rotation(), SpawnLocation);

	if (AEnemyProjectile* Proj = World->SpawnActor<AEnemyProjectile>(Data.ProjectileClass, SpawnTransform, Params))
	{
		Proj->InitAndLaunch(Data.Character, Direction);
	}

	// 开火是瞬时动作，立刻成功，让状态机流转到 Recovery
	return EStateTreeRunStatus::Succeeded;
}

// ---------- FEST_BurstFire ----------

FEST_BurstFire::FEST_BurstFire()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEST_BurstFire::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;
	Data.ShotsFired = 0;
	// 允许进入状态后的第一帧立刻开第一枪
	Data.TimeSinceLastShot = (Data.FireRate > 0.0f) ? (1.0f / Data.FireRate) : 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEST_BurstFire::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	Data.TimeSinceLastShot += DeltaTime;

	if (!IsValid(Data.Character) || !Data.ProjectileClass)
	{
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = Data.Character->GetWorld();
	if (!World)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Interval = (Data.FireRate > 0.0f) ? (1.0f / Data.FireRate) : FLT_MAX;

	// 每跨过一个 Interval 开一枪；用 while 防低帧率漏发
	while (Data.TimeSinceLastShot >= Interval && Data.Elapsed <= Data.Duration)
	{
		Data.TimeSinceLastShot -= Interval;

		// 计算发射位置
		FVector SpawnLocation;
		if (Data.MuzzleSocket != NAME_None && Data.Character->GetMesh())
		{
			SpawnLocation = Data.Character->GetMesh()->GetSocketLocation(Data.MuzzleSocket);
		}
		else
		{
			SpawnLocation = Data.Character->GetActorLocation()
				+ Data.Character->GetActorForwardVector() * Data.ForwardOffset
				+ FVector(0.0f, 0.0f, Data.VerticalOffset);
		}

		// 基础方向指向目标，然后随机散射
		FVector BaseDir = Data.Character->GetActorForwardVector();
		if (IsValid(Data.Target))
		{
			BaseDir = (Data.Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
		}
		const FVector Direction = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(BaseDir, Data.SpreadHalfAngle);

		FActorSpawnParameters Params;
		Params.Owner = Data.Character;
		Params.Instigator = Data.Character;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform SpawnTransform(Direction.Rotation(), SpawnLocation);
		if (AEnemyProjectile* Proj = World->SpawnActor<AEnemyProjectile>(Data.ProjectileClass, SpawnTransform, Params))
		{
			Proj->InitAndLaunch(Data.Character, Direction);
			++Data.ShotsFired;
		}
	}

	return (Data.Elapsed >= Data.Duration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
