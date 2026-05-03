// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Scene/GsGrapplePoint.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Player/Character/GsPlayer.h"
#include "Player/UI/GsGrapplePointUI.h"

AGsGrapplePoint::AGsGrapplePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ProximitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProximitySphere"));
	ProximitySphere->SetupAttachment(SceneRoot);
	ProximitySphere->InitSphereRadius(650.0f);
	ProximitySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProximitySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProximitySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProximitySphere->SetGenerateOverlapEvents(true);

	GrappleWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("GrappleWidget"));
	GrappleWidgetComponent->SetupAttachment(SceneRoot);
	GrappleWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, WidgetHeightOffset));
	GrappleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrappleWidgetComponent->SetGenerateOverlapEvents(false);
	GrappleWidgetComponent->SetDrawSize(FVector2D(96.0f, 96.0f));
	GrappleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	GrappleWidgetComponent->SetTwoSided(true);
}

bool AGsGrapplePoint::IsPlayerNearbyFor(const AGsPlayer* Player) const
{
	return bIsPlayerNearby && NearbyPlayer.Get() == Player;
}

FVector AGsGrapplePoint::GetGrappleTargetLocation() const
{
	return GetActorLocation();
}

void AGsGrapplePoint::BeginPlay()
{
	Super::BeginPlay();

	ApplyConfigToComponents();

	if (ProximitySphere)
	{
		ProximitySphere->OnComponentBeginOverlap.AddDynamic(this, &AGsGrapplePoint::HandleProximityBeginOverlap);
		ProximitySphere->OnComponentEndOverlap.AddDynamic(this, &AGsGrapplePoint::HandleProximityEndOverlap);
	}

	CacheGrapplePointUI();
	SetPlayerNearby(false, nullptr);
	RefreshNearbyPlayerFromCurrentOverlaps();
}

void AGsGrapplePoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyConfigToComponents();
}

void AGsGrapplePoint::HandleProximityBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	AGsPlayer* Player = Cast<AGsPlayer>(OtherActor);
	if (!Player || Player->IsDead())
	{
		return;
	}

	SetPlayerNearby(true, Player);
}

void AGsGrapplePoint::HandleProximityEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;

	AGsPlayer* Player = Cast<AGsPlayer>(OtherActor);
	if (!Player || Player != NearbyPlayer)
	{
		return;
	}

	SetPlayerNearby(false, nullptr);
}

void AGsGrapplePoint::ApplyConfigToComponents()
{
	if (!GrappleWidgetComponent)
	{
		return;
	}

	GrappleWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, WidgetHeightOffset));
	if (GrapplePointUIClass)
	{
		GrappleWidgetComponent->SetWidgetClass(GrapplePointUIClass);
	}
}

void AGsGrapplePoint::CacheGrapplePointUI()
{
	GrapplePointUI = nullptr;

	if (!GrappleWidgetComponent)
	{
		return;
	}

	if (GrapplePointUIClass)
	{
		GrappleWidgetComponent->SetWidgetClass(GrapplePointUIClass);
	}

	GrappleWidgetComponent->InitWidget();
	GrapplePointUI = Cast<UGsGrapplePointUI>(GrappleWidgetComponent->GetUserWidgetObject());
}

void AGsGrapplePoint::RefreshNearbyPlayerFromCurrentOverlaps()
{
	if (!ProximitySphere)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	ProximitySphere->GetOverlappingActors(OverlappingActors, AGsPlayer::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		AGsPlayer* Player = Cast<AGsPlayer>(OverlappingActor);
		if (Player && !Player->IsDead())
		{
			SetPlayerNearby(true, Player);
			return;
		}
	}
}

void AGsGrapplePoint::SetPlayerNearby(bool bNearby, AGsPlayer* InPlayer)
{
	bIsPlayerNearby = bNearby;
	NearbyPlayer = bNearby ? InPlayer : nullptr;

	if (!GrapplePointUI)
	{
		CacheGrapplePointUI();
	}

	if (GrapplePointUI)
	{
		GrapplePointUI->SetPlayerNearby(bIsPlayerNearby);
	}
}
