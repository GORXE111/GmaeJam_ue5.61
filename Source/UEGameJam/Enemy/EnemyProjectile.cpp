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
	// 让子弹"穿透"所有 Pawn（自身 + 其他敌人 + 玩家）：Pawn 通道改 Overlap，
	// 这样：墙壁等 World 物体仍走 Block → OnComponentHit 触发；Pawn 走 Overlap →
	// OnComponentBeginOverlap 触发，由我们在回调里只对带 PlayerTag 的 Actor 应用伤害，
	// 撞到其他敌人/发射者本人直接 return → 子弹继续飞。
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AEnemyProjectile::OnBeginOverlap);
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
	// 关键：禁掉 RealmTag 的 Tick，避免子弹靠近玩家（进入揭示圈）时被
	// SetActorEnableCollision(false) 关闭碰撞。子弹整个生命周期保持碰撞 ON，
	// 这样表/里世界敌人的子弹都能在任意世界命中玩家。
	RealmTag->PrimaryComponentTick.bCanEverTick = false;

	InitialLifeSpan = 0.f; // 由 InitializeAndLaunch 或 BeginPlay 控制
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 强制开启 Actor 碰撞：URealmTagComponent::BeginPlay 会按 RealmType 设初始
	// 碰撞（Realm=关 / Surface=开）。我们要的是"任何世界都能命中玩家"，所以
	// 不论发起者是表还是里世界都强制开启。配合构造函数禁 Tick，整个生命周期保持开启。
	SetActorEnableCollision(true);

	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime);
	}
}

void AEnemyProjectile::InitializeAndLaunch(const FVector& Direction, float Speed, AActor* InInstigator, ERealmType InRealm)
{
	SetInstigator(Cast<APawn>(InInstigator));

	// Pawn 通道改 Overlap 后，发射者本人不会触发 Hit，但仍会触发 BeginOverlap。
	// 在 OnBeginOverlap 里靠 OtherActor != GetInstigator() 过滤即可，无需 IgnoreActorWhenMoving。

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
	// Pawn 通道为 Overlap，因此 OnHit 只会在打到 World 几何体（墙、地、静态物）时触发。
	// 这里不再处理伤害（伤害走 OnBeginOverlap），只播命中特效并销毁。
	if (ImpactFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	Destroy();
}

void AEnemyProjectile::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
                                      UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
                                      bool /*bFromSweep*/, const FHitResult& SweepResult)
{
	// 过滤：自己 / 发射者本人 / 其它敌人（任何不带 PlayerTag 的 Pawn）一律不处理，
	// 子弹继续飞。
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	const bool bRequireTag = !PlayerTag.IsNone();
	const bool bHasTag = bRequireTag ? OtherActor->ActorHasTag(PlayerTag) : true;
	if (!bHasTag)
	{
		// 撞到其他敌人 / 杂物 → 穿透。
		return;
	}

	AController* InstigatorCtrl = GetInstigatorController();
	UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorCtrl, this, DamageTypeClass);

	if (ImpactFX)
	{
		const FVector ImpactLoc = SweepResult.bBlockingHit ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
		const FRotator ImpactRot = SweepResult.bBlockingHit ? FVector(SweepResult.ImpactNormal).Rotation() : GetActorRotation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, ImpactLoc, ImpactRot);
	}

	Destroy();
}
