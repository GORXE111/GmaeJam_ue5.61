// ===================================================
// 文件：RealmHurtSwitchComponent.h
// 说明：里世界"被攻击状态"切换组件。继承 URealmTagComponent，
//       但完全替换 BeginPlay/Tick 行为：不再调用父类的
//       SetActorEnableCollision（会让怪物穿地面），只**追踪**当前
//       是否处于实体态（IsHurtable），具体的"被打中"控制由所属
//       Pawn 在 TakeDamage 入口判断。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "RealmTagComponent.h"
#include "RealmHurtSwitchComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRealmHurtStateChanged, bool, bInRealm);

UCLASS(ClassGroup=(Realm), meta=(BlueprintSpawnableComponent))
class UEGAMEJAM_API URealmHurtSwitchComponent : public URealmTagComponent
{
	GENERATED_BODY()

public:
	URealmHurtSwitchComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 状态切换广播：true=进入里世界（实体态，可被打中），false=回到表世界（残影态） */
	UPROPERTY(BlueprintAssignable, Category="Realm")
	FRealmHurtStateChanged OnRealmHurtStateChanged;

	/** 当前是否处于"实体态"（可被打中） */
	UFUNCTION(BlueprintPure, Category="Realm")
	bool IsHurtable() const { return bHurtable; }

private:
	bool bHurtable = false;
	bool bInitialized = false;
};
