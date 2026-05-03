// ===================================================
// 文件：GhostMeleeEnemy.cpp
// 说明：AGhostMeleeEnemy 的实现。
// ===================================================

#include "GhostMeleeEnemy.h"
#include "RealmHurtSwitchComponent.h"

AGhostMeleeEnemy::AGhostMeleeEnemy()
{
	// 默认在里世界类型——圈内显形可被打中、圈外残影态。
	DefaultRealmType = ERealmType::Realm;
}

void AGhostMeleeEnemy::PostInitializeComponents()
{
	// 销毁基类自带的所有 URealmTagComponent（不是子类 URealmHurtSwitchComponent）。
	// 关键：必须是"全部"，不能只 FindComponentByClass 删一个 ——
	// 如果 BP 重定父类后保留了多个继承实例，只删一个会让漏网那个的
	// BeginPlay 调 SetActorEnableCollision(false)，让怪穿地。
	TArray<URealmTagComponent*> AllTags;
	GetComponents<URealmTagComponent>(AllTags);
	int32 DestroyedCount = 0;
	for (URealmTagComponent* Tag : AllTags)
	{
		if (Tag && !Tag->IsA<URealmHurtSwitchComponent>())
		{
			Tag->DestroyComponent();
			++DestroyedCount;
		}
	}

	URealmHurtSwitchComponent* HurtSwitch = FindComponentByClass<URealmHurtSwitchComponent>();
	if (!HurtSwitch)
	{
		HurtSwitch = NewObject<URealmHurtSwitchComponent>(this, TEXT("RealmHurtSwitch"));
		HurtSwitch->RegisterComponent();
	}
	RealmTag = HurtSwitch;  // protected 基类成员

	UE_LOG(LogTemp, Log, TEXT("[GhostMelee] PostInitComponents: destroyed %d base RealmTag(s), HurtSwitch=%s"),
	       DestroyedCount, *HurtSwitch->GetName());

	Super::PostInitializeComponents();
}

float AGhostMeleeEnemy::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
                                   AController* EventInstigator, AActor* DamageCauser)
{
	if (URealmHurtSwitchComponent* HurtSwitch = FindComponentByClass<URealmHurtSwitchComponent>())
	{
		if (!HurtSwitch->IsHurtable())
		{
			// 残影态：拒绝伤害。返回 0 给玩家，对方就不会扣血。
			return 0.0f;
		}
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}
