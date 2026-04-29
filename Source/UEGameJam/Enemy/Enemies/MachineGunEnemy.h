// ===================================================
// 文件：MachineGunEnemy.h
// 说明：表世界机枪兵。循环 4-5s：预警 1.0-1.5s（多道激光汇聚）
//       → 连发 1.2-1.5s（5-8 发/秒散射）→ 冷却 1.5-2.0s。
//       刀接触一击必杀。
//
//       节奏与散射参数由 StateTree 资产（ST_MachineGunEnemy）
//       的 FEST_BurstFire Task 实例参数配置。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "MachineGunEnemy.generated.h"

UCLASS()
class UEGAMEJAM_API AMachineGunEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMachineGunEnemy();
};
