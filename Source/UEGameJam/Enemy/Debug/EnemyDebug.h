// ===================================================
// 文件：EnemyDebug.h
// 说明：Enemy 模块共用的日志分类与调试 CVar。
//
//       日志：在 Output Log 输入 `log LogEnemyModule Verbose`
//             打开详细日志。
//
//       CVar：`r.Enemy.DebugThreat 1` 打开瞄准期间的激光
//             DrawDebugLine（方便美术没做激光特效前的盲测）。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "HAL/IConsoleManager.h"

UEGAMEJAM_API DECLARE_LOG_CATEGORY_EXTERN(LogEnemyModule, Log, All);

extern UEGAMEJAM_API TAutoConsoleVariable<int32> CVarEnemyDebugThreat;
