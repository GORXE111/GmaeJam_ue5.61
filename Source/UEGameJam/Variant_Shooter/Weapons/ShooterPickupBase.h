// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterPickupBase.generated.h"

class AShooterCharacter;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 *  Base class for shooter pickup items.
 *  Handles item overlap discovery while child classes decide what the pickup does.
 */
UCLASS(abstract)
class UEGAMEJAM_API AShooterPickupBase : public AActor
{
	GENERATED_BODY()

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;

	/** Pickup display mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	/** Constructor */
	AShooterPickupBase();

	/** Returns true if this pickup can be manually picked up now */
	virtual bool CanManualPickup(AShooterCharacter* Character) const;

	/** Returns true if this pickup should be automatically picked up now */
	virtual bool CanAutoPickup(AShooterCharacter* Character) const;

	/** Attempts to apply this pickup to the character */
	virtual bool TryPickup(AShooterCharacter* Character, bool bForcePickup);

	/** Returns true while the pickup can be interacted with */
	bool IsPickupEnabled() const;

protected:

	/** Returns the display mesh for child classes */
	UStaticMeshComponent* GetPickupMesh() const { return Mesh; }

	/** Enables or disables this pickup */
	void SetPickupEnabled(bool bEnabled, AShooterCharacter* PickingCharacter = nullptr);

private:

	/** Handles entering the pickup area */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Handles leaving the pickup area */
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
