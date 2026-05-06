// ===================================================
// 文件：EnemyProjectile.h
// 说明：敌人投射物。带碰撞体的飞行子弹，命中带 PlayerTag 的 Actor 时
//       调用 UGameplayStatics::ApplyDamage。支持继承发射者的 RealmType，
//       这样子弹会跟随 Realm 系统自动处理跨世界碰撞。
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
};
