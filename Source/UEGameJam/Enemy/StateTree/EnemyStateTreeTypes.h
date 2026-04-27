// ===================================================
// 文件：EnemyStateTreeTypes.h
// 说明：敌人 StateTree 共享的枚举/结构体定义。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyStateTreeTypes.generated.h"

UENUM(BlueprintType)
enum class EEnemyAttackPhase : uint8
{
	Idle     UMETA(DisplayName="空闲"),
	Lockon   UMETA(DisplayName="锁定修正"),
	Aim      UMETA(DisplayName="瞄准"),
	Warmup   UMETA(DisplayName="预警"),
	Fire     UMETA(DisplayName="开火"),
	Burst    UMETA(DisplayName="连发"),
	Recover  UMETA(DisplayName="收招"),
	Cooldown UMETA(DisplayName="冷却"),
};
