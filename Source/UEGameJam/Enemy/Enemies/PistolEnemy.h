// ===================================================
// 文件：PistolEnemy.h
// 说明：表世界手枪兵。循环 2.5-3s：锁定 0.8-1.2s → 开火瞬时
//       → 强制冷却 1.5-1.8s。刀接触一击必杀。
//
//       类本身只负责身份与默认参数；具体攻击节奏由 StateTree
//       资产（ST_PistolEnemy）驱动。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "PistolEnemy.generated.h"

UCLASS()
class UEGAMEJAM_API APistolEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	APistolEnemy();
};
