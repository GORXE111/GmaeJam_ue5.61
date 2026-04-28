// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GsPlayer.generated.h"

class UAnimMontage;
class UBoxComponent;
class UCameraComponent;
class UDamageType;
class UInputAction;
class UInputComponent;
class USkeletalMeshComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUEGameJamPlayerDamagedDelegate, float, LifePercent);

UENUM(BlueprintType)
enum class EUEGameJamPlayerAction : uint8
{
	None,
	MeleeAttack,
	Slide
};

/**
 *  纯玩家侧近战角色
 */
UCLASS()
class UEGAMEJAM_API AGsPlayer : public ACharacter
{
	GENERATED_BODY()

	/** 第一人称手臂网格，仅自己可见 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/** 第一人称相机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** 近战造成伤害时使用的盒形检测范围，可在蓝图中调整位置和大小 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> MeleeDamageCollision;

protected:

	/** 跳跃输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** 移动输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** 鼠标视角输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MouseLookAction;

	/** 近战攻击输入动作，沿用 FireAction 名称以兼容输入资源 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;

	/** 滑铲输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SlideAction;

	/** 滑铲时使用的水平移动速度，数值越大向前滑得越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0, Units = "cm/s"))
	float SlideSpeed = 1200.0f;

	/** 滑铲时胶囊体的半高，用于让角色保持低姿态 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0, Units = "cm"))
	float SlideCapsuleHalfHeight = 48.0f;

	/** 滑铲速度低于这个值时会尝试结束滑铲 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0, Units = "cm/s"))
	float SlideStopSpeed = 400.0f;

	/** 滑铲时每秒降低的速度，数值越大滑铲减速越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0))
	float SlideDeceleration = 500.0f;

	/** 蹬墙跳检测距离，表示胶囊体外额外向周围探测的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = 0, Units = "cm"))
	float WallJumpTraceDistance = 40.0f;

	/** 蹬墙跳的水平弹离速度，数值越大离墙越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpHorizontalStrength = 850.0f;

	/** 蹬墙跳的垂直起跳速度，数值越大跳得越高 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpVerticalStrength = 650.0f;

	/** 允许蹬墙跳的墙面法线最大垂直分量，用于过滤地面和天花板 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = 0, ClampMax = 1))
	float WallJumpMaxWallNormalZ = 0.25f;

	/** 选择蹬墙跳墙面时参考的最小朝墙角度，数值越低越容易缓存贴墙状态 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = -1, ClampMax = 1))
	float WallJumpMinApproachDot = 0.05f;

	/** 空中水平速度达到这个值时，才会用朝墙角度来优先选择墙面 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpMinAirHorizontalSpeed = 100.0f;

	/** 判定为同一面墙的法线相似度，数值越高越容易允许相邻墙面连续蹬跳 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Jump", meta = (ClampMin = -1, ClampMax = 1))
	float WallJumpSameWallDot = 0.85f;

	/** 近战攻击时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	/** 近战命中造成的伤害值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta = (ClampMin = 0))
	float MeleeDamage = 100.0f;

	/** 近战攻击使用的伤害类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee")
	TSubclassOf<UDamageType> MeleeDamageType;

	/** 没有成功播放攻击蒙太奇时，近战动作锁定的备用时长 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta = (ClampMin = 0, Units = "s"))
	float MeleeFallbackDuration = 0.35f;

	/** 近战命中判定延迟，用于把 Box Sweep 对齐到挥砍时机 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta = (ClampMin = 0, Units = "s"))
	float MeleeHitDelay = 0.08f;

	/** 玩家默认视野角，静止或低速移动时相机会平滑回到这个 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float DefaultCameraFOV = 100.0f;

	/** 玩家跑起来时过渡到的视野角，用于增强速度感 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float RunningCameraFOV = 120.0f;

	/** 水平移动速度达到这个值时视为跑起来，单位为厘米每秒 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0, Units = "cm/s"))
	float RunFOVSpeedThreshold = 450.0f;

	/** 相机 FOV 向目标值过渡的速度，数值越大变化越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0))
	float CameraFOVInterpSpeed = 8.0f;

	/** 相对最近一次安全落地点，向下掉落超过这个高度后会回传，单位为厘米 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fall Recovery", meta = (ClampMin = 0, Units = "cm"))
	float FallResetDepth = 2000.0f;

	/** 深坑回传后至少间隔这么久才允许再次刷新安全点或再次触发回传 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fall Recovery", meta = (ClampMin = 0, Units = "s"))
	float SafeLandingMinInterval = 0.2f;

	/** 角色最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta = (ClampMin = 0))
	float MaxHP = 500.0f;

	/** 死亡后延时销毁的时间，留 0 表示立即销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta = (ClampMin = 0, Units = "s"))
	float DeferredDestructionTime = 5.0f;

	/** 当前生命值 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 0.0f;

	/** 当前角色动作，用于阻止互斥动作同时触发 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Action", meta = (AllowPrivateAccess = "true"))
	EUEGameJamPlayerAction CurrentAction = EUEGameJamPlayerAction::None;

	/** 是否已经死亡 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	/** 当前动作结束计时器 */
	FTimerHandle ActionTimer;

	/** 近战命中计时器 */
	FTimerHandle MeleeHitTimer;

	/** 死亡后销毁计时器 */
	FTimerHandle DeferredDestroyTimer;

	/** 滑铲前的胶囊体半高 */
	float OriginalSlideCapsuleHalfHeight = 0.0f;

	/** 滑铲前的最大地面速度 */
	float OriginalSlideMaxWalkSpeed = 0.0f;

	/** 最近一次本地空间移动输入，用于确定滑铲方向 */
	FVector2D CachedMoveInput = FVector2D::ZeroVector;

	/** 进入滑铲时锁定的方向 */
	FVector SlideDirection = FVector::ForwardVector;

	/** 当前滑铲沿锁定方向的速度 */
	float CurrentSlideSpeed = 0.0f;

	/** 最近一次空中贴墙时缓存的墙面法线 */
	FVector LastWallContactNormal = FVector::ZeroVector;

	/** 是否缓存了最近一次可用的贴墙信息 */
	bool bHasRecentWallContact = false;

	/** 最近一次成功蹬墙跳使用的墙面法线 */
	FVector LastWallJumpNormal = FVector::ZeroVector;

	/** 自上次落地以来是否已经完成过一次蹬墙跳 */
	bool bHasWallJumpedSinceLanded = false;

	/** 最近一次安全落地点位置 */
	FVector LastSafeLocation = FVector::ZeroVector;

	/** 最近一次安全落地点朝向 */
	FRotator LastSafeRotation = FRotator::ZeroRotator;

	/** 是否已经记录了可回传的安全落地点 */
	bool bHasSafeLocation = false;

	/** 是否正在执行深坑回传，避免重复进入 */
	bool bIsRecoveringFromFall = false;

	/** 最近一次深坑回传发生的时间 */
	float LastFallRecoveryTime = -1.0f;

public:

	/** 生命值变化委托，参数为当前生命百分比 */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FUEGameJamPlayerDamagedDelegate OnDamaged;

public:

	AGsPlayer();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;

	/** 输入系统回调：处理移动输入 */
	void MoveInput(const FInputActionValue& Value);

	/** 输入系统回调：处理视角输入 */
	void LookInput(const FInputActionValue& Value);

public:

	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSlide();

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsCharacterActionActive() const;

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsSliding() const;

	UFUNCTION(BlueprintPure, Category="Health")
	float GetLifePercent() const;

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const;

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	UBoxComponent* GetMeleeDamageCollision() const { return MeleeDamageCollision; }

protected:

	/** 清空移动输入缓存，避免停下后还能沿旧方向滑铲 */
	void OnMoveInputCompleted(const FInputActionValue& Value);

	/** 开始一个角色动作，如果当前已有动作则返回 false */
	bool TryStartCharacterAction(EUEGameJamPlayerAction Action, float Duration);

	/** 结束当前角色动作 */
	void FinishCharacterAction();

	/** 尝试开始滑铲 */
	bool StartSlide();

	/** 根据最近一次移动输入计算滑铲方向 */
	bool TryGetSlideInputDirection(FVector& OutSlideDirection) const;

	/** 停止滑铲；如果站起空间不足且未强制恢复则返回 false */
	bool StopSlide(bool bForceRestore);

	/** 判断滑铲后的胶囊体是否可以安全恢复到站立高度 */
	bool CanRestoreSlideCapsule() const;

	/** 每帧更新滑铲速度与结束条件 */
	void UpdateSlide(float DeltaSeconds);

	/** 缓存空中最近一次贴墙信息 */
	void UpdateWallJumpContact();

	/** 清空贴墙缓存 */
	void ClearWallJumpContact();

	/** 空中时尝试执行蹬墙跳 */
	bool TryWallJump();

	/** 查找可用于蹬墙跳的墙面法线 */
	bool FindWallJumpSurface(FVector& OutWallNormal) const;

	/** 更新最近一次安全落地点 */
	void UpdateSafeLandingTransform();

	/** 触发深坑回传 */
	void RecoverFromDeepFall();

	/** 开始一次近战攻击 */
	bool StartMeleeAttack();

	/** 读取近战伤害盒当前重叠对象并对命中目标造成伤害 */
	void PerformMeleeHit();

	/** 角色死亡时的统一处理 */
	void Die();

	/** 死亡后延时销毁回调 */
	void OnDeferredDestroy();

	/** 蓝图死亡回调 */
	UFUNCTION(BlueprintImplementableEvent, Category="Player Character", meta = (DisplayName = "On Death"))
	void BP_OnDeath();
};
