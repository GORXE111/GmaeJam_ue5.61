// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Skill/GsSkillBall.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Player/Skill/GsSkillBigBall.h"

TWeakObjectPtr<AActor> AGsSkillBall::ActiveSkillPtr;

bool AGsSkillBall::IsAnySkillActive()
{
	return ActiveSkillPtr.IsValid();
}

void AGsSkillBall::SetActiveSkill(AActor* InActor)
{
	ActiveSkillPtr = InActor;
}

void AGsSkillBall::ClearActiveSkillIf(AActor* InActor)
{
	if (ActiveSkillPtr.Get() == InActor)
	{
		ActiveSkillPtr.Reset();
	}
}

AGsSkillBall::AGsSkillBall()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	ImpactBallClass = AGsSkillBigBall::StaticClass();
}

void AGsSkillBall::BeginPlay()
{
	Super::BeginPlay();

	SetActiveSkill(this);
	ApplyFlightBallSettings();

	if (DestroyDelay > 0.0f)
	{
		SetLifeSpan(DestroyDelay);
	}
}

void AGsSkillBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearActiveSkillIf(this);
	Super::EndPlay(EndPlayReason);
}

void AGsSkillBall::InitializeSkillBall(const FVector& InTargetLocation)
{
	TargetLocation = InTargetLocation;
	bHasTarget = true;
	bStopped = false;
	ApplyFlightBallSettings();
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
		HandleImpact(CurrentLocation);
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
		const FVector ImpactLocation = SweepHit.bBlockingHit ? SweepHit.ImpactPoint : DesiredLocation;
		HandleImpact(ImpactLocation);
	}
}

void AGsSkillBall::ApplyFlightBallSettings()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(FlightCollisionRadius, true);
	}

	SetActorScale3D(FlightActorScale);
}

void AGsSkillBall::HandleImpact(const FVector& ImpactLocation)
{
	if (bStopped)
	{
		return;
	}

	bStopped = true;

	if (UWorld* World = GetWorld())
	{
		if (ImpactBallClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			World->SpawnActor<AGsSkillBigBall>(ImpactBallClass, ImpactLocation, GetActorRotation(), SpawnParams);
		}
	}

	Destroy();
}
