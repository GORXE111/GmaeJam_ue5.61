// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Skill/GsSkillBall.h"

#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
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
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AGsSkillBall::OnCollisionComponentBeginOverlap);

	ImpactBallClass = AGsSkillBigBall::StaticClass();
}

void AGsSkillBall::BeginPlay()
{
	Super::BeginPlay();

	SetActiveSkill(this);
}

void AGsSkillBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearActiveSkillIf(this);
	Super::EndPlay(EndPlayReason);
}

void AGsSkillBall::InitializeSkillBall(const FVector& InTargetLocation)
{
	StartLocation = GetActorLocation();

	const FVector TargetDirection = InTargetLocation - StartLocation;
	FlightDirection = TargetDirection.SizeSquared() > KINDA_SMALL_NUMBER
		? TargetDirection.GetSafeNormal()
		: GetActorForwardVector().GetSafeNormal();
	if (FlightDirection.IsNearlyZero())
	{
		FlightDirection = FVector::ForwardVector;
	}

	bHasFlightDirection = true;
	bStopped = false;
}

void AGsSkillBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStopped || !bHasFlightDirection)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const float MoveDistance = MoveSpeed * DeltaSeconds;
	const FVector DesiredLocation = CurrentLocation + (FlightDirection * MoveDistance);
	if (MaxFlightDistance > 0.0f && FVector::DistSquared(StartLocation, DesiredLocation) >= FMath::Square(MaxFlightDistance))
	{
		Destroy();
		return;
	}

	FHitResult SweepHit;
	SetActorLocation(DesiredLocation, true, &SweepHit, ETeleportType::None);
	
	if (SweepHit.bBlockingHit)
	{
		HandleImpact(GetActorLocation());
	}
}

void AGsSkillBall::OnCollisionComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("SkillBall overlap: %s"), *OtherActor->GetName()));
	}
	if (bStopped || !OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	

	HandleImpact(GetActorLocation());
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
