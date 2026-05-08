# 敌人动画蓝图适配交接文档

> 承接 `Handoff.md`。本文覆盖 2026-05-08 这次把敌人动画蓝图从"借玩家 ABP 凑活 / 完全没挂"迁到"基于 Variant_Shooter 示例 ABP 复制定制"的工作。
> 下一位 agent 读完 `Handoff.md` §1–§5 后再读本文即可无缝继续。
>
> 工作方式：通过 UnrealBridge（`E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\scripts\bridge.py`）在运行中的 UE 编辑器内完成全部资产改动，C++ 改动走 Live Coding 热补丁。

---

## 1. 需求背景（为什么做这事）

美术希望敌人动画蓝图"照着 Variant_Shooter 示例敌人的动画蓝图改一份"，这样后续美术做资源时：

- 骨骼保持 `SK_Mannequin`（小白人），不折腾 retarget
- 直接替换 ABP 里引用的 `AnimSequence` / `AimOffset BlendSpace` 资产即可换风格，不用懂 ABP 结构
- 攻击 / 死亡等需要播 Montage 的动作，未来美术做好 Montage 后，代码侧在对应 Task / `Die()` 加一行 `Montage_Play` 就能接上

核心约束：**敌人行为和示例项目不一样**（StateTree 驱动、站桩射击、Realm 双层、无武器 actor），所以 ABP 的数据契约必须对得上我们敌人的运行时状态，而不是照抄。

---

## 2. 调查结论（这次改动的理论依据）

### 2.1 示例 ABP 的数据契约

`ABP_TP_Rifle` / `ABP_TP_Pistol` 的父类是**纯 `UAnimInstance`**（不是自定义 C++ AnimInstance）。
EventGraph 里的 9 个变量全部通过 `TryGetPawnOwner → Cast<Character>` 自行取数，不依赖任何 `AShooterNPC` 字段：

| 变量 | 数据源 | 我们敌人能否提供 |
|---|---|---|
| `Velocity` | `Pawn.GetVelocity()` | ✅ 来自 `CharacterMovement` |
| `GroundSpeed` | `Velocity.Size2D()` | ✅ |
| `ShouldMove` | `GroundSpeed > 阈值 && !IsFalling` | ✅ |
| `IsFalling` | `Movement->IsFalling()` | ✅ |
| `Direction` | `CalculateDirection(Velocity, ActorRotation)` | ✅ |
| `PitchN` / `AimOffset` | `GetControlRotation()`（主要取 Pitch） | ⚠ 只缺 Pitch |
| `Character` / `MovementComponent` | 指针缓存 | ✅ 自动 |

**结论**：示例 ABP **对任何 `ACharacter`** 都可用，无需自定义 C++ AnimInstance。唯一 gap 在 `GetControlRotation().Pitch`——`AAIController` 默认 `bSetControlRotationFromPawnOrientation=true` 只同步 Yaw，Pitch 恒为 0 → 上半身永远水平。

### 2.2 Yaw 已经通了（不改 AIController 也对）

敌人 pawn 的 Yaw 靠 `FEnemyFacePlayerTask` 用 `RInterpConstantTo` 改 actor Yaw；`AAIController` 每帧 `UpdateControlRotation` 会把 `ControlRotation.Yaw` 同步到 pawn orientation。所以 `AimOffset.Yaw ≈ 0`（上半身跟身体对齐），这正是我们要的。

### 2.3 Slot 问题

示例 `ABP_TP_Rifle` / `ABP_TP_Pistol` 的 Slot 数为 **0**——示例项目里射击动画是**武器 actor 自己播的**，NPC 身上不放上半身 recoil Montage。我们没有武器 actor 这一层，且未来希望播：

- Melee Swing Montage（近战挥砍）
- Fire Recoil Montage（远程开火反冲）
- Death Montage（见 `Handoff.md` §7.4）

所以复制出来的 Rifle / Pistol 需要额外**塞一个 `DefaultSlot`**。`ABP_Unarmed` 本身自带 `DefaultSlot`，Melee 变体直接复用。

### 2.4 当前敌人 BP 的动画挂载现状（改动前）

| BP | Mesh | AnimClass（改动前） |
|---|---|---|
| `BP_MachineGunEnemy` | `SKM_Manny_Simple` | **空** |
| `BP_PistolEnemy` | `SKM_Manny_Simple` | **空** |
| `BP_MeleeEnemy` | `SKM_Manny_Simple` | `ABP_GsPlayer`（借玩家 ABP） |
| `BP_GhostMeleeEnemy` | `SKM_Manny_Simple` | `ABP_GsPlayer`（借玩家 ABP） |

Mesh transform 已全部按 mannequin 标准对位（`Z=-90`, `Yaw=-90`）——美术那边已经给够基础，我们只补动画蓝图层。

---

## 3. 最终方案（采纳的决策）

用户三问三答都选推荐：

1. **AimOffset.Pitch**：改 `AEnemyAIController`，Tick 里补 Pitch
2. **Montage Slot**：Rifle / Pistol 的新 ABP 加 `DefaultSlot`
3. **Ghost ABP**：Ghost 与 Melee **共用** `ABP_Enemy_Melee`（§5.8 已说明 Ghost 在行为上就是 Melee 的 Realm 变体）

备选 / 没走的路：
- ❌ 自定义 C++ `UAnimInstance` 子类（示例 ABP 的数据契约已经够用，过度工程）
- ❌ 彻底删掉 ABP 里的 `AO_Rifle`/`AO_Pistol` 节点（选这个会永久放弃瞄准偏移，不值）
- ❌ 给 Ghost 单独做 `ABP_Enemy_GhostMelee`（目前 Ghost 视觉差异靠材质 + Niagara，不需要独立骨骼动画）

---

## 4. 产物清单（这次改了什么）

### 4.1 新建资产

目录：`/Game/Enemy/Anims/`（新建）

| 资产 | 来源 | 差异 |
|---|---|---|
| `ABP_Enemy_Rifle` | `ABP_TP_Rifle` 复制 | AnimGraph 在 `LayeredBoneBlend → Output Pose` 之间插入了 `Slot 'DefaultSlot'` |
| `ABP_Enemy_Pistol` | `ABP_TP_Pistol` 复制 | 同上 |
| `ABP_Enemy_Melee` | `ABP_Unarmed` 复制 | 原版已自带 `DefaultSlot`，未修改结构 |

目标骨骼全部是 `SK_Mannequin`。全部编译通过。

### 4.2 敌人 BP 的 AnimClass 绑定（`CharacterMesh0.AnimClass`）

| BP | AnimClass |
|---|---|
| `BP_MachineGunEnemy` | `ABP_Enemy_Rifle_C` |
| `BP_PistolEnemy`     | `ABP_Enemy_Pistol_C` |
| `BP_MeleeEnemy`      | `ABP_Enemy_Melee_C`（原 `ABP_GsPlayer` 已换掉） |
| `BP_GhostMeleeEnemy` | `ABP_Enemy_Melee_C`（原 `ABP_GsPlayer` 已换掉） |

### 4.3 C++ 改动

`Source/UEGameJam/Enemy/EnemyAIController.h`：

```cpp
virtual void Tick(float DeltaSeconds) override;
```

`Source/UEGameJam/Enemy/EnemyAIController.cpp`：

1. 构造函数新增两行（开 Tick；`AController` 虽然默认有 Tick，但我们显式打开更稳）：
   ```cpp
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   ```

2. 新增 Tick 实现。要点：
   - `Super::Tick` 会把 `ControlRotation.Yaw` 同步到 pawn orientation（默认行为不改），Pitch 不动
   - 我们在 `Super` 之后盖写 Pitch：`Atan2(Dir.Z, Dist2D)` 求玩家相对敌人的仰角，clamp 到 `±60°`
   - 没缓存玩家 / 缓存玩家刚好重叠 / pawn 不存在 → Pitch 归零
   - 敌人死亡会 `UnPossess` → `GetPawn()` 返回 nullptr → Tick 早退，不需要显式死亡判断

没有新增 `UPROPERTY` / `UFUNCTION` / `UCLASS` / `USTRUCT`，纯函数体 + virtual override → **Live Coding 即可热补**，无需 `rebuild_relaunch`。

---

## 5. 数据契约映射表（给下一位 agent 查）

ABP 里的变量 ← 运行时数据源（敌人这边）：

| ABP 变量 | 运行时源 | 位置 |
|---|---|---|
| `Character` | `Cast<ACharacter>(TryGetPawnOwner())` | ABP EventGraph 自动 |
| `MovementComponent` | `Character->GetCharacterMovement()` | ABP EventGraph 自动 |
| `Velocity` | `Character->GetVelocity()` | ABP EventGraph 自动 |
| `GroundSpeed` | `Velocity.Size2D()` | ABP EventGraph 自动 |
| `ShouldMove` | `GroundSpeed > 阈值 && !IsFalling` | ABP EventGraph 自动 |
| `IsFalling` | `MovementComponent->IsFalling()` | ABP EventGraph 自动 |
| `Direction` | `CalculateDirection(Velocity, ActorRotation)` | ABP EventGraph 自动 |
| `PitchN` / `AimOffset.Pitch` | `GetControlRotation().Pitch` ← `AEnemyAIController::Tick` 盖写 | **本次加的**补丁 |
| `AimOffset.Yaw` | `GetControlRotation().Yaw` ← `AAIController` 自动从 `FEnemyFacePlayerTask` 改的 actor Yaw 同步 | 不改 |

---

## 6. 美术"换资源"工作流

美术以后做新资源时：

### 6.1 换 locomotion 姿势（Idle / Run / Jump）

直接在 `ABP_Enemy_*` 对应 state 的 Sequence Player 里：
- 在 Content Browser 选中 ABP 双击打开
- `AnimGraph → LocomotioStateMachine`（或 Unarmed 版的 `Main States → Locomotion`）双击进去
- 每个 State 内部的 Sequence Player 换 `Animation` 引用
- Compile → Save

或者更省事：**把 ABP 现在引用的那些 AnimSequence 资产**（例如 `MF_Rifle_Run`）直接用美术自己做的同名 AnimSequence 覆盖，ABP 一行不用动。

### 6.2 换武器握持姿势 / AimOffset

| ABP | 握持姿势 Sequence | AimOffset BlendSpace |
|---|---|---|
| `ABP_Enemy_Rifle` | `MF_Rifle_Idle_ADS` | `AO_Rifle` |
| `ABP_Enemy_Pistol` | `MF_Pistol_Idle_ADS` | `AO_Pistol` |
| `ABP_Enemy_Melee` | 走 Locomotion state，无单独 ADS | — |

同样在 ABP 里替换引用即可。

### 6.3 接 Montage（Swing / Fire / Death）

Slot 已就位（三个 ABP 都有 `DefaultSlot`）。流程：

1. 美术做好 Montage，Slot 选 `DefaultSlot`
2. 代码侧在触发点加一行：
   ```cpp
   if (USkeletalMeshComponent* Mesh = GetMesh())
   {
       if (UAnimInstance* Anim = Mesh->GetAnimInstance())
       {
           Anim->Montage_Play(MontageAsset);
       }
   }
   ```
3. 建议的触发位置：
   - **Swing**：`FEnemyMeleeSwingTask::EnterState`（`Source/UEGameJam/Enemy/Melee/MeleeEnemyTasks.cpp`）
   - **Fire (Pistol)**：`APistolEnemy::FireProjectile`
   - **Fire (MG)**：`AMachineGunEnemy::FireOneBullet`（注意连发密度，可能需要只在 Burst 开头播一次）
   - **Death**：`AEnemyCharacter::Die`，按 `Handoff.md` §7.4 的改造方式，先播 Montage，OnMontageEnded 回调里再 Ragdoll + 启动销毁计时器

为了让 Montage 资产从 DataAsset 传进来（避免硬编码），建议在 `UMeleeEnemyDataAsset` 等加 `TObjectPtr<UAnimMontage> SwingMontage` 之类的字段。**这是新增 UPROPERTY，需要走 `rebuild_relaunch.py`，不能热补丁**。

---

## 7. 已知限制与后续

### 7.1 `AimOffset.Pitch` 的 clamp 是硬编码

`EnemyAIController.cpp` 里写死 `FMath::ClampAngle(PitchDeg, -60.f, 60.f)`。如果需要不同敌人不同 clamp（比如机枪能瞄更陡角度），把 `60.f` 提成 `UPROPERTY(EditAnywhere)` 放在 `AEnemyAIController` 或 `UEnemyDataAsset`。注意新 UPROPERTY = 反射变更，需要 `rebuild_relaunch.py`。

### 7.2 Melee 敌人的 ADS 问题

`ABP_Enemy_Melee` 来自 `ABP_Unarmed`，没有武器握持姿势层。跑起来应该是"小白人徒手姿势 + 走跑 + 跳"，没有奇怪的握枪叠加。如果美术想给 Melee 做专属的"Ready"持刀姿势，可以：

- 方案 A（省事）：直接替换 Unarmed 的 Idle Sequence
- 方案 B（干净）：在 `ABP_Enemy_Melee` 的 AnimGraph 加一个 `LayeredBoneBlend`（上半身覆盖），参照 Rifle/Pistol 的结构加 `AO_Melee`（需要美术做 AimOffset 资产）

### 7.3 Ghost 和 Melee 共用 ABP 的代价

Ghost 如果将来需要"漂浮"、"缓移"、半透等视觉差异，共用 ABP 会被拖住。目前先共用，等需求出现再分叉。

### 7.4 PIE 还没真验证

资产 / 绑定 / 编译 / 热补丁都已成功，但**没有跑 PIE 看一眼动画跑起来效果**——这是交接前没来得及做的最后一步。下一位 agent 或者用户打开 `/Game/Enemy/EnemyTestMap` 按 Alt+P，确认：

- Idle：敌人站着时有呼吸 / 持械待机动画（不是 T-pose）
- Chase：跟随玩家时下半身 Run / 上半身持枪（Rifle / Pistol）
- AimOffset Pitch：把玩家踩到高处或跳起来，远程敌人上半身应该能抬头瞄

**T-pose / 姿势全错 = AnimClass 没加载 / CDO 没生效**，优先检查：

```python
# bridge 里查 CDO
import unreal
bp = unreal.load_object(None, "/Game/Enemy/Blueprints/BP_PistolEnemy")
cdo = unreal.get_default_object(bp.generated_class())
for comp in cdo.get_components_by_class(unreal.SkeletalMeshComponent):
    print(comp.get_editor_property("anim_class"))  # 应返回 ABP_Enemy_Pistol_C
```

### 7.5 Montage 播放 API 暂未实现

本次不写 `PlayAnimMontage` 代码。Slot 留着，美术做资产+用户明确要接时再动 C++。不要提前写死一个硬编码路径的 Montage 播放调用。

---

## 8. 开发工具路径（给下一位 agent）

- **Bridge 主入口**：`E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\scripts\bridge.py`
- **Bridge skill 文档**：`E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\SKILL.md`
- **Bridge Anim API 文档**：`E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\references\bridge-anim-api.md`
- **热编译（函数体 / override 改动）**：`python E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\scripts\hot_reload.py --no-sync`
- **全量重编（新增/改 UPROPERTY/UFUNCTION）**：`python E:\Repo\UnrealBridge\.claude\skills\unreal-bridge\scripts\rebuild_relaunch.py`
- **环境变量（已写入用户级，新终端自动有）**：
  - `UE_ROOT = E:\EpicDownload\UE_5.6`
  - `UNREAL_BRIDGE_PROJECT = UEGameJam`
- **项目 Build.bat**（给 `rebuild_relaunch.py` 调的）：`E:\Repo\GmaeJam_ue5.61\Build.bat`（本次新建，薄壳转发给引擎的 Build.bat）

### 如果要重放本次改动（清单）

1. 复制 3 个示例 ABP 到 `/Game/Enemy/Anims/`（`unreal.EditorAssetLibrary.duplicate_asset`）
2. 在 Rifle / Pistol 的复制体上 `Anim.add_anim_graph_node_slot('DefaultSlot')`，断 `LayeredBoneBlend → Root`，连 `LayeredBoneBlend → Slot → Root`，`Anim.auto_layout_anim_graph`
3. `Editor.compile_blueprints` + `EditorAssetLibrary.save_asset` 所有新 ABP
4. 4 个敌人 BP 的 `CharacterMesh0` 组件 `set_editor_property('anim_class', ...)`
5. `Editor.compile_blueprints` + `save_asset` 4 个敌人 BP
6. 改 `EnemyAIController.{h,cpp}`（见 §4.3）
7. `hot_reload.py --no-sync`

全部用 heredoc / Edit 工具就能完成，不需要人工点编辑器。

---

## 9. 约定与风格（从 `Handoff.md` §8 继承并补充）

- **文件操作用完整绝对 Windows 路径**（`E:\Repo\GmaeJam_ue5.61\...`）
- **C++ 改动后优先热编译**（`hot_reload.py --no-sync`），不要每次都 `rebuild_relaunch` —— 只有反射变更才需要全量
- **ABP 写操作走 bridge，不要手工点编辑器**。Bridge 有 AST preflight + 事务封装，可重放
- **ABP 节点用 GUID 寻址，不要用 NodeIndex**（graph 变更会让 index 漂移）
- **修改边界**：同 `Handoff.md` §5.1—这次的改动全部在 `Source/UEGameJam/Enemy/` + `/Game/Enemy/` 内部，没有越界
