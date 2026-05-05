// ===================================================
// 文件：MeleeEnemy.h
// 说明：里世界近战敌人。触发前站桩或巡逻，发现玩家后追击+突进+挥砍。
//       MeleeHitbox 默认禁用，仅在 Swing Task 激活窗口内开启。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "MeleeEnemy.generated.h"

class UCapsuleComponent;
class UDamageType;
class UMeleeEnemyDataAsset;

UCLASS()
class UEGAMEJAM_API AMeleeEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMeleeEnemy();

	virtual void ApplyDataAsset() override;

	/** 立即向指定方向施加突进冲量 */
	UFUNCTION(BlueprintCallable, Category="Enemy|Melee")
	void PerformDash(const FVector& Direction);

	/** 打开/关闭近战判定盒 */
	UFUNCTION(BlueprintCallable, Category="Enemy|Melee")
	void SetMeleeHitboxActive(bool bActive);

	UFUNCTION(BlueprintPure, Category="Enemy|Melee")
	float GetMeleeDamage() const { return CachedMeleeDamage; }

	UFUNCTION(BlueprintPure, Category="Enemy|Melee")
	UCapsuleComponent* GetMeleeHitbox() const { return MeleeHitbox; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Melee")
	TObjectPtr<UCapsuleComponent> MeleeHitbox;

	/** 如果骨骼上有合适的 socket 名（如 Hand_R），把 Hitbox 附加到它；空 = 附在 Capsule 上 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Melee")
	FName MeleeHitboxAttachSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Melee")
	TSubclassOf<UDamageType> MeleeDamageType;

	/** 运行时伤害值（由 DataAsset 或默认 20 填充） */
	UPROPERTY()
	float CachedMeleeDamage = 20.f;

	/** 当前挥砍命中过的 Actor 集合，避免一次 Swing 多次触发 */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> AlreadyHitActors;

	UFUNCTION()
	void OnMeleeHitboxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                               bool bFromSweep, const FHitResult& SweepResult);
};
