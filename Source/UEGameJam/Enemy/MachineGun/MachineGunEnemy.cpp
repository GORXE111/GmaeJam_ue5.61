// ===================================================
// 文件：MachineGunEnemy.cpp
// ===================================================

#include "MachineGunEnemy.h"
#include "MachineGunEnemyDataAsset.h"
#include "EnemyProjectile.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

AMachineGunEnemy::AMachineGunEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultRealmType = ERealmType::Surface;

	MuzzleComp = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	MuzzleComp->SetupAttachment(GetMesh());

	WarningLasersFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WarningLasersFX"));
	WarningLasersFX->SetupAttachment(MuzzleComp);
	WarningLasersFX->bAutoActivate = false;
}

void AMachineGunEnemy::ApplyDataAsset()
{
	Super::ApplyDataAsset();
	if (const UMachineGunEnemyDataAsset* Data = Cast<UMachineGunEnemyDataAsset>(EnemyData))
	{
		if (WarningLasersFX && Data->WarningLasersNiagara)
		{
			WarningLasersFX->SetAsset(Data->WarningLasersNiagara);
		}
	}
}

void AMachineGunEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 预警激光持续瞄准
	if (bWarningActive && WarningLasersFX)
	{
		if (AActor* T = CurrentWarningTarget.Get())
		{
			WarningLasersFX->SetVariableVec3(FName("BeamEnd"), T->GetActorLocation());
		}
	}

	// 连发期间缓慢跟踪
	if (bTracking)
	{
		AActor* T = CurrentTrackTarget.Get();
		if (T && !IsDead())
		{
			FVector ToTarget = T->GetActorLocation() - GetActorLocation();
			ToTarget.Z = 0.f;
			if (!ToTarget.IsNearlyZero())
			{
				const FRotator Desired = ToTarget.Rotation();
				float YawRate = 45.f;
				if (const UMachineGunEnemyDataAsset* Data = Cast<UMachineGunEnemyDataAsset>(EnemyData))
				{
					YawRate = Data->TrackingYawSpeed;
				}
				const FRotator Cur = GetActorRotation();
				FRotator New = FMath::RInterpConstantTo(Cur, FRotator(0.f, Desired.Yaw, 0.f), DeltaSeconds, YawRate);
				New.Pitch = 0.f;
				New.Roll = 0.f;
				SetActorRotation(New);
			}
		}
	}
}

void AMachineGunEnemy::SetWarningLasersActive(bool bActive, AActor* AimTarget)
{
	bWarningActive = bActive;
	CurrentWarningTarget = AimTarget;

	if (!WarningLasersFX)
	{
		return;
	}

	if (bActive)
	{
		WarningLasersFX->Activate(true);
		if (AimTarget)
		{
			WarningLasersFX->SetVariableVec3(FName("BeamEnd"), AimTarget->GetActorLocation());
		}
	}
	else
	{
		WarningLasersFX->Deactivate();
	}
}

void AMachineGunEnemy::SetTrackingEnabled(bool bEnabled, AActor* TrackTarget)
{
	bTracking = bEnabled;
	CurrentTrackTarget = TrackTarget;
}

void AMachineGunEnemy::FireOneBullet(const FVector& AimLocation)
{
	if (IsDead() || !EnemyData)
	{
		return;
	}
	const UMachineGunEnemyDataAsset* Data = Cast<UMachineGunEnemyDataAsset>(EnemyData);
	if (!Data || !Data->ProjectileClass)
	{
		return;
	}

	const FVector SpawnLoc = GetMuzzleLocation();
	FVector BaseDir = AimLocation - SpawnLoc;
	if (BaseDir.IsNearlyZero())
	{
		BaseDir = GetActorForwardVector();
	}
	BaseDir = BaseDir.GetSafeNormal();

	// 扇形散射
	const float HalfAngleRad = FMath::DegreesToRadians(Data->SpreadHalfAngleDeg);
	const FVector Dir = FMath::VRandCone(BaseDir, HalfAngleRad);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyProjectile* Proj = GetWorld()->SpawnActor<AEnemyProjectile>(Data->ProjectileClass, SpawnLoc, Dir.Rotation(), Params);
	if (Proj)
	{
		Proj->InitializeAndLaunch(Dir, Data->BulletSpeed, this, GetEnemyRealmType());
	}

	if (Data->MuzzleFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Data->MuzzleFlashFX, SpawnLoc, Dir.Rotation());
	}
}

FVector AMachineGunEnemy::GetMuzzleLocation() const
{
	return MuzzleComp ? MuzzleComp->GetComponentLocation() : GetActorLocation();
}
