// ===================================================
// 文件：EnemyAIController.cpp
// ===================================================

#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "EnemyDataAsset.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"
#include "BrainComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AEnemyAIController::AEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	// 推迟到 OnPossess 里等 Pawn 的 DataAsset 可访问后再设 ST 资产 + 手动启动。
	// 跳过 InitializeComponent 的 ValidateStateTreeReference 自检（避免尚未 Possess 时
	// 报 "The State Tree asset is not set" Error 日志）。
	StateTreeAI->SetStartLogicAutomatically(false);
	StateTreeAI->bWantsInitializeComponent = false;

	// 需要 Tick 每帧补 AimOffset Pitch（见 Tick 注释）。
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn))
	{
		Enemy->OnEnemyDeath.AddDynamic(this, &AEnemyAIController::HandleOwnerDeath);

		// 从 Pawn 的 DataAsset 取 StateTree 资产并启动
		if (UEnemyDataAsset* Data = Enemy->EnemyData)
		{
			if (Data->StateTreeAsset && StateTreeAI)
			{
				StateTreeAI->SetStateTree(Data->StateTreeAsset);
				StateTreeAI->StartLogic();
			}
		}
	}

	RefreshPlayer();
}

void AEnemyAIController::OnUnPossess()
{
	if (StateTreeAI)
	{
		StateTreeAI->StopLogic(TEXT("UnPossess"));
	}
	Super::OnUnPossess();
}

AActor* AEnemyAIController::RefreshPlayer()
{
	CachedPlayer.Reset();
	return FindPlayerByTag();
}

void AEnemyAIController::ClearCachedPlayer()
{
	CachedPlayer.Reset();
}

AActor* AEnemyAIController::FindPlayerByTag()
{
	if (AActor* Existing = CachedPlayer.Get())
	{
		return Existing;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* P = *It;
		if (P && P->ActorHasTag(PlayerTag))
		{
			CachedPlayer = P;
			return P;
		}
	}

	return nullptr;
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Super::Tick 里 AAIController::UpdateControlRotation 会把 ControlRotation.Yaw 同步
	// 到 pawn orientation（bSetControlRotationFromPawnOrientation=true 默认开）。Pitch 不
	// 动。这里盖写 Pitch，让 ABP 的 AimOffset 能让上半身上下瞄到目标——对我们来说 target
	// 就是 StateTree 缓存的玩家；没目标就保持水平（回 0）。
	APawn* P = GetPawn();
	if (!P)
	{
		return;
	}

	FRotator R = GetControlRotation();

	if (const AActor* Target = GetCachedPlayer())
	{
		const FVector Dir = Target->GetActorLocation() - P->GetActorLocation();
		const float Dist2D = Dir.Size2D();
		if (Dist2D > KINDA_SMALL_NUMBER)
		{
			const float PitchDeg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Z, Dist2D));
			R.Pitch = FMath::ClampAngle(PitchDeg, -60.f, 60.f);
		}
		else
		{
			R.Pitch = 0.f;
		}
	}
	else
	{
		R.Pitch = 0.f;
	}

	SetControlRotation(R);
}

void AEnemyAIController::HandleOwnerDeath(AEnemyCharacter* /*DeadEnemy*/)
{
	if (StateTreeAI)
	{
		StateTreeAI->StopLogic(TEXT("OwnerDeath"));
	}

	if (UPathFollowingComponent* PathComp = GetPathFollowingComponent())
	{
		PathComp->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
	}
}
