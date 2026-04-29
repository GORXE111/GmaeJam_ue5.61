// ===================================================
// 文件：EnemyHealthComponent.cpp
// 说明：UEnemyHealthComponent 的实现。
// ===================================================

#include "EnemyHealthComponent.h"
#include "EnemyCoreExposureComponent.h"
#include "EnemyDebug.h"
#include "GameFramework/Actor.h"

UEnemyHealthComponent::UEnemyHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;

	// 非重装敌人没有这个组件，查不到是正常情况
	if (AActor* Owner = GetOwner())
	{
		CachedCoreExposure = Owner->FindComponentByClass<UEnemyCoreExposureComponent>();
	}

	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
}

float UEnemyHealthComponent::ApplyDamage(float InDamage, AActor* /*Causer*/)
{
	if (InDamage <= 0.0f || CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	const float Before = CurrentHP;

	if (bOneShotKillRule)
	{
		// 任何正值伤害都一刀秒
		CurrentHP = 0.0f;
	}
	else if (CachedCoreExposure && CachedCoreExposure->ShouldOneShotKillNow())
	{
		// 重装敌人在里世界圈内：核心暴露，一刀秒
		CurrentHP = 0.0f;
	}
	else
	{
		// 多段血量：每次扣 1 点
		CurrentHP = FMath::Max(0.0f, CurrentHP - 1.0f);
	}

	const float DamageDealt = Before - CurrentHP;
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);

	UE_LOG(LogEnemyModule, Verbose, TEXT("HP %s: %.1f -> %.1f (dmg=%.1f)"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		Before, CurrentHP, DamageDealt);

	return DamageDealt;
}
