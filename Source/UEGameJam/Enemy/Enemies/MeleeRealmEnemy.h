// ===================================================
// 文件：MeleeRealmEnemy.h
// 说明：里世界近战兵。在表世界只有残影（URealmTagComponent
//       Realm 模式下圈外关碰撞——打不到）；玩家打开里世界
//       球体罩住后才会变得可攻击。
//
//       行为：站桩或小范围巡逻 → 感知 Player → NavMesh 追击
//       → 攻击前摇 → 突进斩。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "MeleeRealmEnemy.generated.h"

UCLASS()
class UEGAMEJAM_API AMeleeRealmEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMeleeRealmEnemy();
};
