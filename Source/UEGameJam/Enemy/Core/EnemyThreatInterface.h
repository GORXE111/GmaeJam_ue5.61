// ===================================================
// 文件：EnemyThreatInterface.h
// 说明：敌人"威胁视觉"接口。敌人瞄准玩家时会调用
//       OnThreatVisualStart，结束瞄准时调用 OnThreatVisualStop。
//       蓝图可重写这两个事件用于播放激光/充能特效。
//
//       该接口只负责"敌人自身"的视觉。全局威胁指示器（玩家
//       屏幕 UI 上的弧形警告）由 UEnemyThreatBroker 提供广播，
//       不通过本接口。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EnemyThreatInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UEnemyThreatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *  敌人威胁视觉接口。让敌人在瞄准玩家期间有可视反馈。
 */
class UEGAMEJAM_API IEnemyThreatInterface
{
	GENERATED_BODY()

public:
	/** 开始瞄准目标。LockSeconds 为预计锁定时长，供 UI/动画插值 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Threat")
	void OnThreatVisualStart(AActor* Target, float LockSeconds);

	/** 结束瞄准（被打断、开火完成或目标丢失） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Threat")
	void OnThreatVisualStop();
};
