// ===================================================
// 文件：HeavyEnemy.h
// 说明：重装敌人。表世界需要多刀斩杀，进入里世界球体后核心
//       暴露一刀秒杀。表里世界都可见（RealmType = Surface），
//       差异由 UEnemyCoreExposureComponent 独立判定。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "HeavyEnemy.generated.h"

class UEnemyCoreExposureComponent;

UCLASS()
class UEGAMEJAM_API AHeavyEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AHeavyEnemy();

protected:
	/** 核心暴露检测组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<UEnemyCoreExposureComponent> CoreExposure;
};
