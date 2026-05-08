// ===================================================
// 文件：EnemyProjectile.h
// 说明：敌人投射物。带碰撞体的飞行子弹，命中带 PlayerTag 的 Actor 时
//       调用 UGameplayStatics::ApplyDamage。子弹本身不跟随 RealmTag 的
//       碰撞开关，但会在飞行中检测揭示球边界，禁止跨越表/里世界分界。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealmTagComponent.h"
#include "EnemyProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UDamageType;
class URealmTagComponent;

UCLASS()
class UEGAMEJAM_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	/** 初始化后发射：方向/速度/发起者/世界归属 */
	UFUNCTION(BlueprintCallable, Category="Projectile")
	void InitializeAndLaunch(const FVector& Direction, float Speed, AActor* InInstigator, ERealmType InRealm);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNiagaraComponent> TrailFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<URealmTagComponent> RealmTag;

	/** 命中伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta=(ClampMin=0))
	float Damage = 10.f;

	/** 生存时间（秒），到期自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta=(ClampMin=0))
	float LifeTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	TSubclassOf<UDamageType> DamageTypeClass;

	/** 命中特效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|FX")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	/** 只命中带此 Tag 的 Actor（空字符串 = 命中任意 Character） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FName PlayerTag = FName("Player");

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	           UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

	bool TryHandleRealmBoundaryBlock(const FVector& Start, const FVector& End);
	static bool FindSphereBoundaryIntersection(const FVector& Start, const FVector& End,
		const FVector& Center, float Radius, FVector& OutImpactPoint, FVector& OutImpactNormal);
	void HandleImpactAndDestroy(const FVector& ImpactPoint, const FVector& ImpactNormal);

	/** 读取当前活跃蓝球的"稳定边界"：中心取其 Actor 位置，半径取其设计最大值（BaseRevealRadius）。
	 *  这样彻底忽略 Growing/Shrinking 期的半径动画，只要蓝球存在就按最大范围拦截。返回 false 表示
	 *  当前没有活跃蓝球。O(1) —— 通过 AGsSkillBigBall::GetActiveInstance() 直接拿到实例指针。 */
	static bool GetActiveBallBoundary(FVector& OutCenter, float& OutRadius);

	FVector PreviousLocation = FVector::ZeroVector;
	bool bHasPreviousLocation = false;
};
