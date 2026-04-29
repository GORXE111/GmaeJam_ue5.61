// ===================================================
// 文件：EnemyAIController.cpp
// 说明：AEnemyAIController 实现。构造挂 StateTreeAI/Perception，
//       OnPossess 订阅 Pawn 死亡，死亡时停 StateTree 并销毁控制器。
// ===================================================

#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Navigation/PathFollowingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// 默认配一个 Sight Config，让子类 BP 不配置也能感知玩家
	// 数值保守——BP 可以直接在 AIPerception 组件面板里覆盖参数
	UAISenseConfig_Sight* SightCfg = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightCfg->SightRadius = 2000.0f;
	SightCfg->LoseSightRadius = 2500.0f;
	SightCfg->PeripheralVisionAngleDegrees = 60.0f;
	SightCfg->SetMaxAge(5.0f);
	SightCfg->DetectionByAffiliation.bDetectEnemies = true;
	SightCfg->DetectionByAffiliation.bDetectNeutrals = true;
	SightCfg->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerception->ConfigureSense(*SightCfg);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());

	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::HandlePerceptionUpdated);
	AIPerception->OnTargetPerceptionForgotten.AddDynamic(this, &AEnemyAIController::HandlePerceptionForgotten);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn))
	{
		Enemy->OnEnemyDied.AddUniqueDynamic(this, &AEnemyAIController::HandlePawnDied);
	}
}

void AEnemyAIController::HandlePawnDied(AEnemyCharacter* /*DeadEnemy*/)
{
	// 停止移动
	if (UPathFollowingComponent* Path = GetPathFollowingComponent())
	{
		Path->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
	}

	// 停 StateTree
	if (StateTreeAI)
	{
		StateTreeAI->StopLogic(FString(TEXT("PawnDied")));
	}

	ClearCurrentTarget();

	// Unpossess + 销毁控制器（Pawn 本身由 AEnemyCharacter 的定时器延时销毁）
	UnPossess();
	Destroy();
}

void AEnemyAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	OnEnemyPerceptionUpdated.ExecuteIfBound(Actor, Stimulus);
}

void AEnemyAIController::HandlePerceptionForgotten(AActor* Actor)
{
	OnEnemyPerceptionForgotten.ExecuteIfBound(Actor);
}
