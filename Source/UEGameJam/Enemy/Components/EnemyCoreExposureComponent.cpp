// ===================================================
// 文件：EnemyCoreExposureComponent.cpp
// 说明：UEnemyCoreExposureComponent 的实现。
// ===================================================

#include "EnemyCoreExposureComponent.h"
#include "RealmRevealerComponent.h"
#include "GameFramework/Actor.h"

UEnemyCoreExposureComponent::UEnemyCoreExposureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCoreExposureComponent::BeginPlay()
{
	Super::BeginPlay();
	bCoreExposed = false;
}

void UEnemyCoreExposureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	bool bShouldExpose = false;
	if (URealmRevealerComponent::IsAnyActive())
	{
		const FVector Center = URealmRevealerComponent::GetActiveCenter();
		const float Radius = URealmRevealerComponent::GetActiveRadius();
		const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Center);
		bShouldExpose = DistSq <= (Radius * Radius);
	}

	if (bShouldExpose != bCoreExposed)
	{
		bCoreExposed = bShouldExpose;
		OnCoreExposureChanged.Broadcast(bCoreExposed);
	}
}
