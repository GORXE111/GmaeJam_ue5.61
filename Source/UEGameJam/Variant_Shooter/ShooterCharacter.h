// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UEGameJamCharacter.h"
#include "ShooterWeaponHolder.h"
#include "Character/HeroCharacter.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class AShooterPickupBase;
class UAnimInstance;
class UAnimMontage;
class UDamageType;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;
class USphereComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

UENUM(BlueprintType)
enum class EShooterCharacterAction : uint8
{
	None,
	Kick,
	Slide
};

/**
 *  A player controllable first person shooter character
 *  Manages a single weapon slot through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class UEGAMEJAM_API AShooterCharacter : public AHeroCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

	/** 踢击造成伤害时使用的球形检测范围，可在蓝图中调整位置和大小 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* KickDamageCollision;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** 切换武器输入动作，仅为兼容蓝图绑定保留，当前不会产生实际切枪效果 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* PickupAction;

	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* KickAction;

	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* SlideAction;

	/** 滑铲时使用的水平移动速度，数值越大向前滑得越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Slide", meta = (ClampMin = 0, Units = "cm/s"))
	float SlideSpeed = 1200.0f;

	/** 滑铲时胶囊体的半高，用于让玩家保持较低姿态 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Slide", meta = (ClampMin = 0, Units = "cm"))
	float SlideCapsuleHalfHeight = 48.0f;

	/** 滑铲速度低于这个值时会尝试停止滑铲 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Slide", meta = (ClampMin = 0, Units = "cm/s"))
	float SlideStopSpeed = 400.0f;

	/** 滑铲时每秒降低的速度，单位为厘米每平方秒，数值越大滑铲减速越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Slide", meta = (ClampMin = 0))
	float SlideDeceleration = 500.0f;

	/** 蹬墙跳检测距离，表示胶囊体外额外向周围探测的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = 0, Units = "cm"))
	float WallJumpTraceDistance = 40.0f;

	/** 蹬墙跳的水平弹离速度，数值越大离墙越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpHorizontalStrength = 850.0f;

	/** 蹬墙跳的垂直起跳速度，数值越大跳得越高 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpVerticalStrength = 650.0f;

	/** 允许蹬墙跳的墙面法线最大垂直分量，用于过滤地面、斜坡和天花板 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = 0, ClampMax = 1))
	float WallJumpMaxWallNormalZ = 0.25f;

	/** 选择蹬墙跳墙面时参考的最小朝墙角度，数值越低越容易缓存贴墙状态 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = -1, ClampMax = 1))
	float WallJumpMinApproachDot = 0.05f;

	/** 空中水平速度达到这个值时，才会用朝墙角度来优先选择墙面 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = 0, Units = "cm/s"))
	float WallJumpMinAirHorizontalSpeed = 100.0f;

	/** 判定为同一面墙的法线相似度，数值越高越容易允许相邻墙面连续蹬跳 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Wall Jump", meta = (ClampMin = -1, ClampMax = 1))
	float WallJumpSameWallDot = 0.85f;

	/** 踢击时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Kick")
	UAnimMontage* KickMontage;

	/** 踢击命中时造成的伤害值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Kick", meta = (ClampMin = 0))
	float KickDamage = 50.0f;

	/** 踢击使用的伤害类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Kick")
	TSubclassOf<UDamageType> KickDamageType;

	/** 没有成功播放踢击蒙太奇时，踢击动作锁定的备用时长 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Kick", meta = (ClampMin = 0, Units = "s"))
	float KickFallbackDuration = 0.5f;

	/** 踢击命中敌人时将敌人推开的力度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Kick", meta = (ClampMin = 0))
	float KickPushStrength = 1000.0f;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** 玩家空手时第一人称身体使用的动画蓝图；留空时使用角色蓝图初始动画 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Animation")
	TSubclassOf<UAnimInstance> FirstPersonUnarmedAnimInstanceClass;

	/** 玩家空手时第三人称身体使用的动画蓝图；留空时使用角色蓝图初始动画 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonUnarmedAnimInstanceClass;

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** 玩家默认视野角，静止或低速移动时相机会平滑回到这个FOV */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float DefaultCameraFOV = 100.0f;

	/** 玩家跑起来时过渡到的视野角，用于增强速度感 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float RunningCameraFOV = 120.0f;

	/** 水平移动速度达到这个值时视为跑起来，单位为厘米每秒 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Camera", meta = (ClampMin = 0, Units = "cm/s"))
	float RunFOVSpeedThreshold = 450.0f;

	/** 相机FOV向目标值过渡的速度，数值越大变化越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Camera", meta = (ClampMin = 0))
	float CameraFOVInterpSpeed = 8.0f;

	/** 相对最近一次安全落地点，向下掉落超过这个高度后会回传，单位为厘米 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Fall Recovery", meta = (ClampMin = 0, Units = "cm"))
	float FallResetDepth = 2000.0f;

	/** 深坑回传后至少间隔这么久才允许再次刷新安全点或再次触发回传 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Fall Recovery", meta = (ClampMin = 0, Units = "s"))
	float SafeLandingMinInterval = 0.2f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** The only weapon currently equipped and ready to shoot with */
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	/** Initial first person anim class cached after Blueprint overrides */
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> DefaultFirstPersonUnarmedAnimInstanceClass;

	/** Initial third person anim class cached after Blueprint overrides */
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> DefaultThirdPersonUnarmedAnimInstanceClass;

	/** Pickups currently overlapping this character */
	TArray<TWeakObjectPtr<AShooterPickupBase>> PickupCandidates;

	/** Current character action, used to block overlapping actions */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Action")
	EShooterCharacterAction CurrentAction = EShooterCharacterAction::None;

	/** Timer used to finish the current character action */
	FTimerHandle ActionTimer;

	/** Timer used to trigger the delayed kick hit check */
	FTimerHandle KickDamageTimer;

	/** Capsule half height before entering slide */
	float OriginalSlideCapsuleHalfHeight = 0.0f;

	/** Max walk speed before entering slide */
	float OriginalSlideMaxWalkSpeed = 0.0f;

	/** Most recent local-space move input used to choose the next slide direction */
	FVector2D CachedMoveInput = FVector2D::ZeroVector;

	/** Direction locked when entering slide */
	FVector SlideDirection = FVector::ForwardVector;

	/** Current slide speed along the locked slide direction */
	float CurrentSlideSpeed = 0.0f;

	/** Wall normal cached from the most recent airborne wall contact */
	FVector LastWallContactNormal = FVector::ZeroVector;

	/** Whether there is a recent airborne wall contact cached */
	bool bHasRecentWallContact = false;

	/** Wall normal used by the last successful wall jump before landing */
	FVector LastWallJumpNormal = FVector::ZeroVector;

	/** Whether the character has wall jumped since the last landing */
	bool bHasWallJumpedSinceLanded = false;

	/** Most recent grounded location used for deep fall recovery */
	FVector LastSafeLocation = FVector::ZeroVector;

	/** Rotation stored with the most recent grounded location */
	FRotator LastSafeRotation = FRotator::ZeroRotator;

	/** Whether a valid safe location has been initialized */
	bool bHasSafeLocation = false;

	/** Whether a deep fall recovery is currently repositioning this character */
	bool bIsRecoveringFromFall = false;

	/** Time of the most recent deep fall recovery */
	float LastFallRecoveryTime = -1.0f;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Updates camera effects */
	virtual void Tick(float DeltaSeconds) override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Resets wall jump limits when the character touches ground */
	virtual void Landed(const FHitResult& Hit) override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Handles pickup input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoPickup();

	/** Handles kick input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoKick();

	/** Handles slide input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSlide();

	/** Returns true when any character action is active */
	UFUNCTION(BlueprintPure, Category="Action")
	bool IsCharacterActionActive() const;

	/** Returns true when this character is sliding */
	UFUNCTION(BlueprintPure, Category="Action")
	bool IsSliding() const;

	/** Registers a pickup currently in range */
	void RegisterPickupCandidate(AShooterPickupBase* Pickup);

	/** Removes a pickup from the in-range list */
	void UnregisterPickupCandidate(AShooterPickupBase* Pickup);

	/** Returns true if the character should automatically pick up a weapon */
	bool ShouldAutoPickupWeapon() const;

	/** Replaces the current weapon and returns the replaced weapon class */
	bool ReplaceCurrentWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass, TSubclassOf<AShooterWeapon>& OutReplacedWeaponClass);

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	//~End IShooterWeaponHolder interface

protected:

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward) override;

	/** Clears the cached slide input when movement input is released */
	void OnMoveInputCompleted(const FInputActionValue& Value);

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart() override;

	/** Finds the nearest valid pickup candidate */
	AShooterPickupBase* FindBestPickupCandidate();

	/** Removes invalid pickup candidates */
	void CleanPickupCandidates();

	/** Throws the current empty weapon and leaves this character unarmed */
	void ThrowCurrentWeapon();

	/** Caches the animation classes used when this character has no weapon */
	void CacheDefaultUnarmedAnimInstances();

	/** Restores this character's meshes to the unarmed animation classes */
	void ApplyUnarmedAnimInstances();

	/** Sets both first person and third person character animation classes */
	void SetCharacterAnimInstanceClasses(TSubclassOf<UAnimInstance> FirstPersonAnimClass, TSubclassOf<UAnimInstance> ThirdPersonAnimClass);

	/** Clears the current weapon slot and optionally destroys the weapon actor */
	void ClearCurrentWeapon(bool bDestroyWeapon);

	/** Starts a character action if no other character action is active */
	bool TryStartCharacterAction(EShooterCharacterAction Action, float Duration);

	/** Finishes the current character action */
	void FinishCharacterAction();

	/** Executes kick overlap damage at the hard-coded montage timing */
	void ExecuteKickDamage();

	/** Starts the slide action */
	bool StartSlide();

	/** Converts the cached move input into one of the allowed slide directions */
	bool TryGetSlideInputDirection(FVector& OutSlideDirection) const;

	/** Stops the slide action; returns false if the full-height capsule is blocked */
	bool StopSlide(bool bForceRestore);

	/** Returns true if the slide capsule can safely restore to full height */
	bool CanRestoreSlideCapsule() const;

	/** Keeps slide speed and cancel conditions updated */
	void UpdateSlide(float DeltaSeconds);

	/** Updates the cached nearby wall while airborne */
	void UpdateWallJumpContact();

	/** Clears the cached nearby wall */
	void ClearWallJumpContact();

	/** Attempts to launch away from a nearby wall while airborne */
	bool TryWallJump();

	/** Updates the most recent safe grounded transform used for deep fall recovery */
	void UpdateSafeLandingTransform();

	/** Returns the character to the most recent safe grounded transform */
	void RecoverFromDeepFall();

	/** Finds a valid nearby wall normal for wall jumping */
	bool FindWallJumpSurface(FVector& OutWallNormal) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();
};
