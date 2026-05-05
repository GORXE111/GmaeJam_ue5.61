// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AudioDataAsset.generated.h"

/**
 * 音效资源
 */
UCLASS()
class UEGAMEJAM_API UAudioDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** 里世界BGM */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BGM")
	TObjectPtr<USoundBase> RealmBGM;
	
	/** 表世界BGM */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BGM")
	TObjectPtr<USoundBase> SurfaceBGM;
	
	/** BGM Intro */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BGM")
	TObjectPtr<USoundBase> IntroBGM;
	
	/** BGM Default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BGM")
	TObjectPtr<USoundBase> DefaultBGM;
};
