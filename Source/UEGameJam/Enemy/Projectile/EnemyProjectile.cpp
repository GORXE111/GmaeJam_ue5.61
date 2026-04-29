// ===================================================
// 文件：EnemyProjectile.cpp
// 说明：AEnemyProjectile 的实现。
// ===================================================

#include "EnemyProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(8.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnSphereHit);
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = CollisionSphere;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;

	if (!DamageTypeClass)
	{
		DamageTypeClass = UDamageType::StaticClass();
	}
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (LifeSeconds > 0.0f)
	{
		SetLifeSpan(LifeSeconds);
	}
}

void AEnemyProjectile::InitAndLaunch(APawn* ProjectileOwner, const FVector& Direction)
{
	SetInstigator(ProjectileOwner);
	SetOwner(ProjectileOwner);

	const FVector Dir = Direction.GetSafeNormal();
	if (Movement)
	{
		Movement->InitialSpeed = Speed;
		Movement->MaxSpeed = Speed;
		Movement->Velocity = Dir * Speed;
	}

	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(Dir.Rotation());
	}
}

void AEnemyProjectile::OnSphereHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/, const FHitResult& Hit)
{
	// 忽略命中发射者本身（通常 Projectile 不会碰到 Instigator，但以防万一）
	if (OtherActor && OtherActor == GetInstigator())
	{
		return;
	}

	// 若命中 Pawn 则派发伤害；其他情况（如静态世界）直接销毁
	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
		UGameplayStatics::ApplyDamage(HitPawn, Damage, InstigatorController, this, DamageTypeClass);
	}

	Destroy();
}
