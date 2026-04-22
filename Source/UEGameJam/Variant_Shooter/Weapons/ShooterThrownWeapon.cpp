// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterThrownWeapon.h"
#include "AI/ShooterNPC.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AShooterThrownWeapon::AShooterThrownWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));
	CollisionComponent->SetSphereRadius(CollisionRadius);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	WeaponMesh->SetupAttachment(CollisionComponent);
	WeaponMesh->SetCollisionProfileName(FName("NoCollision"));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = ThrowSpeed;
	ProjectileMovement->MaxSpeed = ThrowSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = false;

	ThrowDamageType = UDamageType::StaticClass();
}

void AShooterThrownWeapon::InitializeThrownWeapon(const USkeletalMeshComponent* SourceWeaponMesh, float InDamage, TSubclassOf<UDamageType> InDamageType, float InPushStrength, AController* InDamageInstigator, AActor* InDamageCauser)
{
	ThrowDamage = InDamage;
	ThrowDamageType = InDamageType ? InDamageType->GetClass() : UDamageType::StaticClass();
	PushStrength = InPushStrength;
	DamageInstigator = InDamageInstigator;
	DamageCauser = InDamageCauser;
	Thrower = InDamageCauser;

	if (Thrower.IsValid())
	{
		CollisionComponent->IgnoreActorWhenMoving(Thrower.Get(), true);
	}

	if (!SourceWeaponMesh)
	{
		return;
	}

	WeaponMesh->SetSkeletalMesh(SourceWeaponMesh->GetSkeletalMeshAsset());
	WeaponMesh->SetRelativeTransform(SourceWeaponMesh->GetRelativeTransform());

	const int32 NumMaterials = SourceWeaponMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
	{
		WeaponMesh->SetMaterial(MaterialIndex, SourceWeaponMesh->GetMaterial(MaterialIndex));
	}
}

void AShooterThrownWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CollisionComponent->SetSphereRadius(CollisionRadius);
	ProjectileMovement->InitialSpeed = ThrowSpeed;
	ProjectileMovement->MaxSpeed = ThrowSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
}

void AShooterThrownWeapon::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->SetSphereRadius(CollisionRadius);
	ProjectileMovement->InitialSpeed = ThrowSpeed;
	ProjectileMovement->MaxSpeed = ThrowSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->Velocity = GetActorForwardVector() * ThrowSpeed;

	if (LifeSpanAfterThrow > 0.0f)
	{
		SetLifeSpan(LifeSpanAfterThrow);
	}
}

void AShooterThrownWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	WeaponMesh->AddLocalRotation(SpinRate * DeltaSeconds);
}

void AShooterThrownWeapon::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit || Other == Thrower.Get())
	{
		return;
	}

	bHasHit = true;
	ProjectileMovement->StopMovementImmediately();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProcessHit(Other);
	Destroy();
}

void AShooterThrownWeapon::ProcessHit(AActor* HitActor)
{
	if (!IsValid(HitActor) || HitActor == this || HitActor == Thrower.Get())
	{
		return;
	}

	if (ThrowDamage > 0.0f)
	{
		UGameplayStatics::ApplyDamage(HitActor, ThrowDamage, DamageInstigator.Get(), DamageCauser.Get(), ThrowDamageType);
	}

	AShooterNPC* HitNPC = Cast<AShooterNPC>(HitActor);
	if (!HitNPC || PushStrength <= 0.0f)
	{
		return;
	}

	const AActor* PushSource = DamageCauser.IsValid() ? DamageCauser.Get() : this;
	FVector PushDirection = HitNPC->GetActorLocation() - PushSource->GetActorLocation();
	PushDirection.Z = 0.0f;

	if (!PushDirection.Normalize())
	{
		PushDirection = GetActorForwardVector();
		PushDirection.Z = 0.0f;
		PushDirection.Normalize();
	}

	HitNPC->ApplyPush(PushDirection * PushStrength);
}
