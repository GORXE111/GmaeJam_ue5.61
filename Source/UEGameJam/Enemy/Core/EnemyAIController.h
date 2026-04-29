// ===================================================
// 文件：EnemyAIController.h
// 说明：Enemy 模块 AI 控制器基类。持有 StateTreeAIComponent
//       与 AIPerceptionComponent，桥接感知事件供 StateTree Task
//       订阅；管理当前目标（StateTree Condition 可读）。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class UStateTreeAIComponent;
class UAIPerceptionComponent;
class AEnemyCharacter;
struct FAIStimulus;

/** StateTree Task 可 Bind 的非动态委托：感知更新与遗忘。用非动态以便 Lambda+Weak Context 模式 */
DECLARE_DELEGATE_TwoParams(FEnemyPerceptionUpdatedDelegate, AActor*, const FAIStimulus&);
DECLARE_DELEGATE_OneParam(FEnemyPerceptionForgottenDelegate, AActor*);

/**
 *  敌人 AI 控制器基类。子类通常只需要在蓝图里设 StateTree 资产、
 *  配置 Sight/Hearing 感知参数；C++ 侧的行为由本基类和 StateTree Task 驱动。
 */
UCLASS(abstract)
class UEGAMEJAM_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	/** 设置当前目标（一般由 FEST_SenseTargets 调用） */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetCurrentTarget(AActor* InTarget) { CurrentTarget = InTarget; }

	/** 读取当前目标 */
	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	/** 清除当前目标 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void ClearCurrentTarget() { CurrentTarget = nullptr; }

	/** StateTree Task 订阅这两个委托可以拿到感知事件 */
	FEnemyPerceptionUpdatedDelegate OnEnemyPerceptionUpdated;
	FEnemyPerceptionForgottenDelegate OnEnemyPerceptionForgotten;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	/** 当控制的敌人死亡时调用 */
	UFUNCTION()
	void HandlePawnDied(AEnemyCharacter* DeadEnemy);

	/** AI Perception 感知更新回调，转发到非动态委托 */
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** AI Perception 感知遗忘回调，转发到非动态委托 */
	UFUNCTION()
	void HandlePerceptionForgotten(AActor* Actor);

private:
	/** 组件：StateTree AI；StateTree 资产在子类蓝图或 CDO 里配置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	/** 组件：AI 感知；Sight/Hearing 等配置在子类 BP 里设 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	/** 当前目标 Actor */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;
};
