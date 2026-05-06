// ===================================================
// 文件：EnemyCharacter.cpp
// ===================================================

#include "EnemyCharacter.h"
#include "EnemyHealthComponent.h"
#include "EnemyDataAsset.h"
#include "EnemySubsystem.h"
#include "RealmTagComponent.h"
#include "RealmHurtSwitchComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 默认 AI 行为：不锁视角，用移动方向旋转
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	}

	// 敌人统一使用 URealmHurtSwitchComponent：仅追踪表/里世界态（IsHurtable），
	// 不调用 SetActorEnableCollision。这样表里世界敌人在任何世界都能正常移动、
	// 都能被自身攻击 Hitbox/子弹触发，伤害是否被吃由 TakeDamage 中按 RealmType 判定。
	RealmTag = CreateDefaultSubobject<URealmHurtSwitchComponent>(TEXT("RealmTag"));
	Health   = CreateDefaultSubobject<UEnemyHealthComponent>(TEXT("Health"));

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (RealmTag)
	{
		RealmTag->SetRealmType(DefaultRealmType);
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyDataAsset();

	if (Health)
	{
		Health->OnDepleted.AddDynamic(this, &AEnemyCharacter::HandleHealthDepleted);
	}

	if (UEnemySubsystem* Sub = UEnemySubsystem::Get(this))
	{
		Sub->RegisterEnemy(this);
	}
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathTimer);
	}

	// 保证任何未走完 Die() 流程的敌人（例如直接 Destroy）也能从 Subsystem 移除
	if (UEnemySubsystem* Sub = UEnemySubsystem::Get(this))
	{
		Sub->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}

float AEnemyCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
                                  AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || !Health)
	{
		return 0.f;
	}

	// 里世界敌人：只有在玩家位于里世界时（HurtSwitch 进入实体态）才接受伤害。
	// 表世界敌人：始终接受伤害（IsHurtable 不参与判定）。
	if (GetEnemyRealmType() == ERealmType::Realm)
	{
		if (const URealmHurtSwitchComponent* HurtSwitch = Cast<URealmHurtSwitchComponent>(RealmTag))
		{
			if (!HurtSwitch->IsHurtable())
			{
				return 0.f;
			}
		}
	}

	const float Applied = Health->ApplyDamage(Damage, DamageCauser);
	return Applied;
}

ERealmType AEnemyCharacter::GetEnemyRealmType() const
{
	return RealmTag ? RealmTag->GetRealmType() : ERealmType::Surface;
}

void AEnemyCharacter::ApplyDataAsset()
{
	if (!EnemyData)
	{
		return;
	}

	if (Health)
	{
		Health->SetMaxHP(EnemyData->MaxHP, /*bHealFully=*/true);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = EnemyData->WalkSpeed;
	}
}

void AEnemyCharacter::HandleHealthDepleted(UEnemyHealthComponent* /*Src*/)
{
	Die();
}

void AEnemyCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// 停 AI
	if (AController* Ctrl = GetController())
	{
		if (AAIController* AICtrl = Cast<AAIController>(Ctrl))
		{
			if (AICtrl->BrainComponent)
			{
				AICtrl->BrainComponent->StopLogic(TEXT("EnemyDeath"));
			}
		}
		Ctrl->UnPossess();
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(RagdollCollisionProfile);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetPhysicsBlendWeight(1.f);
	}

	OnEnemyDeath.Broadcast(this);

	if (UEnemySubsystem* Sub = UEnemySubsystem::Get(this))
	{
		Sub->UnregisterEnemy(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DeathTimer, this, &AEnemyCharacter::DeferredDestruction,
		                                  FMath::Max(0.01f, DeferredDestructionTime), false);
	}
}

void AEnemyCharacter::DeferredDestruction()
{
	Destroy();
}
