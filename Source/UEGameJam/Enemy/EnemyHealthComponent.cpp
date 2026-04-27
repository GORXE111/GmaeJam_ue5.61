// ===================================================
// 文件：EnemyHealthComponent.cpp
// ===================================================

#include "EnemyHealthComponent.h"

UEnemyHealthComponent::UEnemyHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
}

float UEnemyHealthComponent::ApplyDamage(float Amount, AActor* /*Instigator*/)
{
	if (bInvulnerable || IsDead() || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Before = CurrentHP;
	CurrentHP = FMath::Max(0.f, CurrentHP - Amount);
	const float Delta = CurrentHP - Before;

	OnHealthChanged.Broadcast(this, CurrentHP, Delta);

	if (CurrentHP <= 0.f)
	{
		OnDepleted.Broadcast(this);
	}

	return -Delta;
}

void UEnemyHealthComponent::SetMaxHP(float NewMax, bool bHealFully)
{
	MaxHP = FMath::Max(1.f, NewMax);
	if (bHealFully)
	{
		CurrentHP = MaxHP;
		OnHealthChanged.Broadcast(this, CurrentHP, 0.f);
	}
	else
	{
		CurrentHP = FMath::Min(CurrentHP, MaxHP);
	}
}

float UEnemyHealthComponent::GetHPRatio() const
{
	return MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f) : 0.f;
}
