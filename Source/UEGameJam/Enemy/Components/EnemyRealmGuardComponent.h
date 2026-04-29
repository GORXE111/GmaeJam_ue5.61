// ===================================================
// 文件：EnemyRealmGuardComponent.h
// 说明：死亡时冻结 URealmTagComponent，防止其 Tick 在死亡帧
//       之后重新把 ragdoll 的碰撞翻回去。
//
//       坑位来源：URealmTagComponent 每帧根据 Revealer 位置
//       决定 Owner 的 SetActorEnableCollision。敌人死亡时
//       AEnemyCharacter::HandleDeath 会关闭 capsule 碰撞并
//       启动 ragdoll；若不冻结 RealmTag，下一帧它会依据
//       "圈内外"逻辑把 Actor 碰撞翻回 true，导致 ragdoll
//       抖动或破坏。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyRealmGuardComponent.generated.h"

class URealmTagComponent;

UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class UEGAMEJAM_API UEnemyRealmGuardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyRealmGuardComponent();

	/** 冻结 Owner 上的 URealmTagComponent，使其不再 Tick 切换碰撞 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Realm")
	void FreezeRealmTag();

protected:
	virtual void BeginPlay() override;

	/** 缓存同 Owner 上的 RealmTagComponent 引用 */
	UPROPERTY(Transient)
	TObjectPtr<URealmTagComponent> CachedTag;
};
