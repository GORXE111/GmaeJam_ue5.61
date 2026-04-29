// ===================================================
// 文件：RealmTagComponent.h
// 说明：里/表世界标签组件。挂在物体上声明它属于哪个世界：
//         - Surface：表世界物体，圈内隐藏碰撞
//         - Realm  ：里世界物体，圈外隐藏碰撞
//       渲染裁剪由材质（Realm Reveal Mask 节点）负责，本组件只
//       管碰撞同步。每帧读取 URealmRevealerComponent 当前胜者
//       的位置/半径，与 Owner 距离比较切换碰撞。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RealmTagComponent.generated.h"

UENUM(BlueprintType)
enum class ERealmType : uint8
{
	Surface UMETA(DisplayName="表世界"),
	Realm   UMETA(DisplayName="里世界"),
};

UCLASS(ClassGroup=(Realm), meta=(BlueprintSpawnableComponent))
class UEGAMEJAM_API URealmTagComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URealmTagComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 运行时设置本物体的世界归属；也可在编辑器细节面板直接改。 */
	UFUNCTION(BlueprintCallable, Category="Realm")
	void SetRealmType(ERealmType NewType) { RealmType = NewType; }

	/** 读取本物体的世界归属 */
	UFUNCTION(BlueprintPure, Category="Realm")
	ERealmType GetRealmType() const { return RealmType; }

protected:
	/** 物体属于表世界还是里世界 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Realm")
	ERealmType RealmType = ERealmType::Realm;

private:
	bool bCurrentlyVisibleInRealm = false;

	/** 应用碰撞开关到 Owner */
	void ApplyCollisionState(bool bShouldHaveCollision);
};
