// ===================================================
// 文件：RealmHurtSwitchComponent.cpp
// 说明：URealmHurtSwitchComponent 的实现。仅追踪表/里世界态，
//       不动 Owner 任何碰撞设置。
// ===================================================

#include "RealmHurtSwitchComponent.h"
#include "RealmRevealerComponent.h"

URealmHurtSwitchComponent::URealmHurtSwitchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URealmHurtSwitchComponent::BeginPlay()
{
	// 故意跳过 URealmTagComponent::BeginPlay 的 SetActorEnableCollision 调用，
	// 直接走 UActorComponent::BeginPlay。
	UActorComponent::BeginPlay();

	bHurtable = (GetRealmType() == ERealmType::Surface);
	bInitialized = true;

	OnRealmHurtStateChanged.Broadcast(bHurtable);
}

void URealmHurtSwitchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !bInitialized)
	{
		return;
	}

	bool bInsideCircle = false;
	if (URealmRevealerComponent::IsAnyActive())
	{
		const FVector Center = URealmRevealerComponent::GetActiveCenter();
		const float   Radius = URealmRevealerComponent::GetActiveRadius();
		const float   DistSq = FVector::DistSquared(Owner->GetActorLocation(), Center);
		bInsideCircle = DistSq <= (Radius * Radius);
	}

	const bool bShouldBeHurtable = (GetRealmType() == ERealmType::Realm) ? bInsideCircle : !bInsideCircle;
	if (bShouldBeHurtable != bHurtable)
	{
		bHurtable = bShouldBeHurtable;
		OnRealmHurtStateChanged.Broadcast(bHurtable);
	}
}
