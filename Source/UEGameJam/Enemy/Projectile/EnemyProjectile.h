// ===================================================
// 文件：EnemyProjectile.h
// 说明：敌人共用的投射物（手枪/机枪都用它）。构造挂
//       USphereComponent + UProjectileMovementComponent +
//       UStaticMeshComponent。命中 Pawn 时用 UGameplayStatics
//       ApplyDamage，命中世界碰撞或超时则销毁自己。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UDamageType;

UCLASS()
class UEGAMEJAM_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	/**
	 *  初始化并发射。Direction 会被标准化为单位向量。
	 *  ProjectileOwner 作为 Instigator，用于伤害归因。
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Projectile")
	void InitAndLaunch(APawn* ProjectileOwner, const FVector& Direction);

protected:
	/** 球形碰撞体（根） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	/** 视觉网格，仅装饰 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 投射物运动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> Movement;

	/** 命中 Pawn 时造成的伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Projectile", meta = (ClampMin = 0))
	float Damage = 10.0f;

	/** 发射速度（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Projectile", meta = (ClampMin = 0))
	float Speed = 3500.0f;

	/** 生存时间（秒）——到期自毁 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Projectile", meta = (ClampMin = 0))
	float LifeSeconds = 4.0f;

	/** 伤害类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Projectile")
	TSubclassOf<UDamageType> DamageTypeClass;

	virtual void BeginPlay() override;

	/** 碰撞回调 */
	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
