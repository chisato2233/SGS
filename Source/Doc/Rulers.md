# Rulers.md — 项目级编码约束

> **本文档为长期、稳定的编码约束，所有 Agent 必读。**
> 项目全貌 / 技术栈 / 当前状态见根目录 `ProjectBrief.md`。
> 某一次开发的临时计划见 `Source/Doc/Plan/NNNN-*.md`。
> 代码结构 / 依赖关系由 **graphify** 维护，不在文档里手写。

---

## 1. 工作流程

### 1.1 启动协议

每个新会话按**严格顺序**：

1. 读根目录 `ProjectBrief.md`（项目全貌、技术栈、当前状态）
2. 读本文件 `Source/Doc/Rulers.md`（项目级编码约束）
3. 读 `Source/Doc/Plan/README.md`，按 `ProjectBrief.md` 第 5 节「当前项目状态」的指向，读对应 `Source/Doc/Plan/NNNN-*.md`
4. **需要理解既有代码结构 / 依赖关系时，以 graphify 的产物为准**

> **不必读全库**。这是文档体系存在的全部理由。
> `ProjectBrief.md` = 项目是什么；`plan.md` = 本期为什么这么做；graphify = 代码现在长什么样。三者职责互补，不重复。

### 1.2 提交协议

每完成一个有意义的步骤：

- 勾选 / 更新对应计划文档（`Source/Doc/Plan/NNNN-*.md`）的任务清单
- 必要时把决策写入该计划文档的「进度与决策记录」节
- **不要**直接改 `ProjectBrief.md`（除非阶段切换 / Plan 切换，见其第 5 节约定）
- **不要**修改 `Source/Doc/Plan/0000-RawRequirements.md` 的历史条目（只在顶部追加）

### 1.3 引入新约束的协议

发现需沉淀的新项目级约束（例如「所有 UMG 必须重写 NativePreConstruct」）：

1. 先在当前计划文档的「进度与决策记录」中暂存
2. 该 Plan 完成、归档时，提升为本文件的正式条目
3. 重大约束变更需征得用户同意

### 1.4 不要做的事

- ❌ 不要修改 `Source/Doc/Plan/0000-RawRequirements.md` 的历史条目（只追加）
- ❌ 不要在文档里手写代码结构 / 类图 / 依赖关系（交给 graphify）
- ❌ 不要为了「看起来工作多了」加冗余测试 / 注释 / 日志
- ❌ 不要绕过计划文档偷偷做计划外变更
- ❌ 不要在没有对应计划文档的情况下开始一个有规模的任务

---

## 2. UE C++ 编码规范

### 2.1 命名

- **类前缀**：`U`（UObject）/ `A`（AActor）/ `F`（POD/Struct）/ `I`（接口）/ `S`（Slate）/ `E`（Enum）
- **业务前缀**：项目类一律加业务 / 领域前缀避免和引擎/插件冲突：
  - ✅ `UCardEffect`, `UCardDefinition`, `UJudgeComponent`, `UTurnSubsystem`
  - ❌ `UCard`（太泛 / 易冲突）, `UEffect`（太泛）
- **文件名 = 类名（不含前缀）**：`UCardEffect` → `CardEffect.h` / `CardEffect.cpp`
- **函数**：PascalCase（`ApplyDamage`, `DrawCards`）
- **变量**：PascalCase（`CurrentHealth`），bool 加 `b` 前缀（`bIsAlive`）
- **常量**：PascalCase（`DefaultHandLimit`），枚举值 `EFoo::Bar` 形式

### 2.2 头文件

- 总是用 `#pragma once`，不用 include guard
- 单一职责：一个 .h 一个主类（小辅助 struct/enum 可同文件）
- 用 `class FFoo;` 前向声明，避免引入大头
- 公共 API 加 `SGS_API` 宏（项目主模块名为 SGS）

### 2.3 反射与对象

- 公开属性默认 `UPROPERTY(EditAnywhere, BlueprintReadOnly)`，BlueprintReadOnly 是为了未来 debug 工具，不代表会在蓝图里写逻辑
- 实例化子对象：`UPROPERTY(Instanced, EditAnywhere) TArray<TObjectPtr<UCardEffect>> Effects;`
- 容器 / 对象指针使用 `TObjectPtr<>` 而非裸 `*`（UE 5+ 标准）
- ❌ 不用 `BlueprintCallable` / `BlueprintNativeEvent` 除非确实需要蓝图调用（本项目核心逻辑纯 C++）
- 不要为了「提供蓝图扩展点」提前加 BlueprintNative—— YAGNI

### 2.4 模块依赖

`Source/SGS/SGS.Build.cs` 当前已含：
`Core, CoreUObject, Engine, InputCore, EnhancedInput`

按需追加：
- `UMG`, `Slate`, `SlateCore` —— 启用 UMG 纯 C++ 时
- `GameplayTags` —— 势力 / 卡牌标签
- `DeveloperSettings` —— 项目级配置

❌ **不添加 `GameplayAbilities` / `GameplayTasks` / `GameplayAbilitiesEditor`**（不上 GAS）

### 2.5 注释规则

- **不写「narrate the code」型注释**（"// 增加计数器"、"// 设置变量"）
- 只写 **why** / 不变量 / 陷阱 / TODO
- 不在文件头加 changelog / author / date（用 git）
- 不在代码里写中文长解释（用 commit message 或计划文档）

### 2.6 错误处理

- `check()` —— 不可恢复的不变量违反（断言）
- `ensure()` —— 「不该发生但能继续」的诊断
- `UE_LOG(LogCategory, Verbosity, TEXT("..."))` —— 业务日志
- 日志分类：`LogSGS`（通用）/ `LogSGSCard` / `LogSGSSkill` / `LogSGSTurn` / `LogSGSUI`
- **不用 C++ 异常**（UE 默认禁）

### 2.7 头文件组织顺序（规范化模板）

```cpp
// .h
#pragma once

#include "CoreMinimal.h"          // 必须第一个
#include "<父类头文件>.h"
// 其他 #include
// 前向声明
#include "CardEffect.generated.h" // 必须最后一个

UCLASS()
class SGS_API UCardEffect : public UObject
{
    GENERATED_BODY()

public:
    // 构造、析构
    UCardEffect();

    // 公开接口
    void Apply(...);

    // 公开属性
    UPROPERTY(EditAnywhere)
    int32 Value;

protected:
    // 子类可见

private:
    // 内部实现
};
```

---

## 3. 项目硬约束

以下约束**全局生效，不可违反**：

1. **不用 GAS**：技能 / 判定 / 结算一律自研。不引入 `GameplayAbilities` 模块。
2. **UMG 纯 C++**：`UWidget` 子类在 `NativeConstruct` 中用 `WidgetTree->ConstructWidget` 拼装。**不在编辑器 Designer 中拖控件**。少量必须在编辑器建的 Widget 资产（用于绑定动画）必须有充分理由并记录在对应 Plan 中。
3. **当前阶段单机，不写多人复制代码**：所有类不要加 `Replicated` / RPC 标记 / `bReplicates`。（`ProjectBrief.md` 提到未来可能联机，届时再单独评估；当前不写。）
4. **数据驱动优先**：卡牌 / 武将属性走 `DataTable`，效果走 C++ 效果类 + 注册表。新卡 / 新技能优先复用现成 Effect 类，除非现有 Effect 类不够用，否则**不**为单张卡写新 C++ 类。
5. **结算不依赖 wallclock**：所有时序一律按「回合 / 阶段 / 出牌次序」推进。`FTimerHandle`、`Tick` 中的 `DeltaTime` 累加都不可用于游戏逻辑（仅可用于纯表现层动画）。

---

## 4. 命名空间与日志分类

C++ 业务命名空间（可选用）：`SGS::Card::`, `SGS::Turn::` 等。MVP 阶段不强制。

日志分类**必须**在 `Source/SGS/SGSLogChannels.h` 集中声明：

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogSGS, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSCard, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSSkill, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSTurn, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSUI, Log, All);
```

---

## 5. 测试策略

- **MVP 阶段不强制单元测试**
- 关键不变量（伤害结算、回合 / 阶段推进、判定逻辑）可在需要时写 `Source/SGSTests/`（独立模块）
- 不要主动给 utility 函数补测试，除非用户明确要求

---

## 6. Git 工作流

- 仓库根目录为项目根（`SGS.uproject` 所在目录）。新会话开始、提交前、切换任务前都先看 `git status --short --branch`。
- 默认跟踪：`Source/`（含 `Source/Doc/`）、`Config/`、`.uproject`、`.vsconfig`、`.gitignore`、`.gitattributes`、项目级 IDE 任务脚本。
- 默认忽略：`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`、`.sln` / `.vcxproj*` 等 UE/IDE 生成物。
- UE 二进制资产（`.uasset` / `.umap` / 贴图 / 音频 / DCC 源文件等）走 Git LFS（规则见根目录 `.gitattributes`）。
- 提交前只 stage 本次任务相关文件；不要把用户未要求的本地改动顺手塞进提交。
- 任何 `git reset`、`git checkout -- <path>`、批量删除、改写历史操作都必须先确认意图；不要为了「干净」丢弃未知改动。

---

## 7. 提交粒度

- 一个 Plan 通常对应 1-3 个 git commit
- 单 commit **净增 ≤ 500 行**（除非纯样板/生成代码）
- commit message 包含 plan ID：
  - `[plan-0001] add UCardEffect base class`
  - `[plan-0002] implement turn phase machine`
- 中文 / 英文均可

---

## 8. 文件 / 目录创建准则

- 不主动创建 README 或文档文件，除非用户要求或本 Rulers.md 明确规定
- 不创建 `.gitkeep` / 子目录 `.gitignore`（根目录 `.gitignore` 由 Git 基础设施维护）
- 不创建 example / sample 文件
- 不创建未来用得上的「占位」文件

---

## Last Updated

2026-06-19 — 从外部项目模板适配为 SGS（三国杀）：模块名 Stuff→SGS、修正文档路径、删除「模块活文档」系统（改由 graphify 维护代码结构）、删除外部游戏术语表、明确不用 GAS / UMG 纯 C++ / 单机不写复制 / 回合制结算约束。
