// ===================================================
// 文件：EnemyDebug.cpp
// 说明：定义 LogEnemyModule 与 CVarEnemyDebugThreat。
// ===================================================

#include "EnemyDebug.h"

DEFINE_LOG_CATEGORY(LogEnemyModule);

TAutoConsoleVariable<int32> CVarEnemyDebugThreat(
	TEXT("r.Enemy.DebugThreat"),
	0,
	TEXT("Draw debug lines for enemies' aim phase when > 0."),
	ECVF_Default);
