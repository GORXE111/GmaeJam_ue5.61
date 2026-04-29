// ===================================================
// 文件：HeavyEnemy.cpp
// 说明：AHeavyEnemy 实现。
// ===================================================

#include "HeavyEnemy.h"
#include "EnemyHealthComponent.h"
#include "EnemyCoreExposureComponent.h"
#include "RealmTagComponent.h"

AHeavyEnemy::AHeavyEnemy()
{
	Archetype = EEnemyArchetype::Heavy;

	// 表里世界都可见（不走 Realm 的碰撞切换），核心暴露由独立组件判定
	if (RealmTag)
	{
		RealmTag->SetRealmType(ERealmType::Surface);
	}

	// 多段血量：策划默认 3 刀
	if (HealthComponent)
	{
		HealthComponent->MaxHP = 3.0f;
		HealthComponent->bOneShotKillRule = false;
	}

	// 挂核心暴露检测组件
	CoreExposure = CreateDefaultSubobject<UEnemyCoreExposureComponent>(TEXT("CoreExposure"));
}
