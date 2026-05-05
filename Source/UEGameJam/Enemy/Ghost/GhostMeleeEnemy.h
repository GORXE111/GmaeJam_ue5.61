// ===================================================
// 文件：GhostMeleeEnemy.h
// 说明：里世界近战敌人（残影 / 实体双态）。
//       继承 AMeleeEnemy，沿用其 MeleeHitbox + AttackTask + 死亡
//       流程。
//       两点改动：
//         1. PostInitializeComponents 销毁基类自带的所有
//            URealmTagComponent，新建 URealmHurtSwitchComponent
//            替代（避免基类 SetActorEnableCollision 让怪穿地）。
//         2. 重写 TakeDamage：残影态返回 0 拒绝伤害。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "MeleeEnemy.h"
#include "GhostMeleeEnemy.generated.h"

UCLASS()
class UEGAMEJAM_API AGhostMeleeEnemy : public AMeleeEnemy
{
	GENERATED_BODY()

public:
	AGhostMeleeEnemy();

	virtual void PostInitializeComponents() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;
};
