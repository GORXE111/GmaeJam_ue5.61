// ===================================================
// 文件：MeleeRealmEnemy.cpp
// 说明：AMeleeRealmEnemy 实现。
// ===================================================

#include "MeleeRealmEnemy.h"
#include "RealmTagComponent.h"

AMeleeRealmEnemy::AMeleeRealmEnemy()
{
	Archetype = EEnemyArchetype::MeleeRealm;

	// 里世界敌人：表世界状态下圈外关碰撞 = 打不到（只有残影）
	if (RealmTag)
	{
		RealmTag->SetRealmType(ERealmType::Realm);
	}
}
