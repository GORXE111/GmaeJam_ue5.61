// ===================================================
// 文件：EnemyStateTreeCommon.cpp
// 说明：Enemy 模块通用 Tasks 与 Conditions 实现。
// ===================================================

#include "EnemyStateTreeCommon.h"
#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "EnemyCoreExposureComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

// ==================== Tasks ====================

// ---------- FEST_SenseTargets ----------

FEST_SenseTargets::FEST_SenseTargets()
{
	// 父级型 Task：子状态切换时不要反复 EnterState/ExitState（会导致委托反复绑定/解绑）
	bShouldStateChangeOnReselect = false;
	bShouldCallTick = false;
}

EStateTreeRunStatus FEST_SenseTargets::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Controller)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 感知更新：用 WeakExecutionContext 模式防悬空引用
	Data.Controller->OnEnemyPerceptionUpdated.BindLambda(
		[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor, const FAIStimulus& Stimulus)
		{
			const FStateTreeStrongExecutionContext Strong = WeakContext.MakeStrongExecutionContext();
			FInstanceDataType* Lambda = Strong.GetInstanceDataPtr<FInstanceDataType>();
			if (!Lambda || !IsValid(SensedActor) || !IsValid(Lambda->Character))
			{
				return;
			}

			if (!SensedActor->ActorHasTag(Lambda->SenseTag))
			{
				return;
			}

			// 判断直视
			bool bDirectLOS = false;
			const FVector StimulusDir = (Stimulus.StimulusLocation - Lambda->Character->GetActorLocation()).GetSafeNormal();
			const float DirDot = FVector::DotProduct(StimulusDir, Lambda->Character->GetActorForwardVector());
			const float MaxDot = FMath::Cos(FMath::DegreesToRadians(Lambda->DirectLineOfSightCone));
			if (DirDot >= MaxDot)
			{
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(Lambda->Character);
				QueryParams.AddIgnoredActor(SensedActor);

				FHitResult Hit;
				const UWorld* World = Lambda->Character->GetWorld();
				if (World)
				{
					// 未阻挡 == 有直线视野
					bDirectLOS = !World->LineTraceSingleByChannel(
						Hit,
						Lambda->Character->GetActorLocation(),
						SensedActor->GetActorLocation(),
						ECC_Visibility,
						QueryParams);
				}
			}

			if (bDirectLOS)
			{
				Lambda->Controller->SetCurrentTarget(SensedActor);
				Lambda->TargetActor = SensedActor;
				Lambda->bHasTarget = true;
				Lambda->bHasInvestigateLocation = false;
			}
			else
			{
				// 已有目标时忽略弱刺激，保持跟踪
				if (!IsValid(Lambda->TargetActor) && Stimulus.Strength > Lambda->LastStimulusStrength)
				{
					Lambda->LastStimulusStrength = Stimulus.Strength;
					Lambda->InvestigateLocation = Stimulus.StimulusLocation;
					Lambda->bHasInvestigateLocation = true;
				}
			}
		});

	// 感知遗忘
	Data.Controller->OnEnemyPerceptionForgotten.BindLambda(
		[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor)
		{
			const FStateTreeStrongExecutionContext Strong = WeakContext.MakeStrongExecutionContext();
			FInstanceDataType* Lambda = Strong.GetInstanceDataPtr<FInstanceDataType>();
			if (!Lambda)
			{
				return;
			}

			const bool bForgetTarget = (SensedActor == Lambda->TargetActor)
				|| !IsValid(Lambda->TargetActor);
			if (bForgetTarget)
			{
				Lambda->TargetActor = nullptr;
				Lambda->bHasTarget = false;
				Lambda->bHasInvestigateLocation = false;
				Lambda->LastStimulusStrength = 0.0f;
				if (Lambda->Controller)
				{
					Lambda->Controller->ClearCurrentTarget();
					Lambda->Controller->ClearFocus(EAIFocusPriority::Gameplay);
				}
			}
		});

	return EStateTreeRunStatus::Running;
}

void FEST_SenseTargets::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.Controller)
	{
		Data.Controller->OnEnemyPerceptionUpdated.Unbind();
		Data.Controller->OnEnemyPerceptionForgotten.Unbind();
	}
}

// ---------- FEST_FaceTarget ----------

FEST_FaceTarget::FEST_FaceTarget()
{
	// 父级型 Task：子状态切换时不反复 SetFocus / ClearFocus
	bShouldStateChangeOnReselect = false;
	bShouldCallTick = false;
}

EStateTreeRunStatus FEST_FaceTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.Controller && Data.Target)
	{
		Data.Controller->SetFocus(Data.Target);
	}
	return EStateTreeRunStatus::Running;
}

void FEST_FaceTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.Controller)
	{
		Data.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

// ---------- FEST_Idle ----------

FEST_Idle::FEST_Idle()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FEST_Idle::EnterState(FStateTreeExecutionContext& /*Context*/, const FStateTreeTransitionResult& /*Transition*/) const
{
	return EStateTreeRunStatus::Running;
}

// ---------- FEST_Cooldown ----------

FEST_Cooldown::FEST_Cooldown()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FEST_Cooldown::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEST_Cooldown::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	return (Data.Elapsed >= Data.Seconds) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// ==================== Conditions ====================

bool FEC_HasTarget::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const bool bResult = Data.Controller && IsValid(Data.Controller->GetCurrentTarget());
	return Data.bInvertResult ? !bResult : bResult;
}

bool FEC_InRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Character || !IsValid(Data.Target))
	{
		return Data.bInvertResult;
	}
	const float DistSq = FVector::DistSquared(Data.Character->GetActorLocation(), Data.Target->GetActorLocation());
	const bool bResult = DistSq <= (Data.MaxDistance * Data.MaxDistance);
	return Data.bInvertResult ? !bResult : bResult;
}

bool FEC_HasLineOfSight::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Character || !IsValid(Data.Target))
	{
		return !Data.bRequireLOS;
	}

	// 视锥判定
	const FVector ToTarget = (Data.Target->GetActorLocation() - Data.Character->GetActorLocation()).GetSafeNormal();
	const float FacingDot = FVector::DotProduct(ToTarget, Data.Character->GetActorForwardVector());
	const float MaxDot = FMath::Cos(FMath::DegreesToRadians(Data.ConeHalfAngle));
	if (FacingDot <= MaxDot)
	{
		return !Data.bRequireLOS;
	}

	// 多点垂直 LineTrace
	FVector Center, Extent;
	Data.Target->GetActorBounds(true, Center, Extent, false);

	const FVector Start = Data.Character->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Data.Character);
	Params.AddIgnoredActor(Data.Target);

	const UWorld* World = Data.Character->GetWorld();
	if (!World)
	{
		return !Data.bRequireLOS;
	}

	const int32 Steps = FMath::Max(1, Data.NumVerticalChecks);
	const float ZStep = (Extent.Z * 2.0f) / Steps;

	FHitResult Hit;
	for (int32 i = 0; i < Steps; ++i)
	{
		const FVector End = Center + FVector(0.0f, 0.0f, Extent.Z - ZStep * i);
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			// 未阻挡 → 有 LOS
			return Data.bRequireLOS;
		}
	}

	// 全部阻挡 → 无 LOS
	return !Data.bRequireLOS;
}

bool FEC_IsDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const bool bResult = Data.Character && Data.Character->IsDead();
	return Data.bInvertResult ? !bResult : bResult;
}

bool FEC_IsCoreExposed::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Character)
	{
		return Data.bInvertResult;
	}
	const UEnemyCoreExposureComponent* Core = Data.Character->FindComponentByClass<UEnemyCoreExposureComponent>();
	const bool bResult = Core && Core->IsCoreExposed();
	return Data.bInvertResult ? !bResult : bResult;
}
