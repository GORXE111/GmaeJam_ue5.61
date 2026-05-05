// ===================================================
// 文件：RealmTagComponent.cpp
// 说明：URealmTagComponent 的实现。读取当前 Revealer 胜者中心和
//       半径，根据 Owner 与中心距离判断本物体当前应"存在"于哪个
//       世界，调用 SetActorEnableCollision 切换。
//       规则：
//         Surface（表）：圈内隐藏 → 关碰撞；圈外显示 → 开碰撞
//         Realm  （里）：圈内显示 → 开碰撞；圈外隐藏 → 关碰撞
// ===================================================

#include "RealmTagComponent.h"
#include "RealmRevealerComponent.h"
#include "GameFramework/Actor.h"

URealmTagComponent::URealmTagComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URealmTagComponent::BeginPlay()
{
	Super::BeginPlay();

	// 默认按 RealmType 设初始状态：表世界开碰撞，里世界关碰撞
	const bool bInitialCollision = (RealmType == ERealmType::Surface);
	ApplyCollisionState(bInitialCollision);
}

void URealmTagComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const bool bAnyActive = URealmRevealerComponent::IsAnyActive();
	bool bInsideCircle = false;

	if (bAnyActive)
	{
		const FVector Center = URealmRevealerComponent::GetActiveCenter();
		const float   Radius = URealmRevealerComponent::GetActiveRadius();
		const float   DistSq = FVector::DistSquared(Owner->GetActorLocation(), Center);
		bInsideCircle = DistSq <= (Radius * Radius);
	}

	// 圈内：Realm 显示 / Surface 隐藏；圈外反之
	const bool bShouldHaveCollision = (RealmType == ERealmType::Realm) ? bInsideCircle : !bInsideCircle;

	if (bShouldHaveCollision != bCurrentlyVisibleInRealm)
	{
		ApplyCollisionState(bShouldHaveCollision);
		bCurrentlyVisibleInRealm = bShouldHaveCollision;
	}
}

void URealmTagComponent::ApplyCollisionState(bool bShouldHaveCollision)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorEnableCollision(bShouldHaveCollision);
	}
}
