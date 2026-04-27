// ===================================================
// 文件：EnemyAIController.cpp
// ===================================================

#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "BrainComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AEnemyAIController::AEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn))
	{
		Enemy->OnEnemyDeath.AddDynamic(this, &AEnemyAIController::HandleOwnerDeath);
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
