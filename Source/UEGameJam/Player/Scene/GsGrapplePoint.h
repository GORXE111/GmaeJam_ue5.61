// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GsGrapplePoint.generated.h"

class USceneComponent;
class USphereComponent;
class UPrimitiveComponent;
class UWidgetComponent;
class AGsPlayer;
class UGsGrapplePointUI;

/**
 * 可放置在场景中的钩爪点
 */
UCLASS(Blueprintable)
class UEGAMEJAM_API AGsGrapplePoint : public AActor
{
	GENERATED_BODY()

	/** 钩爪点根组件，用于承载检测范围和场景 UI */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** 玩家靠近检测范围，进入范围后切换钩爪点 UI 的 Image 状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ProximitySphere;

	/** 显示在场景中的钩爪点 UI 组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> GrappleWidgetComponent;

protected:
	/** 场景 UI 相对钩爪点向上的偏移高度，单位为厘米 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grapple Point|UI", meta = (Units = "cm"))
	float WidgetHeightOffset = 20.0f;

	/** 钩爪点场景 UI 蓝图类，需要继承 UGsGrapplePointUI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grapple Point|UI")
	TSubclassOf<UGsGrapplePointUI> GrapplePointUIClass;

public:
	AGsGrapplePoint();

	bool IsPlayerNearbyFor(const AGsPlayer* Player) const;
	FVector GetGrappleTargetLocation() const;
	float GetGrappleProximityRadius() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGsGrapplePointUI> GrapplePointUI;

	UPROPERTY(Transient)
	TObjectPtr<AGsPlayer> NearbyPlayer;

	bool bIsPlayerNearby = false;

	UFUNCTION()
	void HandleProximityBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProximityEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ApplyConfigToComponents();
	void CacheGrapplePointUI();
	void RefreshNearbyPlayerFromCurrentOverlaps();
	void SetPlayerNearby(bool bNearby, AGsPlayer* InPlayer);
};
