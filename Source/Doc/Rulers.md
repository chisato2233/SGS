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
> `AGENTS.md` 仅为 Codex 的自动发现适配层，必须指回 `ProjectBrief.md`，不得另立项目事实源。

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
- `UMG`, `Slate`, `SlateCore` —— 启用 Native Code-first UI / SGSUI 时
- `CommonUI` —— 只有在 UI Plan 明确需要菜单栈、输入路由、手柄/键鼠提示等能力时才启用，并同步记录插件/模块变更
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
- 日志分类：`LogSGS`（通用）/ `LogSGSCard` / `LogSGSSkill` / `LogSGSTurn` / `LogSGSCombat`（伤害/回复/濒死）/ `LogSGSNet`（网络/复制/RPC）/ `LogSGSAI` / `LogSGSUI`
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
2. **Native Code-first UI**：UI 主路线为 Unreal 原生代码优先体系（Slate 为主要自定义控件底座，UMG 仅作视口/生命周期/对象系统适配，CommonUI 按需引入）。**不在编辑器 Designer 中拖控件**；如使用 `UWidget`/`UUserWidget`，必须在 C++ 中拼装控件树。项目不以 WebView/React/Vue/Noesis/Gameface 作为主 UI 技术栈，也不自研完整 Gameface/浏览器级 UI runtime，除非新 Plan 明确推翻本约束。SGSUI 只做薄封装：theme/token、通用组件、游戏组件、动画预设、状态/动作桥接；UI 只显示状态和采集输入，不承载游戏规则结算。
3. **服务器权威 + 多人/AI 并存**：游戏逻辑只在服务器执行，是唯一真相源；客户端只显示与采集输入。状态用 UE 复制（`GameState`/`PlayerState`），私密信息（如手牌）**只复制给拥有者**；玩家指令走可靠 RPC。真人与 AI 一律通过 `ISGSDecisionAgent` 接入，逻辑层**不感知**对端是人还是 AI。等待决策时逻辑层**异步挂起**，不阻塞游戏线程（应答 / 超时 → 默认或 AI 托管后恢复）。逻辑层**不反向依赖**表现层与具体网络实现。<br>（本条于 2026-06-19 由「当前阶段单机、不写复制代码」改定，原因见 Plan 0002 / RawRequirements #4。）
4. **数据驱动优先**：卡牌 / 武将属性走 `DataTable`，效果走 C++ 效果类 + 注册表。新卡 / 新技能优先复用现成 Effect 类，除非现有 Effect 类不够用，否则**不**为单张卡写新 C++ 类。
5. **结算不依赖 wallclock**：所有时序一律按「回合 / 阶段 / 出牌次序」推进。`FTimerHandle`、`Tick` 中的 `DeltaTime` 累加都不可用于游戏逻辑（仅可用于纯表现层动画）。

---

## 4. 命名空间与日志分类

C++ 业务命名空间（可选用）：`SGS::Card::`, `SGS::Turn::` 等。MVP 阶段不强制。

日志分类**必须**在 `Source/SGS/Core/SGSLogChannels.h` 集中声明：

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogSGS, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSCard, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSSkill, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSTurn, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSNet, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSGSAI, Log, All);
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
- 默认跟踪：`Source/`（含 `Source/Doc/`）、`Config/`、`.uproject`、`.vsconfig`、`.gitignore`、`.gitattributes`、`AGENTS.md`、`.codex/hooks.json`、`.githooks/`、`graphify-out/.graphify_root`、`graphify-out/graph.json`、`graphify-out/manifest.json`、项目级 IDE/Agent 任务脚本。
- 默认忽略：`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`、`.sln` / `.vcxproj*`、`graphify-out/cache/` 等 UE/IDE/graphify 缓存生成物。
- Git LFS 仅用于单文件大小 >= 50 MiB 的大文件；不要按扩展名把所有二进制资产一概放入 LFS。`.gitattributes` 只保留文本归一化和脚本生成的显式大文件 LFS 条目。
- 提交前必须运行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\CheckLargeFiles.ps1 -Scope Staged`；本地 `.githooks/pre-commit` 执行同一检查。若检查失败，运行同脚本加 `-Fix` 为对应大文件写入显式 LFS 规则并重新 stage。
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

2026-06-19 — UI 路线定为 Native Code-first UI（Slate/UMG/CommonUI 按需组合 + SGSUI 薄封装）：不使用 WebView/React/Vue/Noesis/Gameface 作为主 UI，不自研完整 Gameface；硬约束 #2 由「UMG 纯 C++」升级为「Native Code-first UI」。见 Plan 0011。
2026-06-23 — Git LFS 策略改为仅托管单文件 >= 50 MiB 的大文件；提交前通过 `Tools/CheckLargeFiles.ps1` / `.githooks/pre-commit` 检查，按需生成显式 per-file LFS 条目。
2026-06-19 — Codex 入口适配纳入文档系统：`AGENTS.md` 只作为自动发现/graphify 规则入口，必须指回 `ProjectBrief.md`；`graphify-out/graph.json` 与 manifest 作为项目级图谱产物跟踪，cache 忽略。
2026-06-19 — 架构转向：硬约束 #3 由「单机不写复制」改为「服务器权威 + 多人/AI 并存（决策代理 + 异步非阻塞）」；新增 `LogSGSNet`/`LogSGSAI` 日志分类；日志头文件路径定为 `Source/SGS/Core/`。见 Plan 0002。
2026-06-19 — 从外部项目模板适配为 SGS（三国杀）：模块名 Stuff→SGS、修正文档路径、删除「模块活文档」系统（改由 graphify 维护代码结构）、删除外部游戏术语表、明确不用 GAS / UMG 纯 C++ / 单机不写复制 / 回合制结算约束。
