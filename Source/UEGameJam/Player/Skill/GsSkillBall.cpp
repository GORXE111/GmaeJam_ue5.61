// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Skill/GsSkillBall.h"

#include "RealmRevealerComponent.h"
#include "Components/SphereComponent.h"

AGsSkillBall::AGsSkillBall()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	
	RealmRevealerComponent = CreateDefaultSubobject<URealmRevealerComponent>(TEXT("RealmRevealerComponent"));
}

void AGsSkillBall::BeginPlay()
{
	Super::BeginPlay();

	if (DestroyDelay > 0.0f)
	{
		SetLifeSpan(DestroyDelay);
	}
}

void AGsSkillBall::InitializeSkillBall(const FVector& InTargetLocation)
{
	TargetLocation = InTargetLocation;
	bHasTarget = true;
	bStopped = false;
}

void AGsSkillBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStopped || !bHasTarget)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = TargetLocation - CurrentLocation;
	if (ToTarget.SizeSquared() <= KINDA_SMALL_NUMBER)
	{
		bStopped = true;
		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	const float MoveDistance = MoveSpeed * DeltaSeconds;
	const bool bReachTargetThisFrame = ToTarget.SizeSquared() <= FMath::Square(MoveDistance);
	const FVector DesiredLocation = bReachTargetThisFrame
		? TargetLocation
		: CurrentLocation + (MoveDirection * MoveDistance);

	FHitResult SweepHit;
	SetActorLocation(DesiredLocation, true, &SweepHit, ETeleportType::None);

	if (SweepHit.bBlockingHit || bReachTargetThisFrame)
	{
		bStopped = true;
	}
}
