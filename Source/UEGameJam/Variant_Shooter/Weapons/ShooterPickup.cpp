// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterPickup.h"
#include "Components/StaticMeshComponent.h"
#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UEGameJam.h"

void AShooterPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshWeaponDataFromRow();
}

void AShooterPickup::BeginPlay()
{
	Super::BeginPlay();

	RefreshWeaponDataFromRow();
}

void AShooterPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

bool AShooterPickup::CanManualPickup(AShooterCharacter* Character) const
{
	return Super::CanManualPickup(Character) && WeaponClass;
}

bool AShooterPickup::CanAutoPickup(AShooterCharacter* Character) const
{
	return CanManualPickup(Character) && Character->ShouldAutoPickupWeapon();
}

bool AShooterPickup::TryPickup(AShooterCharacter* Character, bool bForcePickup)
{
	if (!CanManualPickup(Character))
	{
		return false;
	}

	if (!bForcePickup && !CanAutoPickup(Character))
	{
		return false;
	}

	TSubclassOf<AShooterWeapon> ReplacedWeaponClass;
	if (!Character->ReplaceCurrentWeaponClass(WeaponClass, ReplacedWeaponClass))
	{
		return false;
	}

	SpawnDroppedWeaponPickup(Character, ReplacedWeaponClass);
	HandlePickupConsumed(Character);

	return true;
}

void AShooterPickup::InitializeDroppedWeapon(const TSubclassOf<AShooterWeapon>& DroppedWeaponClass, const UDataTable* WeaponDataTable)
{
	bShouldRespawn = false;
	RefreshWeaponDataFromClass(DroppedWeaponClass, WeaponDataTable);
}

void AShooterPickup::RespawnPickup()
{
	// unhide this pickup
	SetActorHiddenInGame(false);

	// call the BP handler
	BP_OnRespawn();
}

void AShooterPickup::FinishRespawn()
{
	SetPickupEnabled(true);
}

void AShooterPickup::RefreshWeaponDataFromRow()
{
	if (!WeaponType.DataTable || WeaponType.RowName.IsNone())
	{
		return;
	}

	if (FWeaponTableRow* WeaponData = WeaponType.GetRow<FWeaponTableRow>(FString()))
	{
		ApplyWeaponData(*WeaponData);
	}
}

void AShooterPickup::RefreshWeaponDataFromClass(const TSubclassOf<AShooterWeapon>& InWeaponClass, const UDataTable* WeaponDataTable)
{
	WeaponClass = InWeaponClass;

	if (!WeaponDataTable)
	{
		GetPickupMesh()->SetStaticMesh(nullptr);
		UE_LOG(LogUEGameJam, Warning, TEXT("Dropped weapon pickup %s has no weapon data table."), *GetName());
		return;
	}

	WeaponType.DataTable = WeaponDataTable;

	const FString ContextString = TEXT("ShooterPickup");
	for (const FName& RowName : WeaponDataTable->GetRowNames())
	{
		FWeaponTableRow* WeaponData = WeaponDataTable->FindRow<FWeaponTableRow>(RowName, ContextString);
		if (WeaponData && WeaponData->WeaponToSpawn == InWeaponClass)
		{
			WeaponType.DataTable = WeaponDataTable;
			WeaponType.RowName = RowName;
			ApplyWeaponData(*WeaponData);
			return;
		}
	}

	GetPickupMesh()->SetStaticMesh(nullptr);
	UE_LOG(LogUEGameJam, Warning, TEXT("Could not find weapon pickup data for dropped weapon class %s."), *GetNameSafe(InWeaponClass.Get()));
}

void AShooterPickup::ApplyWeaponData(const FWeaponTableRow& WeaponData)
{
	WeaponClass = WeaponData.WeaponToSpawn;
	GetPickupMesh()->SetStaticMesh(WeaponData.StaticMesh.LoadSynchronous());
}

void AShooterPickup::HandlePickupConsumed(AShooterCharacter* Character)
{
	if (bShouldRespawn)
	{
		SetPickupEnabled(false, Character);
		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterPickup::RespawnPickup, RespawnTime, false);
		return;
	}

	if (Character)
	{
		Character->UnregisterPickupCandidate(this);
	}

	Destroy();
}

void AShooterPickup::SpawnDroppedWeaponPickup(AShooterCharacter* Character, const TSubclassOf<AShooterWeapon>& DroppedWeaponClass)
{
	if (!Character || !DroppedWeaponClass)
	{
		return;
	}

	TSubclassOf<AShooterPickup> PickupClass = DroppedPickupClass->GetClass();
	if (!PickupClass)
	{
		return;
	}

	FTransform DropTransform = GetActorTransform();
	AShooterPickup* DroppedPickup = GetWorld()->SpawnActorDeferred<AShooterPickup>(PickupClass, DropTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (DroppedPickup)
	{
		DroppedPickup->InitializeDroppedWeapon(DroppedWeaponClass, WeaponType.DataTable);
		UGameplayStatics::FinishSpawningActor(DroppedPickup, DropTransform);
	}
}
