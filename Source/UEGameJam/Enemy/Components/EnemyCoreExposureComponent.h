// ===================================================
// 文件：EnemyCoreExposureComponent.h
// 说明：重装敌人"核心暴露"检测组件。每 Tick 读
//       URealmRevealerComponent 的静态 API，判断 Owner 是否
//       处在任意里世界球体内；在圈内视为"核心暴露"，受击时
//       一刀秒杀；圈外需要多刀。
//
//       组件本身只管判定 + 广播；不改碰撞、不改视觉（视觉由
//       美术订阅 OnCoreExposureChanged 切换 MID 参数）。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyCoreExposureComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreExposureChanged, bool, bExposed);

UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class UEGAMEJAM_API UEnemyCoreExposureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyCoreExposureComponent();

	/** 当前是否被里世界球体覆盖 */
	UFUNCTION(BlueprintPure, Category = "Enemy|CoreExposure")
	bool IsCoreExposed() const { return bCoreExposed; }

	/** 受击规则：当前是否应当一刀秒杀 */
	UFUNCTION(BlueprintPure, Category = "Enemy|CoreExposure")
	bool ShouldOneShotKillNow() const { return bCoreExposed; }

	/** 暴露状态变化广播（美术 MID / UI 订阅） */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|CoreExposure")
	FCoreExposureChanged OnCoreExposureChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool bCoreExposed = false;
};
