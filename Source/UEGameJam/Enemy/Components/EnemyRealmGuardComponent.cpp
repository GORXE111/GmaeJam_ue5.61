// ===================================================
// 文件：EnemyRealmGuardComponent.cpp
// 说明：UEnemyRealmGuardComponent 的实现。
// ===================================================

#include "EnemyRealmGuardComponent.h"
#include "RealmTagComponent.h"
#include "GameFramework/Actor.h"

UEnemyRealmGuardComponent::UEnemyRealmGuardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyRealmGuardComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedTag = Owner->FindComponentByClass<URealmTagComponent>();
	}
}

void UEnemyRealmGuardComponent::FreezeRealmTag()
{
	if (CachedTag)
	{
		CachedTag->SetComponentTickEnabled(false);
		CachedTag->SetAutoActivate(false);
	}
}
