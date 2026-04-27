// ===================================================
// 文件：MeleeEnemy.cpp
// ===================================================

#include "MeleeEnemy.h"
#include "MeleeEnemyDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AMeleeEnemy::AMeleeEnemy()
{
	DefaultRealmType = ERealmType::Realm;

	MeleeHitbox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(GetMesh()); // 默认挂骨骼；socket 在 BeginPlay 生效
	MeleeHitbox->InitCapsuleSize(40.f, 60.f);
	MeleeHitbox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	MeleeHitbox->SetGenerateOverlapEvents(true);
	MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 默认禁用
}

void AMeleeEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (!MeleeHitboxAttachSocket.IsNone() && GetMesh()->DoesSocketExist(MeleeHitboxAttachSocket))
	{
		MeleeHitbox->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, MeleeHitboxAttachSocket);
	}

	MeleeHitbox->OnComponentBeginOverlap.AddDynamic(this, &AMeleeEnemy::OnMeleeHitboxBeginOverlap);
}

void AMeleeEnemy::ApplyDataAsset()
{
	Super::ApplyDataAsset();
	if (UMeleeEnemyDataAsset* Data = Cast<UMeleeEnemyDataAsset>(EnemyData))
	{
		CachedMeleeDamage = Data->MeleeDamage;
	}
}

void AMeleeEnemy::PerformDash(const FVector& Direction)
{
	if (IsDead())
	{
		return;
	}
	FVector Dir = Direction;
	Dir.Z = 0.f;
	if (Dir.IsNearlyZero())
	{
		Dir = GetActorForwardVector();
	}
	Dir = Dir.GetSafeNormal();

	float Impulse = 900.f;
	if (const UMeleeEnemyDataAsset* Data = Cast<UMeleeEnemyDataAsset>(EnemyData))
	{
		Impulse = Data->DashImpulse;
	}
	LaunchCharacter(Dir * Impulse, /*bXYOverride=*/true, /*bZOverride=*/false);
}

void AMeleeEnemy::SetMeleeHitboxActive(bool bActive)
{
	if (!MeleeHitbox)
	{
		return;
	}
	if (bActive)
	{
		AlreadyHitActors.Reset();
		MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMeleeEnemy::OnMeleeHitboxBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
                                            UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
                                            bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!OtherActor || OtherActor == this || IsDead())
	{
		return;
	}
	if (AlreadyHitActors.Contains(OtherActor))
	{
		return;
	}
	// 只命中带 Player Tag 的 Actor
	if (!OtherActor->ActorHasTag(FName("Player")))
	{
		return;
	}

	AlreadyHitActors.Add(OtherActor);
	UGameplayStatics::ApplyDamage(OtherActor, CachedMeleeDamage, GetController(), this, MeleeDamageType);
}
