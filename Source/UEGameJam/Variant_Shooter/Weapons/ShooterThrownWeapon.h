// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterThrownWeapon.generated.h"

class AController;
class UDamageType;
class UProjectileMovementComponent;
class USkeletalMeshComponent;
class USphereComponent;

/**
 *  Weapon actor thrown after its magazine is empty.
 */
UCLASS()
class UEGAMEJAM_API AShooterThrownWeapon : public AActor
{
	GENERATED_BODY()

	/** 投掷武器的碰撞体，用来检测飞行中的命中 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** 投掷武器在飞行中显示的模型 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;

	/** 投掷武器的飞行移动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

protected:

	/** 投掷武器命中检测的球形半径，数值越大越容易命中目标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Thrown Weapon", meta = (ClampMin = 0, Units = "cm"))
	float CollisionRadius = 20.0f;

	/** 武器被扔出时的飞行速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Thrown Weapon", meta = (ClampMin = 0, Units = "cm/s"))
	float ThrowSpeed = 3000.0f;

	/** 投掷武器飞行时每秒旋转的角度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Thrown Weapon", meta = (Units = "deg/s"))
	FRotator SpinRate = FRotator(720.0f, 0.0f, 0.0f);

	/** 投掷武器的重力缩放，0表示直线飞行 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Thrown Weapon", meta = (ClampMin = 0))
	float GravityScale = 0.0f;

	/** 投掷武器没有命中时多久后自动销毁，0表示不自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Thrown Weapon", meta = (ClampMin = 0, Units = "s"))
	float LifeSpanAfterThrow = 5.0f;

	/** Damage to apply when this thrown weapon hits. */
	float ThrowDamage = 0.0f;

	/** Damage type to use when this thrown weapon hits. */
	TSubclassOf<UDamageType> ThrowDamageType;

	/** Push strength to apply to NPCs when this thrown weapon hits. */
	float PushStrength = 0.0f;

	/** Controller credited as the damage instigator. */
	TWeakObjectPtr<AController> DamageInstigator;

	/** Actor credited as the damage causer. */
	TWeakObjectPtr<AActor> DamageCauser;

	/** Actor that threw the weapon and should be ignored. */
	TWeakObjectPtr<AActor> Thrower;

	/** True after the first valid hit has been processed. */
	bool bHasHit = false;

public:

	/** Constructor */
	AShooterThrownWeapon();

	/** Copies the source weapon mesh and configures damage data. */
	void InitializeThrownWeapon(const USkeletalMeshComponent* SourceWeaponMesh, float InDamage, TSubclassOf<UDamageType> InDamageType, float InPushStrength, AController* InDamageInstigator, AActor* InDamageCauser);

protected:

	/** Updates editable component values in editor and at spawn. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Spins the visible weapon mesh while flying. */
	virtual void Tick(float DeltaSeconds) override;

	/** Handles collision. */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Applies damage and kick-style push to the hit actor. */
	void ProcessHit(AActor* HitActor);
};
