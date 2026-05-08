# 敌人系统交接文档

> 代码目录：`Source/UEGameJam/Enemy/`
> 引擎：UE 5.6.1
> 基类：UE 原生 `ACharacter`（敌人系统完全独立于项目已有的玩家角色链 `AUEGameJamCharacter` / `AShooterCharacter` / `AHorrorCharacter` 及 `AShooterNPC`）
> AI 框架：StateTree（`UStateTreeAIComponent`）
> 状态：近战 / 手枪 / 机枪三种敌人全部可用；DataAsset + StateTree 资产在编辑器里搭好后即可 PIE 跑通

---

## 1. 已实现的功能

### 1.1 运行时层（C++，`Source/UEGameJam/Enemy/`）

| 文件 | 类 | 职责 |
|---|---|---|
| `EnemyCharacter.h/.cpp` | `AEnemyCharacter` (Abstract) | 基类：构造时把 RealmTag 实例化为 `URealmHurtSwitchComponent`（不切 SetActorEnableCollision，敌人在任何世界都能正常移动）；TakeDamage 按 RealmType 分流——Surface 始终接受、Realm 仅在 HurtSwitch.IsHurtable=true 时接受；`Die()` 做 Ragdoll / 停 AI / 延迟销毁；向 `UEnemySubsystem` 注册/注销；`ApplyDataAsset()` 把 DataAsset 数值拷到运行时 |
| `EnemyAIController.h/.cpp` | `AEnemyAIController` | 挂 `UStateTreeAIComponent`；`OnPossess` 时从 Pawn 的 `EnemyData.StateTreeAsset` 加载并 `StartLogic()`；`FindPlayerByTag` 缓存带 `"Player"` Tag 的玩家 Pawn |
| `EnemyHealthComponent.h/.cpp` | `UEnemyHealthComponent` | 独立血量组件：MaxHP / CurrentHP / bInvulnerable / `ApplyDamage`；广播 `OnDepleted` / `OnHealthChanged`；**不处理死亡效果**，那是 `AEnemyCharacter::Die` 的活 |
| `EnemyDataAsset.h/.cpp` | `UEnemyDataAsset` (Abstract) | `UPrimaryDataAsset` 基类：MaxHP / DetectionRadius / LoseSightTimeout / WalkSpeed / ChaseSpeed / StateTreeAsset |
| `EnemyProjectile.h/.cpp` | `AEnemyProjectile` | 投射物：Sphere + ProjectileMovement + Niagara Trail + RealmTag（RealmTag Tick 被禁，见 §5.6）；Sphere 对 Pawn 通道 = Overlap，对 World 通道 = Block；每帧检测是否跨越当前揭示球边界，跨越则视为命中分界面并销毁；OnHit 处理墙面命中，OnBeginOverlap 处理 Pawn 命中（仅对带 `"Player"` Tag 的 Actor 应用伤害+销毁，其他 Pawn 穿透） |
| `EnemySubsystem.h/.cpp` | `UEnemySubsystem` | `UWorldSubsystem`：敌人列表；按 Realm / Class 过滤计数；**一次性** `OnAllEnemiesDead` 广播；Exec 命令 `EnemyKillAll` / `EnemyDump` |

#### 三种具体敌人

| 子目录 | 敌人类 | DataAsset 类 | 专属 Task |
|---|---|---|---|
| `Melee/` | `AMeleeEnemy` (Realm) | `UMeleeEnemyDataAsset` | `FEnemyMeleeDashTask` / `FEnemyMeleeSwingTask` |
| `Pistol/` | `APistolEnemy` (Surface) | `UPistolEnemyDataAsset` | `FEnemyPistolAimTask` / `FEnemyPistolFireTask` |
| `MachineGun/` | `AMachineGunEnemy` (Surface) | `UMachineGunEnemyDataAsset` | `FEnemyMGWarmupTask` / `FEnemyMGBurstTask` |

专属行为：
- **Melee**：Capsule Hitbox（默认 NoCollision，Swing 窗口才开 QueryOnly）；`PerformDash` 用 `LaunchCharacter`；Hitbox 命中用 `AlreadyHitActors` 集合防止一次 Swing 多次触发
- **Pistol**：MuzzleComp + Niagara Beam (LaserFX)；Tick 每帧把 `BeamEnd` 用户参数刷到玩家位置；`SetLaserFlicker` 设 `FlickerIntensity` 用户参数；`FireProjectile` Spawn AEnemyProjectile
- **MachineGun**：MuzzleComp + Niagara WarningLasersFX；Tick 处理两件事 — 预警激光 BeamEnd 刷新 + Burst 期的缓慢偏航跟踪（`RInterpConstantTo`）；`FireOneBullet` 用 `FMath::VRandCone` 做扇形散射

### 1.2 StateTree 层（`Source/UEGameJam/Enemy/StateTree/`）

#### 共享枚举 `EEnemyAttackPhase`

Idle / Lockon / Aim / Warmup / Fire / Burst / Recover / Cooldown — 仅作为 `WaitPhase` Task 的语义标签，方便调试打印，不驱动引擎行为。

#### 通用 Task（`EnemyStateTreeTasks.h/.cpp`）

| Task | 用途 | 关键行为 |
|---|---|---|
| `FEnemyAcquireTargetTask` | 感知玩家 | Tick 调 `FindPlayerByTag + Radius + 可选 LOS`；输出 `TargetActor` / `bFound`；若判定丢目标会同步清掉 `AIController` 的玩家缓存，避免远程敌人卡在攻击循环；**从不 Succeed**（随状态退出才停） |
| `FEnemyMoveToTargetTask` | 移动跟踪 | `EnterState` 先强制 Repath；Tick 距离检查 + 每 `RepathInterval` 重发 `MoveTo` 请求以跟踪移动玩家；到达 AcceptRadius 内即 Succeeded；ExitState `AbortMove` |
| `FEnemyPatrolTask` | 巡逻 | `EnterState` 记录 Origin；在 PatrolRadius 内用 `NavSys->GetRandomReachablePointInRadius` 选点，到达后等 IdleBetweenPoints 再选下一个 |
| `FEnemyFacePlayerTask` | 朝向玩家 | Tick 用 `RInterpConstantTo` 改 Yaw；从不 Succeed |
| `FEnemyWaitPhaseTask` | 定时+语义 | 累计时间到 Duration → Succeeded；打印 `"Phase: <Name> (Xs)"` |
| `FEnemySetMovementSpeedTask` | 切速度 | EnterState 改 `MaxWalkSpeed`（Running 挂住状态） |

#### 通用 Condition（`EnemyStateTreeConditions.h/.cpp`）

每个 Condition 的 InstanceData 都含 `bInvert` 参数（见 §5.2）：

| Condition | 原始判定 | 典型用法 |
|---|---|---|
| `FEnemyHasPlayerTargetCondition` | `IsValid(Controller->GetCachedPlayer())` | Searching → Chase / Aim；bInvert=true 用于"丢目标打断"。依赖 `AcquireTarget` 在丢目标时同步清缓存 |
| `FEnemyPlayerInRadiusCondition` | `DistSq ≤ Radius²` | Chase → Attack（Radius=180 进攻击范围）；bInvert=true 代替"玩家跑远"判定 |
| `FEnemyPlayerInRangeCondition` | `Min² ≤ DistSq ≤ Max²` | 保持某个射击距离段 |
| `FEnemyHasLineOfSightCondition` | LineTrace 未被阻挡 | 高级感知 |
| `FEnemyIsDeadCondition` | `Enemy->IsDead()` | 死亡分支；bInvert=true 等效 "Is Alive" |

---

## 2. 架构关系

### 2.1 类关系

```
ACharacter (UE)
  └── AEnemyCharacter [Abstract]
        ├── AMeleeEnemy
        ├── APistolEnemy
        └── AMachineGunEnemy

AAIController (UE)
  └── AEnemyAIController
        └── UStateTreeAIComponent

UWorldSubsystem (UE)
  └── UEnemySubsystem
        └── TArray<TWeakObjectPtr<AEnemyCharacter>>

UPrimaryDataAsset (UE)
  └── UEnemyDataAsset [Abstract]
        ├── UMeleeEnemyDataAsset
        ├── UPistolEnemyDataAsset
        └── UMachineGunEnemyDataAsset

UActorComponent
  ├── UEnemyHealthComponent
  └── URealmTagComponent (外部)
```

### 2.2 生命周期

```
Spawn Enemy
  Ctor: 创建 RealmTag + Health，默认 Ragdoll 配置
  PostInitializeComponents: RealmTag->SetRealmType(DefaultRealmType)
  BeginPlay:
    ApplyDataAsset()       // MaxHP、WalkSpeed、各敌人专属字段
    Health.OnDepleted → Die
    Subsystem->RegisterEnemy

AIController.OnPossess:
  Enemy.OnEnemyDeath → HandleOwnerDeath
  StateTreeAI.SetStateTree(Data.StateTreeAsset)
  StateTreeAI.StartLogic()
  RefreshPlayer()

--- 玩家扔伤害过来 ---
Enemy.TakeDamage → Health.ApplyDamage → OnDepleted
  → AEnemyCharacter::Die:
       bIsDead = true
       AIController.StopLogic / UnPossess
       Movement.DisableMovement
       Capsule.NoCollision
       Mesh: Ragdoll Profile + SimulatePhysics
       Subsystem.UnregisterEnemy → 若 0 则广播 OnAllEnemiesDead
       SetTimer(DeferredDestructionTime) → Destroy

EndPlay: 兜底再次 UnregisterEnemy（幂等）
```

### 2.3 数据流

```
BP_MeleeEnemy (蓝图)
  └── EnemyData: DA_MeleeEnemy_Default ┐
                                        │ (BeginPlay)
  ApplyDataAsset() ←──────────────── ───┘
    Health.MaxHP ← Data.MaxHP
    MaxWalkSpeed ← Data.WalkSpeed
    CachedMeleeDamage ← Data.MeleeDamage

AIController (Possess):
  StateTreeAI.SetStateTree( Data.StateTreeAsset = ST_MeleeEnemy )
```

**关键点**：StateTree 资产**不是**在 AIController 类里固定的，而是由 Pawn 的 DataAsset 间接决定。`AEnemyAIController` 一个类可以驱动所有三种敌人；不同敌人靠不同 StateTree 资产区分。

---

## 3. 每种敌人的 StateTree 结构

### 3.1 Melee

```
Root
└── Active
    ├── [Task] Acquire Target (DetectionRadius=1500)
    │
    ├── Searching (default child)
    │   ├── [Task] Patrol (bPatrol=true) 或 Wait Phase
    │   └── [Transition] → Chase  (On Tick, Cond: Has Player Target)
    │
    ├── Chase
    │   ├── [Task] Set Movement Speed (= ChaseSpeed)
    │   ├── [Task] Move To Target (Target ← AcquireTarget.TargetActor)
    │   ├── [Transition] → LockOn    (On Tick, Cond: Player In Radius, Radius=180)
    │   └── [Transition] → Searching (On Tick, Cond: Player In Radius, Radius=2000, bInvert=true)
    │
    └── Attack (子状态 Sequence)
        ├── LockOn: Face Player + Wait Phase (0.25s)     → Dash   (On Completed)
        ├── Dash:   Melee Dash (Impulse 900, 0.35s)      → Swing  (On Completed)
        ├── Swing:  Melee Swing (Hitbox 0.2s, Total 0.4s) → Recover (On Completed)
        └── Recover: Wait Phase (0.6s)                   → Chase  (On Completed)
```

**AcquireTarget 放 Active 的关键**：StateTree 作用域规则下，Task 的 Output 只对"同一分支祖先下的后续节点"可见。放在 Searching 子状态下，Chase / Attack 看不到 `TargetActor`；放在 Active 父节点上，所有子状态都能绑。

### 3.2 Pistol

```
Root
└── Active
    ├── [Task] Acquire Target (DetectionRadius=2500)
    │
    ├── Searching → Aim      (On Tick, Cond: Has Player Target)
    │
    ├── Aim
    │   ├── [Task] Face Player
    │   ├── [Task] Pistol Aim (Duration=1.0, FlickerStartRatio=0.7)
    │   ├── [Transition] → Searching (On Tick, Cond: Has Player Target, bInvert=true)
    │   └── [Transition] → Fire      (On State Completed)
    │
    ├── Fire: Pistol Fire (单帧 → Succeeded) → Cooldown  (On State Completed)
    │
    └── Cooldown: Wait Phase (1.6s) → Aim  (On State Completed)
```

没有独立 Chase — 手枪敌人定位为站桩射手。

### 3.3 MachineGun

```
Root
└── Active
    ├── [Task] Acquire Target (DetectionRadius=3000)
    │
    ├── Searching → Warmup   (On Tick, Cond: Has Player Target)
    │
    ├── Warmup
    │   ├── [Task] Face Player (Yaw 180°/s)
    │   ├── [Task] MG Warmup   (Duration=1.25)  // 激光预警
    │   ├── [Transition] → Searching (On Tick, Cond: Has Player Target, bInvert=true)
    │   └── [Transition] → Burst     (On State Completed)
    │
    ├── Burst: MG Burst (Duration=1.35, FireRate=6) → Cooldown  (On Completed)
    │   // Burst 期间不允许被打断；敌人缓慢偏航跟踪玩家
    │
    └── Cooldown: Wait Phase (1.7s)
        ├── → Warmup    (On Completed, Cond: Has Player Target)
        └── → Searching (On Completed)
```

Warmup 允许中断（丢目标则回 Searching），Burst 不允许（一旦开枪就打完这波）。Cooldown 结束时按是否有目标选择下一个状态。

---

## 4. DataAsset 字段速查

所有字段都是 `EditAnywhere, BlueprintReadOnly`。留空 StateTreeAsset 会导致 AIController 在 OnPossess 时什么都不做（敌人站着不动）。

| 类 | 字段分组 | 用途 |
|---|---|---|
| `UEnemyDataAsset` | MaxHP / DetectionRadius / LoseSightTimeout / WalkSpeed / ChaseSpeed / **StateTreeAsset** | 通用 |
| `UMeleeEnemyDataAsset` | AttackRadius / LockOnDuration / DashImpulse / DashDuration / HitboxActiveWindow / SwingDuration / AttackRecovery / MeleeDamage / bPatrol / PatrolRadius / PatrolIdleTime | 近战 |
| `UPistolEnemyDataAsset` | AimDuration / AimFlickerStartRatio / Cooldown / ProjectileSpeed / ProjectileDamage* / ProjectileClass / LaserNiagara / MuzzleFlashFX | 手枪 |
| `UMachineGunEnemyDataAsset` | WarmupDuration / BurstDuration / Cooldown / FireRate / SpreadHalfAngleDeg / TrackingYawSpeed / BulletDamage* / BulletSpeed / ProjectileClass / WarningLasersNiagara / MuzzleFlashFX | 机枪 |

\* `ProjectileDamage` / `BulletDamage` 字段**未被消费**（见 §7.2）。实际伤害读 `AEnemyProjectile::Damage`（子弹蓝图 Class Defaults 里填）。

---

## 5. 关键设计决策

### 5.1 敌人模块严格独立

项目里已有的 `AUEGameJamCharacter` / `AShooterCharacter` / `AHorrorCharacter` / `AShooterNPC` 不是本次协作者写的，**不动**。敌人基类直接继承 UE 原生 `ACharacter`，不复用任何 Variant_* 代码。这样保证：

- 敌人模块可以独立演化、重构
- 不会破坏已有玩家/玩法模块
- 可以把整个 `Source/UEGameJam/Enemy/` 目录和对应文档作为一个独立单元交接

### 5.2 StateTree Condition 的 `bInvert` 参数

UE 5.6.1 的 StateTree 编辑器里，Condition 节点**没有原生 Invert 勾选项**。为了让"玩家不在半径内"这类反向判定自然表达，每个 Condition 的 InstanceData 都加了一个 `bInvert` 参数（`Category="Parameter"`）。实现模式：

```cpp
bool FXxxCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    // 边界处理：Target 无效 / World 拿不到时原始判定视为 false
    if (!IsValid(Data.Enemy) || !IsValid(Data.Target)) { return Data.bInvert; }

    const bool bRaw = /* 原始判定逻辑 */;
    return Data.bInvert ? !bRaw : bRaw;
}
```

**边界必须等价于 "bRaw=false 后 Invert"**，这样 `Player In Radius (bInvert=true)` 在 Target 尚未就位时不会误判为"玩家不在半径内 → 成立"，而是认为"尚未就位，条件不成立"。避免刚 Spawn 那一帧因为 AcquireTarget 还没填 Output 就触发 Invert 分支。

**用途示例**：
- Chase → Searching：`Player In Radius (Radius=2000, bInvert=true)` = 玩家跑到 2000 外
- Aim → Searching（打断）：`Has Player Target (bInvert=true)` = 目标丢了
- Cooldown → Warmup（循环继续）：`Has Player Target (bInvert=false)` = 还看得见

### 5.3 AIController 的延迟启动

`UStateTreeAIComponent::InitializeComponent` 会在 StateTree 资产未设时打印 `Error: The State Tree asset is not set`。由于我们的资产通过 Pawn 的 DataAsset 传递，构造 AIController 时拿不到。解决方案（`EnemyAIController.cpp:22-25`）：

```cpp
AEnemyAIController::AEnemyAIController()
{
    StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
    StateTreeAI->SetStartLogicAutomatically(false);   // 不在 OnRegister 时自动 Start
    StateTreeAI->bWantsInitializeComponent = false;   // 跳过早期 ValidateStateTreeReference
}
```

然后在 `OnPossess` 里手动：

```cpp
StateTreeAI->SetStateTree(Data->StateTreeAsset);
StateTreeAI->StartLogic();
```

### 5.4 玩家感知用 Actor Tag 而非 AIPerceptionComponent

玩家 Pawn 挂 Actor Tag `"Player"`。`AEnemyAIController::FindPlayerByTag` 在需要时用 `TActorIterator<APawn>` 扫一次并缓存。理由：

- MVP 阶段单人、敌人数量可控，扫描成本可忽略
- AIPerception 要配 SenseConfig、维护 Perception 事件，MVP 不值
- 同一 `"Player"` Tag 约定同时被 `EnemyProjectile::OnHit` 和 `MeleeEnemy::OnMeleeHitboxBeginOverlap` 用来过滤命中，三处一致

`FindPlayerByTag` 的缓存语义要注意：`FEnemyHasPlayerTargetCondition` 只检查 `GetCachedPlayer()` 是否有效，不重新做距离判定。2026-05 的一次 bugfix 中，曾出现机枪敌人把玩家纳入目标后，即使玩家跑出 `DetectionRadius`，也因为缓存没清掉而持续通过 `Has Player Target` 条件，导致不回 `Searching`，而是在 `Warmup/Burst/Cooldown` 间继续循环并朝旧方向扫射。

当前修复放在 `FEnemyAcquireTargetTask`：当它因为“玩家不存在 / 超出 DetectionRadius / LOS 丢失”而判定 `bFound=false`、`TargetActor=nullptr` 时，会额外调用 `AEnemyAIController::ClearCachedPlayer()`。这样现有 StateTree 资产无需改动，所有依赖 `Has Player Target` 的远程敌人都能正确触发"丢目标"分支。

### 5.5 独立血量组件

`UEnemyHealthComponent` 不复用玩家的 HP 系统（玩家用 `AShooterCharacter::CurrentHP` / `MaxHP`，是玩家自有私有字段，无组件化）。敌人自己一套组件化 HP 便于：

- 子弹 / 挥砍 / 其他投射物统一走 `AActor::TakeDamage` → `Health->ApplyDamage`
- 后续做无敌帧、护甲、HP bar 等扩展时只动组件

### 5.6 子弹豁免 Realm 碰撞切换 + Pawn 通道穿透

项目的 `URealmTagComponent`（外部模块）会根据物体的 `RealmType` + 它离 RealmRevealer（玩家）中心的距离，每帧 `SetActorEnableCollision`：Surface 物体在里世界圈内关碰撞，Realm 物体在圈外关碰撞。

**问题 A**：玩家身上挂着 RealmRevealer（开启时），表世界敌人（手枪/机枪）的子弹 `RealmType=Surface` 在接近玩家时进入了"里世界圈内"→ 被 RealmTag 关碰撞 → OnHit 不触发 → 打不中玩家。里世界敌人的子弹更糟：BeginPlay 初始就按 `Realm` 设碰撞 OFF，整个生命周期都打不到任何东西。

**约束**：不允许改 `Source/UEGameJam/Realm/` 模块（非本协作者所有）。`URealmTagComponent` 也没预留豁免开关。

**方案 A**：构造时禁用 RealmTag 的 Tick + BeginPlay 强制 `SetActorEnableCollision(true)`：

```cpp
RealmTag = CreateDefaultSubobject<URealmTagComponent>(TEXT("RealmTag"));
RealmTag->PrimaryComponentTick.bCanEverTick = false;
// BeginPlay:
SetActorEnableCollision(true);
```

这样无论发起者是表还是里世界，子弹整个生命周期保持碰撞开启，跨世界都能命中玩家，Realm 模块代码一个字没改。

**问题 B**：Sphere 用 `BlockAllDynamic` 时，子弹出生在敌人胶囊体内 → 第一帧自命中 → 被自己的 `OnHit` 销毁。改成 `OverlapAllDynamic` 又不再触发 Hit、收不到伤害事件。同时还要让子弹**穿透其他敌人**（不打队友），但仍能被墙挡住、能命中玩家。

**方案 B**：保留 `BlockAllDynamic` 作为底，但单独把 `ECC_Pawn` 通道改 `Overlap`：

```cpp
CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
CollisionComp->SetGenerateOverlapEvents(true);
CollisionComp->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnHit);
CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AEnemyProjectile::OnBeginOverlap);
```

事件分流：
- **OnHit**：仅墙面/世界几何 → 播 ImpactFX + Destroy。**不再处理伤害**。
- **OnBeginOverlap**：所有 Pawn（自己、发射者、其他敌人、玩家）都会命中。回调里：
  - 过滤 `OtherActor == GetInstigator()` → 解决出生自爆（替代了之前用过的 `IgnoreActorWhenMoving` 方案）
  - 过滤 `!OtherActor->ActorHasTag(PlayerTag)` → 撞到其他敌人/杂物时 return（不扣血、不销毁），子弹继续飞
  - 命中玩家时 `ApplyDamage + ImpactFX + Destroy`

视觉不受影响：Realm 视觉剔除走 MPC + `MaterialExpressionRealmRevealMask` 材质节点，跟 `URealmTagComponent` 无关。

### 5.6.1 子弹不能穿过揭示球边界

上面的方案让子弹不再被 Realm 系统自动关碰撞，但这也意味着它会无视当前活跃揭示球的表/里世界分界，出现"球内发出的子弹能打到球外、球外能打进球内"的问题。

当前修正放在 `AEnemyProjectile` 自身：保留 `RealmTag` 禁 Tick + 强制碰撞开启，但在 `Tick` 和命中回调里读取 `URealmRevealerComponent::GetActiveCenter/Radius`，比较子弹上一帧位置与当前位置是否一内一外；若发生跨界，则解一遍线段与球面的交点，把它当作命中分界面处理（播 `ImpactFX` + `Destroy()`）。

这样维持了两点同时成立：
- 子弹不会因为靠近玩家或 Reveal 圈而被 `SetActorEnableCollision(false)` 直接失效
- 子弹也不能穿过当前揭示球边界，球内外互相无法命中

约束：当前 `URealmRevealerComponent` 仍是全局只取一个活动胜者，所以子弹阻挡也只认"当前激活的那个球/揭示器"，与现有材质和 RealmTag 规则保持一致。

代价：子弹仍可能撞到一些按 Realm 规则本该隐藏的世界几何（如圈外的里世界静态物）。MVP 场景里子弹飞行路径短、这类误撞罕见，可接受。

### 5.7 为什么 Subsystem 用 `UWorldSubsystem`

- 敌人列表按关卡重置（PIE 每次开始 Initialize，结束 Deinitialize）
- `OnAllEnemiesDead` 只在当前关卡语境下有意义
- `UGameInstanceSubsystem` 会跨关卡保留，不符需求

`AliveEnemies` 用 `TWeakObjectPtr` 避免敌人 Destroy 后的野指针；遍历时双重检查 `IsValid + !IsDead`。

`OnAllEnemiesDead` 用 `bAnnouncedAllDead` + `bHadAnyRegistration` 两个 flag 防止：
- 关卡开始时 0 敌人就误触发
- 死光后再 Spawn 一只再死时重复触发

如果将来要做分波，需把 `bAnnouncedAllDead` 改成按波次重置。

### 5.8 跨世界攻击规则（敌人本体）

游戏的核心交互之一是表/里世界双层敌人。需求：

| 规则 | 表世界敌人（Surface） | 里世界敌人（Realm） |
|---|---|---|
| 敌人攻击玩家 | 任何世界都能命中 | 任何世界都能命中 |
| 玩家攻击敌人 | 任何世界都能命中 | **仅玩家位于里世界时**能命中 |
| 移动 | 任何世界都能移动 | 任何世界都能移动 |

**问题**：`URealmTagComponent` 默认行为是按"圈内/圈外 + RealmType"每帧切 `SetActorEnableCollision(false)`，导致：
1. 表世界敌人靠近玩家（进入揭示圈）就被关碰撞 → 站不住地、打不到玩家、子弹也打不到它。
2. 里世界敌人在圈外被关碰撞 → 同上。
3. 总之只要进出"自己不属于的那一侧"，敌人就完全瘫痪。

**方案**：基类 `AEnemyCharacter` 构造时把 RealmTag 实例化为 `URealmHurtSwitchComponent`（`Source/UEGameJam/Realm/`，外部模块预留的子类）。该组件继承自 `URealmTagComponent`，但**完全跳过** `SetActorEnableCollision`，只追踪"当前是否处于实体态" `IsHurtable`。

```cpp
// EnemyCharacter.cpp 构造函数
RealmTag = CreateDefaultSubobject<URealmHurtSwitchComponent>(TEXT("RealmTag"));
```

声明仍是基类指针 `TObjectPtr<URealmTagComponent> RealmTag`（多态），所有读取 `RealmType` 的旧代码不变。

**伤害分流**在基类 `TakeDamage` 实现：

```cpp
float AEnemyCharacter::TakeDamage(...)
{
    if (bIsDead || !Health) return 0.f;

    if (GetEnemyRealmType() == ERealmType::Realm)
    {
        if (const URealmHurtSwitchComponent* HurtSwitch = Cast<URealmHurtSwitchComponent>(RealmTag))
        {
            if (!HurtSwitch->IsHurtable())
            {
                return 0.f;  // 玩家在表世界 / 敌人在揭示圈外 → 拒绝伤害
            }
        }
    }

    return Health->ApplyDamage(Damage, DamageCauser);
}
```

`URealmHurtSwitchComponent::IsHurtable` 的语义：
- `RealmType == Surface` → `bInsideCircle ? false : true`（与表世界默认相反，本基类对 Surface 不参考此值，故无影响）
- `RealmType == Realm`   → `bInsideCircle ? true : false`（敌人在揭示圈内 = 实体态）

由于 RealmRevealer 几乎只在玩家身上活动，"敌人在圈内" 等价于"玩家位于里世界且能看见这个敌人"。

**与 §5.6 的协同**：敌人本体不再被切碰撞 → Hitbox 永远在线，能正常打到玩家（满足"敌人在任何世界都能攻击玩家"）；子弹通过 §5.6 的 RealmTag 禁 Tick + Pawn 通道 Overlap 双重处理，跨世界都能命中玩家、且穿透其他敌人。

**与 `AGhostMeleeEnemy` 的关系**：Ghost 早期实现里自己 `PostInitializeComponents` 销毁基类 RealmTag 再 new 一个 HurtSwitch、`TakeDamage` 自己判 `IsHurtable`。基类升级后这两段逻辑都变成"防御性空转"——基类已经创建好 HurtSwitch、已经在 TakeDamage 里做判定。Ghost 的 override 行为仍正确，不破坏，但已经冗余，后续可清理（删除 Ghost 的 PostInitComponents 和 TakeDamage override，仅保留 `DefaultRealmType = Realm`）。

---

## 6. 扩展指南

### 6.1 加一种新敌人

以"投掷敌人"为例：

1. **C++**
   - 新建 `Source/UEGameJam/Enemy/Thrower/` 下三个文件：
     - `ThrowerEnemy.h/.cpp` — 继承 `AEnemyCharacter`，加 `Hand` SceneComponent、`ThrowProjectile` 方法；override `ApplyDataAsset` 读取专属字段
     - `ThrowerEnemyDataAsset.h/.cpp` — 继承 `UEnemyDataAsset`，加 `WindupDuration` / `ThrowImpulse` 等
     - `ThrowerEnemyTasks.cpp` — 实现 `FEnemyThrowerWindupTask` 等专属 Task（声明统一写在 `StateTree/EnemyStateTreeTasks.h` 里，和三个已有敌人一致）
   - 在 `UEGameJam.Build.cs` 的 `PublicIncludePaths` 加 `"UEGameJam/Enemy/Thrower"`
2. **编辑器**
   - 建 `BP_ThrowerEnemy`，Class Defaults 里 AI Controller Class = `AEnemyAIController`，Enemy Data 指向下面的 DA
   - 建 `DA_ThrowerEnemy_Default`（UThrowerEnemyDataAsset），填字段，`StateTreeAsset` 指向下面的 ST
   - 建 `ST_ThrowerEnemy`，Schema = State Tree AI Component Schema，Context Actor Class = `AThrowerEnemy`，AI Controller Class = `AEnemyAIController`；参考已有树搭状态

### 6.2 加一种通用 Task

统一写在 `EnemyStateTreeTasks.h/.cpp` 里，不要分散到敌人专属目录。Task 写作约定：

1. `USTRUCT` InstanceData + `using FInstanceDataType = ...`
2. `Category="Context"` 字段（TObjectPtr）由 Schema 自动绑定
3. `Category="Input"` 字段需要在节点上手动绑到其他 Task 的 Output
4. 需要 Tick 时构造函数里 `bShouldCallTick = true`
5. `#if WITH_EDITOR` 里实现 `GetDescription`
6. InstanceData 含 `TObjectPtr` 就**不要**加 `STATETREE_POD_INSTANCEDATA` 宏

### 6.3 加一种新 Condition

模板：

```cpp
USTRUCT()
struct FEnemyXxxConditionInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Context")
    TObjectPtr<AEnemyCharacter> Enemy;

    // ... 其他字段 ...

    UPROPERTY(EditAnywhere, Category="Parameter")
    bool bInvert = false;   // ← 默认都加，因为 UE 5.6 StateTree 没有原生 Invert
};

USTRUCT(meta=(DisplayName="Enemy: Xxx", Category="Enemy"))
struct UEGAMEJAM_API FEnemyXxxCondition : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    using FInstanceDataType = FEnemyXxxConditionInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

bool FEnemyXxxCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (/* 边界：Context 不完整 */) { return Data.bInvert; }
    const bool bRaw = /* 原始判定 */;
    return Data.bInvert ? !bRaw : bRaw;
}
```

### 6.4 接入胜利条件

在 GameMode / UI 蓝图 / 任意 Actor BeginPlay 里：

```
UEnemySubsystem::Get(Self) → Bind Event On All Enemies Dead → <显示胜利 UI>
```

**注意**：`OnAllEnemiesDead` 只广播一次。监听者要在第一个敌人 Spawn 之前就 Bind 好（否则可能错过）。

---

## 7. 已知局限与后续

### 7.1 Projectile Damage 字段未从 DataAsset 贯通

`UPistolEnemyDataAsset::ProjectileDamage` / `UMachineGunEnemyDataAsset::BulletDamage` 字段存在但未消费。当前实际伤害读子弹蓝图 Class Defaults 里的 `AEnemyProjectile::Damage`。修复方式：在 `APistolEnemy::FireProjectile` / `AMachineGunEnemy::FireOneBullet` Spawn 子弹后：

```cpp
if (Proj) {
    Proj->Damage = Data->ProjectileDamage;  // 或 Data->BulletDamage
    Proj->InitializeAndLaunch(...);
}
```

需要把 `AEnemyProjectile::Damage` 从 protected 改为 public 或加 setter。MVP 不强制。

### 7.2 Hitbox / Projectile 的 PlayerTag 硬编码

`AMeleeEnemy::OnMeleeHitboxBeginOverlap` 直接写死 `ActorHasTag(FName("Player"))`；`AEnemyProjectile::PlayerTag` 默认 `"Player"`。`AEnemyAIController::PlayerTag` 也是 `"Player"`。三处都要改时可能忘。将来可以把 Tag 统一抽到一个 Settings / Subsystem。

### 7.3 AcquireTarget 的 LineOfSight 参数未被 StateTree 使用

`FEnemyAcquireTargetTaskInstanceData::bRequireLineOfSight` 代码里实现了（`EnemyStateTreeTasks.cpp:96-113`），但三个敌人的 StateTree 目前都保持 false。如果要做"躲墙后不被发现"的玩法，直接在节点上勾上即可。

### 7.4 死亡动画

目前死亡效果是 Ragdoll。要接入死亡 Montage 时，在 `AEnemyCharacter::Die` 里判断是否有 DeathMontage，有就 Play Montage + 延后 Ragdoll 到 Montage 结束。

### 7.5 分波 Spawn

`UEnemySubsystem::OnAllEnemiesDead` 的"只触发一次"逻辑是写死的。做分波需要引入 Wave ID，在每波开始时重置 `bAnnouncedAllDead`，并新增 `OnWaveCleared(WaveId)` 事件。

---

## 8. 新接手时必读

1. **用户用中文沟通**，技术名词保持英文。
2. **文件操作用完整绝对 Windows 路径**（`E:\Repo\GmaeJam_ue5.61\...`），不要用 `/e/` 或相对路径。
3. **修改边界**：敌人模块 `Source/UEGameJam/Enemy/` 内部可以随便改；**不要动** `Source/UEGameJam/Realm/` / `Variant_Horror/` / `Variant_Shooter/` / `Character/`，这些不属于本次协作者。要跨模块时先跟用户确认。
4. **UE 版本 5.6.1** — StateTree Condition 无原生 Invert，新建 Condition 一律加 `bInvert`（见 §5.2）。
5. **调试优先用屏幕打印**：`GEngine->AddOnScreenDebugMessage(-1, seconds, color, msg)`。现有 Task 的打印颜色约定：
   - AcquireTarget 绿 / 银（Found / Lost）
   - MoveToTarget 蓝（ENTER / ARRIVED）/ 红（Exit / Fail）
   - FacePlayer 紫
   - WaitPhase 青
   - MeleeDash 橙、MeleeSwing 黄
6. **Pistol / MG 的 Niagara 资产约定**：System 必须有一个 `BeamEnd` 用户参数（Position 或 Vector3），C++ 的 Tick 每帧刷它；Pistol 还需要 `FlickerIntensity` (Float)。拼错名字 C++ `SetVariableVec3` 静默失败。
7. **玩家必须挂 Actor Tag `"Player"`**，三处代码都靠它识别玩家（AIController / Projectile / Melee Hitbox）。
8. **Realm 系统集成注意**：
   - 敌人本体由基类自动用 `URealmHurtSwitchComponent` 替换默认 RealmTag（不切 SetActorEnableCollision），跨世界规则由基类 `TakeDamage` 按 `RealmType` 处理（见 §5.8）。继承 `AEnemyCharacter` 即可，蓝图无需额外配置。
   - 任何"敌人附属物"（子弹、AOE、抛物体）如果挂了 `URealmTagComponent`，**必须**用 §5.6 的禁 Tick + 强制 `SetActorEnableCollision(true)` 方案，否则会因为靠近玩家被关碰撞。
   - 子弹要支持"穿透其他敌人但能命中玩家与墙"，参照 §5.6 方案 B：Sphere 用 `BlockAllDynamic` + `ECC_Pawn` 通道改 Overlap，并同时绑 OnHit（处理墙）和 OnBeginOverlap（处理 Pawn 中的玩家）。

---

## 9. 模块依赖（`UEGameJam.Build.cs`）

```
PublicDependencyModuleNames += {
  AIModule, NavigationSystem, GameplayTasks,
  StateTreeModule, GameplayStateTreeModule,
  Niagara
}

PublicIncludePaths += {
  UEGameJam/Enemy,
  UEGameJam/Enemy/StateTree,
  UEGameJam/Enemy/Melee,
  UEGameJam/Enemy/Pistol,
  UEGameJam/Enemy/MachineGun
}
```
