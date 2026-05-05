// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GsProjectResourceSettings.generated.h"

class UAudioDataAsset;

/**
 * 项目资源配置。
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="资源配置"))
class UEGAMEJAM_API UGsProjectResourceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	/** 全局音频资源配置，用于集中指定项目使用的音效资源DataAsset */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<UAudioDataAsset> AudioDataAsset;
};
