// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "ShooterPickupBase.h"
#include "ShooterPickup.generated.h"

class AShooterCharacter;
class AShooterWeapon;
class UDataTable;

/**
 *  Holds information about a type of weapon pickup
 */
USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 拾取物在场景中显示的静态网格 */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** 拾取后授予玩家的武器类型 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AShooterWeapon> WeaponToSpawn;
};

/**
 *  Simple shooter game weapon pickup
 */
UCLASS(abstract)
class UEGAMEJAM_API AShooterPickup : public AShooterPickupBase
{
	GENERATED_BODY()
	
protected:

	/** 武器拾取物的数据表行，用来决定拾取后获得的武器和显示模型 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	FDataTableRowHandle WeaponType;

	/** Type to weapon to grant on pickup. Set from the weapon data table. */
	TSubclassOf<AShooterWeapon> WeaponClass;
	
	/** 是否在被拾取后刷新；地图固定枪支应开启，替换掉落的临时枪支会关闭 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	bool bShouldRespawn = true;

	/** 替换武器时生成旧武器掉落物所使用的拾取物类型 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	TSubclassOf<AShooterPickup> DroppedPickupClass;

	/** 被拾取后等待多久刷新，只有开启刷新时生效 */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (ClampMin = 0, ClampMax = 120, Units = "s"))
	float RespawnTime = 4.0f;

	/** Timer to respawn the pickup */
	FTimerHandle RespawnTimer;

protected:

	/** Native construction script */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Gameplay Initialization*/
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Returns true if this weapon can be manually picked up */
	virtual bool CanManualPickup(AShooterCharacter* Character) const override;

	/** Returns true if this weapon should be automatically picked up */
	virtual bool CanAutoPickup(AShooterCharacter* Character) const override;

	/** Attempts to give this weapon to the character */
	virtual bool TryPickup(AShooterCharacter* Character, bool bForcePickup) override;

public:

	/** Initializes a non-respawning dropped weapon pickup */
	void InitializeDroppedWeapon(const TSubclassOf<AShooterWeapon>& DroppedWeaponClass, const UDataTable* WeaponDataTable);

protected:

	/** Called when it's time to respawn this pickup */
	void RespawnPickup();

	/** Passes control to Blueprint to animate the pickup respawn. Should end by calling FinishRespawn */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "OnRespawn"))
	void BP_OnRespawn();

	/** Enables this pickup after respawning */
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void FinishRespawn();

	/** Refreshes weapon class and mesh from the configured row */
	void RefreshWeaponDataFromRow();

	/** Refreshes weapon class and mesh by searching a data table for the given class */
	void RefreshWeaponDataFromClass(const TSubclassOf<AShooterWeapon>& InWeaponClass, const UDataTable* WeaponDataTable);

	/** Applies data table values to this pickup */
	void ApplyWeaponData(const FWeaponTableRow& WeaponData);

	/** Hides or destroys this pickup after it has been collected */
	void HandlePickupConsumed(AShooterCharacter* Character);

	/** Spawns the old weapon as a non-respawning dropped pickup */
	void SpawnDroppedWeaponPickup(AShooterCharacter* Character, const TSubclassOf<AShooterWeapon>& DroppedWeaponClass);
};
