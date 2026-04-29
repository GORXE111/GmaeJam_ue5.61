# AGENTS文档

这是一个基于 Unreal Engine 5.6 的第一人称跑酷游戏，类似霓虹白客、幽灵行者。

## 项目

- 在UE官方的第一人称项目Shooter示例代码基础上进行修改
- 代码写在Source\UEGameJam\Variant_Shooter中，按模块分类，单个文件尽量不超过500行
- 

## 代码风格约定

### 命名约定

- 优先遵循UE的命名风格，如U为UObject、A为Actor 等等
- 不要用引擎父类的名字，小心声明隐藏了类成员

### 注释

- 暴露给蓝图的可调的参数需要加上中文注释，说明这个参数的作用


## 注意事项

- 完成工作后无需你测试

## Enemy 模块

敌人逻辑集中在 `Source/UEGameJam/Enemy/` 下，与 `Variant_Shooter/` 完全解耦（不相互 include）。基类继承 UE 原生 `ACharacter`、`AAIController`，行为用 StateTree + C++ 驱动。

子目录职责：
- `Core/`：`AEnemyCharacter`、`AEnemyAIController`、类型与威胁视觉接口
- `Subsystem/`：`UEnemyManagerSubsystem`（胜利判断查此处拿计数/订阅死亡）、`UEnemyThreatBroker`（UI 威胁指示器的数据源）
- `Components/`：血量 / 死亡守卫 / 核心暴露检测
- `Projectile/`：手枪与机枪共用的投射物
- `Enemies/`：四种敌人的具体子类（手枪 / 机枪 / 近战里世界 / 重装）
- `StateTree/`：C++ Task 与 Condition，StateTree 资产留给 Editor 搭建
- `Debug/`：`LogEnemyModule` 日志分类 + `r.Enemy.DebugThreat` CVar

架构计划文件（含端到端验证步骤与每个敌人的 StateTree 搭法）：`C:\Users\yixiao.liu\.claude\plans\iridescent-rolling-frost.md`。