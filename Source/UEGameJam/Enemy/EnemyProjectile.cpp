// ===================================================
// 文件：EnemyProjectile.cpp
// ===================================================

#include "EnemyProjectile.h"
#include "RealmTagComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComp->InitSphereRadius(8.f);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnHit);
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Trail"));
	TrailFX->SetupAttachment(CollisionComp);
	TrailFX->bAutoActivate = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 12000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	RealmTag = CreateDefaultSubobject<URealmTagComponent>(TEXT("RealmTag"));

	InitialLifeSpan = 0.f; // 由 InitializeAndLaunch 或 BeginPlay 控制
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime);
	}
}

void AEnemyProjectile::InitializeAndLaunch(const FVector& Direction, float Speed, AActor* InInstigator, ERealmType InRealm)
{
	SetInstigator(Cast<APawn>(InInstigator));
	if (ProjectileMovement)
	{
		const FVector Dir = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
		ProjectileMovement->Velocity = Dir * Speed;
		ProjectileMovement->UpdateComponentVelocity();
	}

	if (RealmTag)
	{
		RealmTag->SetRealmType(InRealm);
	}

	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime);
	}
}

void AEnemyProjectile::OnHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor,
                             UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this && OtherActor != GetInstigator())
	{
		const bool bRequireTag = !PlayerTag.IsNone();
		const bool bHasTag = bRequireTag ? OtherActor->ActorHasTag(PlayerTag) : true;

		if (bHasTag)
		{
			AController* InstigatorCtrl = GetInstigatorController();
			UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorCtrl, this, DamageTypeClass);
		}
	}

	if (ImpactFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	Destroy();
}
