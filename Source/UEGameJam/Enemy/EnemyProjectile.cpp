// ===================================================
// 文件：EnemyProjectile.cpp
// ===================================================

#include "EnemyProjectile.h"
#include "RealmTagComponent.h"
#include "Player/Skill/GsSkillBigBall.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

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

	PreviousLocation = GetActorLocation();
	bHasPreviousLocation = true;
}

void AEnemyProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector CurrentLocation = GetActorLocation();
	if (bHasPreviousLocation && TryHandleRealmBoundaryBlock(PreviousLocation, CurrentLocation))
	{
		return;
	}

	PreviousLocation = CurrentLocation;
	bHasPreviousLocation = true;
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
	if (bHasPreviousLocation && TryHandleRealmBoundaryBlock(PreviousLocation, Hit.ImpactPoint))
	{
		return;
	}

	// Pawn 通道为 Overlap，因此 OnHit 只会在打到 World 几何体（墙、地、静态物）时触发。
	// 这里不再处理伤害（伤害走 OnBeginOverlap），只播命中特效并销毁。
	HandleImpactAndDestroy(Hit.ImpactPoint, Hit.ImpactNormal);
}

void AEnemyProjectile::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 /*OtherBodyIndex*/,
                                      bool /*bFromSweep*/, const FHitResult& SweepResult)
{
	// 过滤：自己 / 发射者本人 / 其它敌人（任何不带 PlayerTag 的 Pawn）一律不处理，
	// 子弹继续飞。
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	// 只接受"真正的 Pawn 身体"触发的 Overlap：玩家 Capsule 的 ObjectType 是 ECC_Pawn。
	// 玩家身上还有一个常开的 MeleeDamageCollision（攻击盒，ObjectType 不是 Pawn，
	// 从玩家前方 140cm 向外延伸 70cm），如果不过滤，子弹在还没飞出蓝球边界前就会
	// 撞上伸进球里的攻击盒，被当作"打到玩家"直接结算伤害，绕过跨界拦截。
	if (OtherComp && OtherComp->GetCollisionObjectType() != ECC_Pawn)
	{
		return;
	}

	if (bHasPreviousLocation && TryHandleRealmBoundaryBlock(PreviousLocation, GetActorLocation()))
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

bool AEnemyProjectile::GetActiveBallBoundary(FVector& OutCenter, float& OutRadius)
{
	AGsSkillBigBall* Ball = AGsSkillBigBall::GetActiveInstance();
	if (!IsValid(Ball))
	{
		return false;
	}
	const float MaxR = Ball->GetMaxRevealRadius();
	if (MaxR <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	OutCenter = Ball->GetActorLocation();
	OutRadius = MaxR;
	return true;
}

bool AEnemyProjectile::TryHandleRealmBoundaryBlock(const FVector& Start, const FVector& End)
{
	FVector Center;
	float Radius;
	if (!GetActiveBallBoundary(Center, Radius))
	{
		return false;
	}

	// 策略：只要蓝球存在，边界就固定为其设计最大半径（BaseRevealRadius），
	// 彻底忽略 Growing/Shrinking 动画期的半径抖动。这样 Start/End 两端用同一个
	// 稳定半径做内外判定，不会出现动画过程中跨界事件被吞的情况。
	const float RadSq = FMath::Square(Radius);
	const bool bStartInside = FVector::DistSquared(Start, Center) <= RadSq;
	const bool bEndInside = FVector::DistSquared(End, Center) <= RadSq;

	if (bStartInside == bEndInside)
	{
		return false;
	}

	FVector ImpactPoint = End;
	FVector ImpactNormal = (End - Start).GetSafeNormal();
	if (!FindSphereBoundaryIntersection(Start, End, Center, Radius, ImpactPoint, ImpactNormal))
	{
		const FVector FallbackDir = (End - Start).GetSafeNormal();
		ImpactPoint = Start;
		ImpactNormal = bStartInside ? FallbackDir : -FallbackDir;
	}

	HandleImpactAndDestroy(ImpactPoint, ImpactNormal);
	return true;
}

bool AEnemyProjectile::FindSphereBoundaryIntersection(const FVector& Start, const FVector& End,
	const FVector& Center, float Radius, FVector& OutImpactPoint, FVector& OutImpactNormal)
{
	const FVector Segment = End - Start;
	const float SegmentLenSq = Segment.SizeSquared();
	if (SegmentLenSq <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector StartToCenter = Start - Center;
	const float A = SegmentLenSq;
	const float B = 2.0f * FVector::DotProduct(StartToCenter, Segment);
	const float C = StartToCenter.SizeSquared() - FMath::Square(Radius);
	const float Discriminant = (B * B) - (4.0f * A * C);
	if (Discriminant < 0.0f)
	{
		return false;
	}

	const float SqrtDiscriminant = FMath::Sqrt(Discriminant);
	const float Denominator = 2.0f * A;
	const float T0 = (-B - SqrtDiscriminant) / Denominator;
	const float T1 = (-B + SqrtDiscriminant) / Denominator;

	float HitT = TNumericLimits<float>::Max();
	if (T0 >= 0.0f && T0 <= 1.0f)
	{
		HitT = T0;
	}
	if (T1 >= 0.0f && T1 <= 1.0f)
	{
		HitT = FMath::Min(HitT, T1);
	}
	if (!FMath::IsFinite(HitT) || HitT == TNumericLimits<float>::Max())
	{
		return false;
	}

	OutImpactPoint = Start + (Segment * HitT);
	OutImpactNormal = (OutImpactPoint - Center).GetSafeNormal();
	return !OutImpactNormal.IsNearlyZero();
}

void AEnemyProjectile::HandleImpactAndDestroy(const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	if (ImpactFX)
	{
		const FVector SafeNormal = ImpactNormal.IsNearlyZero() ? FVector::UpVector : ImpactNormal;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, ImpactPoint, SafeNormal.Rotation());
	}

	Destroy();
}
