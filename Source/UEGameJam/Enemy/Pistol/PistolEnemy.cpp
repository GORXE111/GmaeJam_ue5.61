// ===================================================
// 文件：PistolEnemy.cpp
// ===================================================

#include "PistolEnemy.h"
#include "PistolEnemyDataAsset.h"
#include "EnemyProjectile.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

APistolEnemy::APistolEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultRealmType = ERealmType::Surface;

	MuzzleComp = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	MuzzleComp->SetupAttachment(GetMesh());

	LaserFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LaserFX"));
	LaserFX->SetupAttachment(MuzzleComp);
	LaserFX->bAutoActivate = false;
}

void APistolEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void APistolEnemy::ApplyDataAsset()
{
	Super::ApplyDataAsset();
	if (const UPistolEnemyDataAsset* Data = Cast<UPistolEnemyDataAsset>(EnemyData))
	{
		if (LaserFX && Data->LaserNiagara)
		{
			LaserFX->SetAsset(Data->LaserNiagara);
		}
	}
}

void APistolEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bLaserActive && LaserFX)
	{
		if (AActor* T = CurrentAimTarget.Get())
		{
			LaserFX->SetVariableVec3(FName("BeamEnd"), T->GetActorLocation());
		}
	}
}

void APistolEnemy::SetLaserActive(bool bActive, AActor* AimTarget)
{
	bLaserActive = bActive;
	CurrentAimTarget = AimTarget;

	if (!LaserFX)
	{
		return;
	}

	if (bActive)
	{
		LaserFX->Activate(true);
		LaserFX->SetVariableFloat(FName("FlickerIntensity"), 0.f);
		if (AimTarget)
		{
			LaserFX->SetVariableVec3(FName("BeamEnd"), AimTarget->GetActorLocation());
		}
	}
	else
	{
		LaserFX->Deactivate();
	}
}

void APistolEnemy::SetLaserFlicker(bool bFlicker)
{
	if (LaserFX)
	{
		LaserFX->SetVariableFloat(FName("FlickerIntensity"), bFlicker ? 1.f : 0.f);
	}
}

void APistolEnemy::FireProjectile(const FVector& AimLocation)
{
	if (IsDead() || !EnemyData)
	{
		return;
	}
	const UPistolEnemyDataAsset* Data = Cast<UPistolEnemyDataAsset>(EnemyData);
	if (!Data || !Data->ProjectileClass)
	{
		return;
	}

	const FVector SpawnLoc = GetMuzzleLocation();
	FVector Dir = AimLocation - SpawnLoc;
	if (Dir.IsNearlyZero())
	{
		Dir = GetActorForwardVector();
	}
	Dir = Dir.GetSafeNormal();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyProjectile* Proj = GetWorld()->SpawnActor<AEnemyProjectile>(Data->ProjectileClass, SpawnLoc, Dir.Rotation(), Params);
	if (Proj)
	{
		Proj->InitializeAndLaunch(Dir, Data->ProjectileSpeed, this, GetEnemyRealmType());
	}

	if (Data->MuzzleFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Data->MuzzleFlashFX, SpawnLoc, Dir.Rotation());
	}
}

FVector APistolEnemy::GetMuzzleLocation() const
{
	return MuzzleComp ? MuzzleComp->GetComponentLocation() : GetActorLocation();
}
