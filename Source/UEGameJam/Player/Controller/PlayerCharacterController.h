// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerCharacterController.generated.h"

class UInputMappingContext;
class UPlayerUI;

/**
 *  纯玩家侧第一人称控制器。
 */
UCLASS()
class UEGAMEJAM_API APlayerCharacterController : public APlayerController
{
	GENERATED_BODY()

public:

	APlayerCharacterController();

protected:
	/** 默认启用的输入映射，上线后可在蓝图里挂载键鼠和手柄方案 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** 角色 UI 蓝图类，用于显示生命值和受伤反馈 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player|UI")
	TSubclassOf<UPlayerUI> PlayerUIClass;

	/** 当前创建的角色 UI 实例 */
	UPROPERTY(Transient)
	TObjectPtr<UPlayerUI> PlayerUI;

protected:

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	void InitializePlayerUI(APawn* InPawn);
};
