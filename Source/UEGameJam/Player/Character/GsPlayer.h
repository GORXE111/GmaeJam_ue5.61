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
class AGsSkillBall;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUEGameJamPlayerDamagedDelegate, float, LifePercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUEGameJamPlayerDeathDelegate);

UENUM(BlueprintType)
enum class EUEGameJamPlayerAction : uint8
{
	None,
	MeleeAttack,
	Skill,
	Dash,
	Slide,
	WallRun
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
	
	/** 技能输入动作，用于释放技能球 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillAction;

	/** 滑铲输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SlideAction;

	/** 冲刺输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> DashAction;
	
	/** 钩爪输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FalculaAction;

	/** 冲刺速度，用于计算 0.3 秒冲刺可到达的总位移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash", meta = (ClampMin = 0, Units = "cm/s"))
	float DashSpeed = 2000.0f;

	/** 冲刺持续时间，数值越大前冲位移段持续越久 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash", meta = (ClampMin = 0, Units = "s"))
	float DashDuration = 0.3f;

	/** 两次冲刺之间的冷却时间，数值越大连续冲刺间隔越久 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash", meta = (ClampMin = 0, Units = "s"))
	float DashCooldown = 0.75f;

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

	/** 下坡滑铲时每秒增加的速度，数值越大下坡加速越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0))
	float SlideSlopeAcceleration = 900.0f;

	/** 滑铲可达到的最大水平速度，数值越大下坡时最高速度越高 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide", meta = (ClampMin = 0, Units = "cm/s"))
	float SlideMaxSpeed = 1800.0f;

	/** 滑行期间循环播放的第一人称蒙太奇，用于表现滑行动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slide")
	TObjectPtr<UAnimMontage> SlideMontage;

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

	/** 技能释放时生成的技能球类，可在蓝图中替换具体表现 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGsSkillBall> SkillProjectileClass;

	/** 旧版技能发射 Socket 配置，当前极简发射逻辑不再使用 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta = (AllowPrivateAccess = "true"))
	FName SkillSpawnSocketName = NAME_None;

	/** 旧版技能发射前推距离，当前极简发射逻辑不再使用 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta = (ClampMin = 0, Units = "cm"))
	float SkillSpawnForwardOffset = 100.0f;

	/** 技能瞄准检测的最远距离，数值越大越容易命中远处准星中心位置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta = (ClampMin = 0, Units = "cm"))
	float SkillAimTraceDistance = 10000.0f;

	/** 技能释放占用动作状态的时长，数值越大越久不能触发其他互斥动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta = (ClampMin = 0, Units = "s"))
	float SkillActionDuration = 0.15f;

	/** 玩家默认视野角，静止或低速移动时相机会平滑回到这个 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float DefaultCameraFOV = 100.0f;

	/** 玩家跑起来时过渡到的视野角，用于增强速度感 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float RunningCameraFOV = 120.0f;

	/** 冲刺期间目标视野角，用于增强爆发速度感 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 1, ClampMax = 170, Units = "deg"))
	float DashCameraFOV = 130.0f;

	/** 水平移动速度达到这个值时视为跑起来，单位为厘米每秒 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0, Units = "cm/s"))
	float RunFOVSpeedThreshold = 450.0f;

	/** 相机 FOV 向目标值过渡的速度，数值越大变化越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0))
	float CameraFOVInterpSpeed = 8.0f;

	/** 头部原始旋转偏移的保留比例，数值越大保留的方向轻晃越明显 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0, ClampMax = 1))
	float HeadCameraRotationBlendAlpha = 0.25f;

	/** 头部原始旋转偏移平滑过渡的速度，数值越大轻晃跟随越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta = (ClampMin = 0))
	float HeadCameraRotationInterpSpeed = 12.0f;

	/** 起跳后延迟多久才开始检测墙跑触发，单位为秒 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, Units = "s"))
	float WallRunCheckDelay = 0.2f;

	/** 左右两侧墙跑检测的射线距离，数值越大越容易探测到侧边墙面 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, Units = "cm"))
	float WallRunSideTraceDistance = 80.0f;

	/** 相机朝向与墙面法线允许的最大点积绝对值，越小越要求沿墙观察 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, ClampMax = 1))
	float WallRunMaxCameraWallNormalDot = 0.6f;

	/** 角色前进方向与相机朝向至少需要多接近才允许触发墙跑 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = -1, ClampMax = 1))
	float WallRunMinForwardCameraDot = 0.8f;

	/** 墙跑时沿墙横向移动的固定速度，数值越大沿墙跑得越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, Units = "cm/s"))
	float WallRunSpeed = 900.0f;

	/** 墙跑跳出时水平发射力度，数值越大斜向离墙跳得越远 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, Units = "cm/s"))
	float WallRunJumpHorizontalStrength = 850.0f;

	/** 墙跑跳出时向上的发射力度，数值越大离墙后跳得越高 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, Units = "cm/s"))
	float WallRunJumpVerticalStrength = 650.0f;

	/** 墙跑时第一人称视角倾斜的角度，右墙为负左墙为正 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0, ClampMax = 89, Units = "deg"))
	float WallRunCameraTiltAngle = 15.0f;

	/** 墙跑视角倾斜切换的速度，数值越大进入和回正越快 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wall Run", meta = (ClampMin = 0))
	float WallRunCameraTiltInterpSpeed = 8.0f;

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

	/** 起跳后延迟开启墙跑检测的计时器 */
	FTimerHandle WallRunDetectionDelayTimer;

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

	/** 进入冲刺时锁定的方向 */
	FVector DashDirection = FVector::ForwardVector;

	/** 最近一次成功冲刺发生的时间 */
	float LastDashTime = 0.0f;

	/** 进入冲刺前缓存的完整速度，用于冲刺结束时提取前向惯性和竖直速度 */
	FVector PreDashVelocity = FVector::ZeroVector;

	/** 进入冲刺前缓存的移动模式，用于冲刺结束后恢复移动组件 */
	EMovementMode PreDashMovementMode = MOVE_Walking;

	/** 进入冲刺前缓存的自定义移动模式 */
	uint8 PreDashCustomMovementMode = 0;

	/** 冲刺开始时的位置 */
	FVector DashStartLocation = FVector::ZeroVector;

	/** 冲刺目标位置 */
	FVector DashTargetLocation = FVector::ZeroVector;

	/** 当前冲刺已推进的时间 */
	float CurrentDashElapsedTime = 0.0f;

	/** 自上次落地以来是否已经完成过一次空中冲刺 */
	bool bHasDashedSinceLanded = false;

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

	/** 是否已进入起跳后的墙跑检测阶段 */
	bool bCanCheckWallRun = false;

	/** 本次腾空是否已经成功触发过墙跑提示 */
	bool bHasTriggeredWallRunThisJump = false;

	/** 墙跑开始时锁定的沿墙移动方向 */
	FVector WallRunDirection = FVector::ZeroVector;

	/** 当前墙跑依附的墙面法线 */
	FVector WallRunSurfaceNormal = FVector::ZeroVector;

	/** 进入墙跑前缓存的重力缩放 */
	float PreWallRunGravityScale = 1.0f;

	/** 进入墙跑前缓存的空中控制强度 */
	float PreWallRunAirControl = 0.0f;

	/** 进入墙跑前缓存的移动模式 */
	EMovementMode PreWallRunMovementMode = MOVE_Falling;

	/** 进入墙跑前缓存的自定义移动模式 */
	uint8 PreWallRunCustomMovementMode = 0;

	/** 第一人称相机默认的相对变换，用于还原头部 Socket 的原始跟随朝向 */
	FTransform DefaultFirstPersonCameraRelativeTransform = FTransform::Identity;

	/** 当前平滑后的头部旋转偏移 */
	FRotator CurrentHeadCameraRotationOffset = FRotator::ZeroRotator;

	/** 当前墙跑视角目标 Roll，右墙为负左墙为正，非墙跑为 0 */
	float TargetWallRunCameraRoll = 0.0f;

	/** 当前墙跑视角已经平滑到的 Roll 值 */
	float CurrentWallRunCameraRoll = 0.0f;

public:

	/** 生命值变化委托，参数为当前生命百分比 */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FUEGameJamPlayerDamagedDelegate OnDamaged;

	/** 玩家死亡委托 */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FUEGameJamPlayerDeathDelegate OnDeath;

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
	void DoSkill();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSlide();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSlideEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoDash();

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsCharacterActionActive() const;

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsSliding() const;

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsDashing() const;

	UFUNCTION(BlueprintPure, Category="Action")
	bool IsWallRunning() const;

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

	/** 正常结束冲刺并恢复移动状态，只保留进入冲刺前的前向惯性和竖直速度 */
	void FinishDash();

	/** 强制中断冲刺并恢复移动组件，不恢复进入冲刺前速度 */
	void AbortDash();

	/** 尝试开始滑铲 */
	bool StartSlide();

	/** 尝试开始冲刺 */
	bool StartDash();

	/** 根据最近一次移动输入计算滑铲方向 */
	bool TryGetSlideInputDirection(FVector& OutSlideDirection) const;

	/** 停止滑铲；如果站起空间不足且未强制恢复则返回 false */
	bool StopSlide(bool bForceRestore);

	/** 判断滑铲后的胶囊体是否可以安全恢复到站立高度 */
	bool CanRestoreSlideCapsule() const;

	/** 停止当前滑行蒙太奇，避免误停其他蒙太奇 */
	void StopSlideMontage();

	/** 每帧更新滑铲速度与结束条件 */
	void UpdateSlide(float DeltaSeconds);

	/** 每帧推进冲刺位移并处理碰撞与结束条件 */
	void UpdateDash(float DeltaSeconds);

	/** 起跳后开启墙跑检测延迟 */
	void StartWallRunDetectionDelay();

	/** 延迟结束后正式允许墙跑检测 */
	void EnableWallRunDetection();

	/** 重置本次腾空的墙跑检测状态 */
	void ResetWallRunDetection();

	/** 每帧检测是否满足墙跑触发条件 */
	void UpdateWallRunDetection();

	/** 从角色左右两侧寻找可用于墙跑的墙面 */
	bool TryFindWallRunSurface(FHitResult& OutWallHit, FVector& OutWallNormal) const;

	/** 判断当前状态是否满足墙跑触发条件 */
	bool CanTriggerWallRun(const FVector& WallNormal) const;

	/** 开始一次沿墙横向跑动 */
	bool StartWallRun(const FVector& WallNormal);

	/** 每帧维持墙跑移动与退出条件 */
	void UpdateWallRun(float DeltaSeconds);

	/** 结束当前墙跑并恢复普通空中状态 */
	void StopWallRun();

	/** 尝试从墙跑状态跳出并重新开启墙跑检测延迟 */
	bool TryWallRunJump();

	/** 每帧平滑更新墙跑时的相机倾斜 */
	void UpdateWallRunCameraTilt(float DeltaSeconds);

	/** 每帧更新第一人称相机朝向，合成控制器瞄准、头部轻晃与墙跑倾斜 */
	void UpdateFirstPersonCameraRotation(float DeltaSeconds);

	/** 设置墙跑相机倾斜的目标 Roll */
	void SetWallRunCameraTiltTarget(float InTargetRoll);

	/** 清理冲刺运行时状态缓存 */
	void ClearDashState();

	/** 更新最近一次安全落地点 */
	void UpdateSafeLandingTransform();

	/** 触发深坑回传 */
	void RecoverFromDeepFall();

	/** 开始一次近战攻击 */
	bool StartMeleeAttack();

	/** 获取技能发射使用的真实玩家视角位置与朝向 */
	bool GetSkillViewPoint(FVector& OutViewLocation, FRotator& OutViewRotation) const;

	/** 获取技能沿屏幕中心瞄准时的目标点 */
	FVector GetSkillAimTarget(const FVector& ViewLocation, const FVector& ViewDirection) const;

	/** 释放一次技能球 */
	bool StartSkillCast();

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
