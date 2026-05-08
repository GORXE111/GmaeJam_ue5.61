// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Skill/GsSkillBigBall.h"

#include "Components/SphereComponent.h"
#include "Player/Skill/GsSkillBall.h"
#include "RealmRevealerComponent.h"

TWeakObjectPtr<AGsSkillBigBall> AGsSkillBigBall::ActiveInstance = nullptr;

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

	// 从飞行小球接管"当前活跃技能"占位，让玩家在大球完整生命周期里都不能再施法。
	AGsSkillBall::SetActiveSkill(this);

	// 登记为全局活跃大球，供 Enemy 模块 O(1) 查询最大揭示半径。
	ActiveInstance = this;

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(CollisionRadius, true);
	}

	SetActorScale3D(GrowDuration <= 0.0f ? TargetActorScale : InitialActorScale);

	if (RealmRevealerComponent)
	{
		BaseRevealRadius = RealmRevealerComponent->GetRevealRadius();
		RealmRevealerComponent->SetEnabled(true);
	}

	EnterPhase(EPhase::Growing);
}

void AGsSkillBigBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AGsSkillBall::ClearActiveSkillIf(this);
	if (ActiveInstance.Get() == this)
	{
		ActiveInstance.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AGsSkillBigBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PhaseElapsed += DeltaSeconds;

	switch (Phase)
	{
	case EPhase::Growing:
	{
		const float Alpha = (GrowDuration <= 0.0f) ? 1.0f : FMath::Clamp(PhaseElapsed / GrowDuration, 0.0f, 1.0f);
		SetActorScale3D(FMath::Lerp(InitialActorScale, TargetActorScale, Alpha));
		if (Alpha >= 1.0f)
		{
			EnterPhase(EPhase::Holding);
		}
		break;
	}

	case EPhase::Holding:
	{
		if (PhaseElapsed >= HoldDuration)
		{
			EnterPhase(EPhase::Shrinking);
		}
		break;
	}

	case EPhase::Shrinking:
	{
		const float Alpha = (ShrinkDuration <= 0.0f) ? 1.0f : FMath::Clamp(PhaseElapsed / ShrinkDuration, 0.0f, 1.0f);
		SetActorScale3D(FMath::Lerp(TargetActorScale, FVector::ZeroVector, Alpha));
		if (Alpha >= 1.0f)
		{
			Destroy();
			return;
		}
		break;
	}
	}

	// 揭示半径跟随视觉缩放：以 TargetActorScale 为基准取比例
	if (RealmRevealerComponent && BaseRevealRadius > 0.0f)
	{
		const float TargetScaleRef = FMath::Max(TargetActorScale.GetAbsMax(), KINDA_SMALL_NUMBER);
		const float CurrentScaleRef = GetActorScale3D().GetAbsMax();
		RealmRevealerComponent->SetRevealRadius(BaseRevealRadius * (CurrentScaleRef / TargetScaleRef));
	}
}

void AGsSkillBigBall::EnterPhase(EPhase NewPhase)
{
	Phase = NewPhase;
	PhaseElapsed = 0.0f;
}
