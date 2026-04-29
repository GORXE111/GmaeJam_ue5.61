// ===================================================
// 文件：EnemyCharacter.cpp
// 说明：AEnemyCharacter 基类实现。构造挂组件、处理 TakeDamage、
//       死亡流程、威胁广播门面。
// ===================================================

#include "EnemyCharacter.h"
#include "EnemyHealthComponent.h"
#include "EnemyRealmGuardComponent.h"
#include "EnemyManagerSubsystem.h"
#include "EnemyThreatBroker.h"
#include "RealmTagComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 标记自身为敌人，供玩家武器 / 胜利判断 / 威胁指示器快速筛选
	Tags.AddUnique(EnemyTagNames::Enemy);

	// 挂公共组件。默认 RealmType = Surface（表世界），近战里世界子类改为 Realm
	RealmTag = CreateDefaultSubobject<URealmTagComponent>(TEXT("RealmTag"));
	HealthComponent = CreateDefaultSubobject<UEnemyHealthComponent>(TEXT("HealthComponent"));
	RealmGuard = CreateDefaultSubobject<UEnemyRealmGuardComponent>(TEXT("RealmGuard"));
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredDestroyHandle);

		if (UEnemyManagerSubsystem* Manager = World->GetSubsystem<UEnemyManagerSubsystem>())
		{
			Manager->UnregisterEnemy(this);
		}

		if (UEnemyThreatBroker* Broker = World->GetSubsystem<UEnemyThreatBroker>())
		{
			Broker->EndThreat(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UEnemyManagerSubsystem* Manager = World->GetSubsystem<UEnemyManagerSubsystem>())
		{
			Manager->RegisterEnemy(this);
		}
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f && HealthComponent)
	{
		HealthComponent->ApplyDamage(ActualDamage, DamageCauser);

		if (HealthComponent->IsDepleted())
		{
			HandleDeath(DamageCauser);
		}
	}

	return ActualDamage;
}

void AEnemyCharacter::ApplyPush(const FVector& PushVelocity)
{
	if (PushVelocity.IsNearlyZero())
	{
		return;
	}

	if (!bIsDead)
	{
		LaunchCharacter(PushVelocity, true, false);
		return;
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (SkelMesh->IsSimulatingPhysics())
		{
			SkelMesh->AddImpulse(PushVelocity, NAME_None, true);
		}
	}
}

void AEnemyCharacter::HandleDeath(AActor* /*Killer*/)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	// 关键顺序：先冻结 RealmTag，再关闭 Actor 碰撞；否则 RealmTag 下一帧
	// 会把 SetActorEnableCollision 翻回 true，破坏 ragdoll 表现
	if (RealmGuard)
	{
		RealmGuard->FreezeRealmTag();
	}

	// 关掉移动和 capsule 碰撞
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Mesh 启用 ragdoll
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionProfileName(RagdollCollisionProfile);
		SkelMesh->SetSimulatePhysics(true);
		SkelMesh->SetPhysicsBlendWeight(1.0f);
	}

	// 广播死亡（管理器更新计数；AI 控制器停掉 StateTree）
	OnEnemyDied.Broadcast(this);

	// 延时销毁，让 ragdoll 有时间稳定
	if (UWorld* World = GetWorld())
	{
		if (DestroyDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(DeferredDestroyHandle, this, &AEnemyCharacter::DeferredDestroy, DestroyDelay, false);
		}
		else
		{
			DeferredDestroy();
		}
	}
}

void AEnemyCharacter::DeferredDestroy()
{
	Destroy();
}

void AEnemyCharacter::BroadcastThreatBegin(AActor* Target, float LockSeconds)
{
	Execute_OnThreatVisualStart(this, Target, LockSeconds);

	if (UWorld* World = GetWorld())
	{
		if (UEnemyThreatBroker* Broker = World->GetSubsystem<UEnemyThreatBroker>())
		{
			Broker->BeginThreat(this, Target, LockSeconds);
		}
	}
}

void AEnemyCharacter::BroadcastThreatEnd()
{
	Execute_OnThreatVisualStop(this);

	if (UWorld* World = GetWorld())
	{
		if (UEnemyThreatBroker* Broker = World->GetSubsystem<UEnemyThreatBroker>())
		{
			Broker->EndThreat(this);
		}
	}
}
