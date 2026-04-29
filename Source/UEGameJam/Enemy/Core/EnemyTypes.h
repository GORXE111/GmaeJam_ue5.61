// ===================================================
// 文件：EnemyTypes.h
// 说明：Enemy 模块的公共类型声明。包括敌人原型枚举、
//       公共常量 FName（Tag、Socket）、公共委托签名占位。
//       本文件不引用除引擎核心外的任何依赖，保证 Enemy 模块
//       可在不依赖 Variant_Shooter 等其他子模块的情况下独立构建。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

/** 敌人原型，供 AEnemyCharacter 子类自报身份，便于管理器按类型计数 */
UENUM(BlueprintType)
enum class EEnemyArchetype : uint8
{
	/** 未分类（测试/抽象占位） */
	None              UMETA(DisplayName = "未分类"),
	/** 表世界手枪兵 */
	PistolSurface     UMETA(DisplayName = "手枪兵(表世界)"),
	/** 表世界机枪兵 */
	MachineGunSurface UMETA(DisplayName = "机枪兵(表世界)"),
	/** 里世界近战兵 */
	MeleeRealm        UMETA(DisplayName = "近战兵(里世界)"),
	/** 重装兵（表里世界都可见，表世界需多刀） */
	Heavy             UMETA(DisplayName = "重装兵"),
};

/** Enemy 模块使用的公共常量 */
namespace EnemyTagNames
{
	/** 敌人 Pawn 感知玩家所使用的 Tag（与 HeroCharacter::Tags 中的 "Player" 对应） */
	static const FName Player = FName(TEXT("Player"));

	/** 敌人自身身上挂的 Tag，用于玩家武器/胜利判断快速筛选 */
	static const FName Enemy = FName(TEXT("Enemy"));
}
