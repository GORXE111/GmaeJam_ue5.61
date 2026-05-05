// ===================================================
// 文件：EnemyDataAsset.h
// 说明：敌人通用数值数据资产基类。每类敌人派生出自己的子类扩展
//       专属字段。基类负责 HP/感知半径/巡逻速度/StateTree 绑定等
//       通用数值，供 AEnemyCharacter::ApplyDataAsset() 应用。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDataAsset.generated.h"

class UStateTree;

UCLASS(BlueprintType, Abstract)
class UEGAMEJAM_API UEnemyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 敌人最大 HP（刀一击必杀时设 1） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin=1))
	float MaxHP = 1.f;

	/** 发现玩家所需的距离（单位 cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Perception", meta=(ClampMin=0, Units="cm"))
	float DetectionRadius = 1500.f;

	/** 失去目标后回到 Searching 的超时（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Perception", meta=(ClampMin=0))
	float LoseSightTimeout = 5.f;

	/** 默认移动速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin=0))
	float WalkSpeed = 300.f;

	/** 战斗/追击移动速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin=0))
	float ChaseSpeed = 550.f;

	/** 该敌人使用的 StateTree 资产 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="StateTree")
	TObjectPtr<UStateTree> StateTreeAsset = nullptr;
};
