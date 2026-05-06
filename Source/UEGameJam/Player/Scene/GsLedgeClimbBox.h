// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsLedgeClimbBox.generated.h"

class UBoxComponent;

/**
 * 可放置在平台边缘的攀爬检测盒
 */
UCLASS(Blueprintable)
class UEGAMEJAM_API AGsLedgeClimbBox : public AActor
{
	GENERATED_BODY()

	/** 平台边缘攀爬检测盒，角色空中向前接触到它时会尝试拉上平台 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> LedgeClimbCollision;
public:
	AGsLedgeClimbBox();

	float GetLedgeTopWorldZ() const;
};
