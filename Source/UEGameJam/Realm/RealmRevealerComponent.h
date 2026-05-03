// ===================================================
// 文件：RealmRevealerComponent.h
// 说明：里世界揭示组件。可挂在任意 Actor 上（玩家、道具、灯笼…）。
//       多个 Revealer 同时存在时按 Priority 选胜者，同优先级取
//       离主摄像机最近的；选中的组件每帧把所在位置和半径写入
//       MPC_RealmReveal，供材质和 RealmTagComponent 共用。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RealmRevealerComponent.generated.h"

class UMaterialParameterCollection;

UCLASS(ClassGroup=(Realm), meta=(BlueprintSpawnableComponent))
class UEGAMEJAM_API URealmRevealerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URealmRevealerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 当前胜者的世界位置（无胜者返回 ZeroVector） */
	static FVector GetActiveCenter();
	/** 当前胜者的半径（无胜者返回 0） */
	static float GetActiveRadius();
	/** 当前是否有任何 Revealer 在工作 */
	static bool IsAnyActive();

	UFUNCTION(BlueprintCallable, Category="Realm")
	void SetEnabled(bool bNewEnabled) { bEnabled = bNewEnabled; }

	UFUNCTION(BlueprintCallable, Category="Realm")
	void SetRevealRadius(float NewRadius) { RevealRadius = FMath::Max(0.0f, NewRadius); }

	UFUNCTION(BlueprintPure, Category="Realm")
	float GetRevealRadius() const { return RevealRadius; }

protected:
	/** 揭示半径（厘米），默认 500 = 5 米 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm", meta=(ClampMin=0, Units="cm"))
	float RevealRadius = 500.0f;

	/** 边缘过渡宽度（厘米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm", meta=(ClampMin=0, Units="cm"))
	float EdgeSoftness = 50.0f;

	/** MPC_RealmReveal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm")
	TObjectPtr<UMaterialParameterCollection> RealmMPC;

	/** 是否启用（false 不参与竞争） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm")
	bool bEnabled = true;

	/** 多个 Revealer 时高者胜（玩家建议 10，普通道具 0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm")
	int32 Priority = 0;

private:
	static TArray<TWeakObjectPtr<URealmRevealerComponent>> ActiveRevealers;
	static uint64 LastResolvedFrame;
	static FVector CachedActiveCenter;
	static float CachedActiveRadius;
	static bool bCachedAnyActive;

	/** 选出本帧胜者并写 MPC，同帧多次调用直接返回 */
	void ResolveAndApply();
};
