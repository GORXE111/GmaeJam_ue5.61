// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterPickupBase.h"
#include "ShooterCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AShooterPickupBase::AShooterPickupBase()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(RootComponent);
	Sphere->SetRelativeLocation(FVector(0.0f, 0.0f, 84.0f));
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldStatic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->bFillCollisionUnderneathForNavmesh = true;
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AShooterPickupBase::OnOverlapBegin);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &AShooterPickupBase::OnOverlapEnd);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionProfileName(FName("NoCollision"));
}

bool AShooterPickupBase::CanManualPickup(AShooterCharacter* Character) const
{
	return IsValid(Character) && IsPickupEnabled();
}

bool AShooterPickupBase::CanAutoPickup(AShooterCharacter* Character) const
{
	return false;
}

bool AShooterPickupBase::TryPickup(AShooterCharacter* Character, bool bForcePickup)
{
	return false;
}

bool AShooterPickupBase::IsPickupEnabled() const
{
	return !IsHidden() && GetActorEnableCollision();
}

void AShooterPickupBase::SetPickupEnabled(bool bEnabled, AShooterCharacter* PickingCharacter)
{
	if (PickingCharacter)
	{
		PickingCharacter->UnregisterPickupCandidate(this);
	}

	SetActorHiddenInGame(!bEnabled);
	SetActorEnableCollision(bEnabled);
	SetActorTickEnabled(bEnabled);
}

void AShooterPickupBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
	{
		ShooterCharacter->RegisterPickupCandidate(this);
	}
}

void AShooterPickupBase::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
	{
		ShooterCharacter->UnregisterPickupCandidate(this);
	}
}
