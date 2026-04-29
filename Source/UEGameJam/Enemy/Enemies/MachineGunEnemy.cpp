// ===================================================
// 文件：MachineGunEnemy.cpp
// 说明：AMachineGunEnemy 实现。
// ===================================================

#include "MachineGunEnemy.h"
#include "RealmTagComponent.h"

AMachineGunEnemy::AMachineGunEnemy()
{
	Archetype = EEnemyArchetype::MachineGunSurface;

	if (RealmTag)
	{
		RealmTag->SetRealmType(ERealmType::Surface);
	}
}
