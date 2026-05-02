// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Skill/GsSkillBigBall.h"

#include "Components/SphereComponent.h"
#include "RealmRevealerComponent.h"

AGsSkillBigBall::AGsSkillBigBall()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	RealmRevealerComponent = CreateDefaultSubobject<URealmRevealerComponent>(TEXT("RealmRevealerComponent"));
}

void AGsSkillBigBall::BeginPlay()
{
	Super::BeginPlay();

	CurrentGrowTime = 0.0f;

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(CollisionRadius, true);
	}

	SetActorScale3D(GrowDuration <= 0.0f ? TargetActorScale : InitialActorScale);

	if (RealmRevealerComponent)
	{
		RealmRevealerComponent->SetEnabled(true);
	}

	if (DestroyDelay > 0.0f)
	{
		SetLifeSpan(DestroyDelay);
	}
}

void AGsSkillBigBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GrowDuration <= 0.0f || CurrentGrowTime >= GrowDuration)
	{
		return;
	}

	CurrentGrowTime = FMath::Min(CurrentGrowTime + DeltaSeconds, GrowDuration);
	const float GrowAlpha = CurrentGrowTime / GrowDuration;
	SetActorScale3D(FMath::Lerp(InitialActorScale, TargetActorScale, GrowAlpha));
}
