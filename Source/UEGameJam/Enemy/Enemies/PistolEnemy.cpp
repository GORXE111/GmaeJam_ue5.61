// ===================================================
// 文件：PistolEnemy.cpp
// 说明：APistolEnemy 实现。
// ===================================================

#include "PistolEnemy.h"
#include "RealmTagComponent.h"

APistolEnemy::APistolEnemy()
{
	// 基类 protected 成员，子类构造器内可直接赋值，供管理器按原型分类统计
	Archetype = EEnemyArchetype::PistolSurface;

	// 表世界敌人：进入里世界球体时关闭碰撞（看不见也打不到）
	if (RealmTag)
	{
		RealmTag->SetRealmType(ERealmType::Surface);
	}
}
