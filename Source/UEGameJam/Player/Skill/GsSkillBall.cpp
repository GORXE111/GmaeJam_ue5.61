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
		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	const float MoveDistance = MoveSpeed * DeltaSeconds;
	const bool bReachTargetThisFrame = ToTarget.SizeSquared() <= FMath::Square(MoveDistance);
	const FVector DesiredLocation = bReachTargetThisFrame
		? TargetLocation
		: CurrentLocation + (MoveDirection * MoveDistance);

	SetActorLocation(DesiredLocation, true);
}

void AGsSkillBall::ApplyFlightBallSettings()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(FlightCollisionRadius, true);
	}

	SetActorScale3D(FlightActorScale);
}

void AGsSkillBall::OnCollisionComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bStopped || !OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("SkillBall overlap: %s"), *OtherActor->GetName()));
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
