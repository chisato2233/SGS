# 0014 — Rule 层长期重构：Typed Invocation、索引注册表与结算栈

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-03 |
| 最近更新 | 2026-07-03 |
| 关联需求 | `0000-RawRequirements.md` 中的第 14 条 |
| 子计划 | `0014-M2-typed-rules-resolution-stack.md`、`0014-M3-resolution-stack-resume-timing-event.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Commands/`、`Source/SGS/Server/Engine/`、`Source/SGS/Server/Effects/`、`Source/SGS/Server/Timing/`、`Source/SGS/Shared/Core/SGSIndexedStore.h` |

---

## 1. 目标

重构当前 Plan 0013 建立的最小 `Command -> Rule -> EffectPipeline` 管线，使 Rule 层具备长期承载大量卡牌、锦囊、装备、武将技和模式规则的能力。

本计划完成后应达到：

1. `FSGSRuleRequest` 不再继续演化为“万能参数包”；Command 翻译为 typed `RuleInvocation`，不同规则意图携带自己的 payload。
2. Rule 不再只有基础牌执行分支，而是形成稳定分类：`ActionRule`、`TriggerRule`、`ModifierRule`、`ViewAsRule`，分别承载主动动作、时机触发、查询修正和视为 / 转化能力。
3. `FSGSRuleRegistry` 使用 `TSGSIndexedStore` 管理 `RuleEntry`，按 `RuleDescriptor` 多索引查询候选规则，避免所有 Rule 线性盲扫。
4. `USGSGameDriver` 不再保存不断膨胀的 pending 规则状态；响应窗口、处理中牌、濒死求桃、后续 continuation 进入显式 `ResolutionStack / ResolutionFrame`。
5. 状态变更继续只通过 `EffectPipeline -> USGSGameContext`，保持服务器权威、ReplayLog、随机审计和自动化验收可追溯。
6. Plan 0005 / 0013 的杀 / 闪 / 桃 / 濒死求桃行为保持等价，并通过现有自动化回归。

本计划的核心不是多写几张牌，而是建立后续大量规则可以安全接入的结构。

## 2. 背景与约束

### 2.1 当前问题

Plan 0013 已完成基础迁移：`CommandRouter` 校验命令，`CommandType` 构造 `FSGSRuleRequest`，`RuleRegistry` 顺序匹配 `ISGSRule`，基础牌通过 `EffectPipeline` 修改权威状态。

本节记录的是 0014 创建时的问题背景。2026-07-03 的 M2 核心实现后，`FSGSRuleRequest` 运行时代码路径已经删除，基础牌 Rule 已迁移到 typed payload，Driver 基础牌 pending 状态已迁入 `ResolutionStack`；剩余风险主要转为测试覆盖、Trigger / Modifier / ViewAs 分类和后续内容扩展。

当前实现可完成最小纵切，但长期风险已经显现：

- `FSGSRuleRequest` 当前包含 `CommandType`、`CardId`、`CardName`、`TargetSeatIndices`、`WindowName`、`RequiredCardName`、`EffectSourceSeat`、`EffectTargetSeat`、`bIsPass`。继续加入锦囊、装备、技能、判定、伤害修正、目标修正后会参数爆炸。
- `FSGSRuleRegistry::Resolve` 目前顺序遍历所有 Rule，以 `CanHandle` 逐个试探。规则数量增长后会难以审计、难以排序、难以表达全局 / wildcard / modifier 规则。
- `USGSGameDriver` 当前保存 `PendingWindowName`、`PendingProcessingCard`、`PendingDyingResponders` 等规则运行时状态。响应嵌套、技能连锁和多段结算出现后，Driver 会再次变成规则状态中心。
- `SGSBasicCardRules.cpp` 已聚集 Pass / Slash / Dodge / Peach / DyingPeach。若后续锦囊和技能继续照此堆叠，会出现新的巨型规则文件。

### 2.2 外部参考

本计划参考 QSanguosha 的技能架构，但不照搬：

- 可借鉴：C++ 定义稳定技能抽象，具体技能按触发事件、视为技、距离 / 目标修正等接口接入；触发系统按事件建立表，按优先级执行 `triggerable / cost / effect`。
- 不照搬：Lua / 字符串 / QVariant 风格的弱类型数据包；脚本直接持有 `Room` 并修改状态；大量 flag / tag 作为隐式控制流。

SGS 的方向是：保留 C++ typed payload、GameplayTag / FName 开放标识、`EffectPipeline` 审计和服务器权威边界。未来若引入脚本，也只能作为 Rule 接口实现层，不允许绕过 `EffectPipeline` 直接改 `GameContext`。

### 2.3 约束

- 不破坏现有 Command 入口。真人、AI、未来 RPC、回放仍统一进入 `FSGSCommandRouter`。
- 不把 Command payload 直接当 Rule payload 使用。Command payload 是外部意图协议，Rule payload 是服务器翻译后的内部规则调用数据。
- 不使用封闭 enum 表达卡牌名、技能名、阶段、时机、效果类型等可扩展概念。
- 不绕过 `USGSGameContext`、`FSGSEffectPipeline`、`FSGSReplayLog` 和 `FSGSRandomAudit`。
- 不在本计划中引入 Lua 或其他脚本 runtime。
- 重构必须保持 Plan 0005、0011-M1、0013 相关自动化回归通过。

## 3. 方案设计

### 3.1 概念分层

建立以下稳定概念：

- `FSGSCommand`：外部意图信封。表示玩家 / AI / RPC / 回放提交了什么。
- `CommandPayload`：外部协议 payload，不可信，只描述输入意图。
- `FSGSCommandExecutionContext`：服务器当前期待谁、在哪个请求、哪个阶段、哪个窗口中行动。
- `FSGSRuleInvocation`：服务器内部规则调用。由已校验 Command 和当前上下文翻译而来。
- `RuleInvocationPayload`：规则调用的 typed payload。不同意图使用不同 payload，避免万能 struct。
- `FSGSRuleDescriptor`：Rule 的可索引描述，说明它处理哪类意图、时机、对象、窗口和优先级。
- `FSGSRuleEntry`：Registry 内部条目，包含 descriptor、Rule 实例、注册顺序和稳定 handle。
- `FSGSResolutionFrame`：一次正在结算的规则帧，保存处理中对象、目标、窗口、continuation 和 typed frame state。
- `FSGSResolutionStack`：结算栈，支持响应窗口、技能嵌套、后续结算恢复。
- `ISGSRuleRuntime`：Rule 与执行环境之间的受控门面。Rule 不直接改状态，只通过 Runtime 请求 effect、窗口、trigger 和 continuation。

目标调用路径：

```text
FSGSCommand
  -> FSGSCommandRouter::SubmitCommand
  -> ISGSCommandType::BuildRuleInvocation
  -> FSGSRuleRegistry::Resolve
  -> ISGSRule / typed rule base
  -> FSGSResolutionStack / ISGSRuleRuntime
  -> FSGSEffectPipeline
  -> USGSGameContext
```

### 3.2 Typed RuleInvocation

替换当前扁平 `FSGSRuleRequest` 的长期形态。

建议结构：

```text
FSGSRuleInvocation
  InvocationId
  RuleKind
  IntentTag
  SubjectName / SubjectTag
  ActorSeat
  SourceCommandId
  SourceRequestId
  TimingPoint
  WindowName
  FrameHandle
  Payload: FInstancedStruct
```

首批 payload：

- `FSGSPassRulePayload`：是否为窗口 pass、pass reason、当前窗口 / frame 引用。
- `FSGSUseCardRulePayload`：CardId、CardName、CardState 快照所需字段、目标座位列表。
- `FSGSRespondCardRulePayload`：CardId、CardName、响应窗口、RequiredCardName、响应目标 / 来源 frame。
- `FSGSTriggerRulePayload`：TimingPoint、事件数据句柄或 typed event data、触发候选 owner。
- `FSGSModifierRulePayload`：查询类型、base value、source / target / card / skill 上下文。
- `FSGSViewAsRulePayload`：选择的原始牌、期望响应 pattern、生成的虚拟牌 / 技能意图。

迁移原则：

- `CommandType` 从 `BuildRuleRequest` 改为 `BuildRuleInvocation`。
- 现有 Pass / UseCard / RespondCard 先用首批 payload 实现等价迁移。
- Rule 读取 payload 时通过 typed helper，不手动猜字段。

### 3.3 Rule 分类与派生基类

保留 `ISGSRule` 作为共同接口，但新增更贴近玩法的基类：

```text
ISGSRule
  GetDescriptor()
  CanHandle()
  Validate()
  Execute()

TSGSTypedRule<TPayload>
  CanHandleTyped()
  ValidateTyped()
  ExecuteTyped()

FSGSActionRuleBase
FSGSCardActionRuleBase
FSGSResponseRuleBase
FSGSTriggerRuleBase
FSGSModifierRuleBase
FSGSViewAsRuleBase
```

派生类职责：

- `ActionRule`：处理主动动作，如使用牌、装备牌、发动主动技能。
- `TriggerRule`：处理时机触发，如伤害前、造成伤害后、摸牌阶段开始、牌移动后、进入濒死。
- `ModifierRule`：修正查询结果，如距离、目标数、使用次数、手牌上限、是否禁止成为目标。
- `ViewAsRule`：处理视为 / 转化，如把一张红桃牌当桃、把两张牌当决斗、响应窗口内虚拟闪。

基类负责共性：

- payload 类型检查。
- RuleDescriptor 基础匹配。
- 牌是否存在、区域归属、座位存活、窗口是否匹配等通用校验。
- 统一错误码和日志字段。

具体规则只覆写玩法差异，例如 Slash 的距离和响应窗口、Peach 的治疗目标、Dodge 的抵消逻辑。

### 3.4 IndexedStore 驱动的 RuleRegistry

使用 `TSGSIndexedStore<FSGSRuleEntry>` 重构 Registry。

建议 `FSGSRuleDescriptor` 字段：

```text
RuleName
RuleKind
IntentTag
SubjectName
SubjectTag
TimingTag
WindowName
OwnerSkillName
Priority
RegistrationOrder
bGlobal
bWildcardIntent
bWildcardSubject
bWildcardWindow
```

建议索引：

- `ByRuleName`：唯一索引。
- `ByKind`：Action / Trigger / Modifier / ViewAs。
- `ByIntent`：UseCard / RespondCard / Pass / Trigger / Query 等。
- `BySubjectName`：Slash / Peach / Dying.Peach / 技能名等。
- `ByTimingTag`：DamageInflicted / CardUsed / PhaseStart 等。
- `ByWindowName`：Slash.Dodge / Dying.Peach 等。
- `ByOwnerSkillName`：技能启用 / 禁用时批量查询。
- `ByLookupKey`：组合键 + priority 排序的 ordered index。

Registry 解析流程：

```text
BuildLookupKeys(Invocation)
  -> exact key
  -> wildcard subject key
  -> wildcard window key
  -> global key

QueryCandidates(keys)
  -> IndexedStore handles
  -> dedupe
  -> sort by Priority desc, RegistrationOrder asc

Resolve
  -> CanHandle
  -> Validate
  -> Execute
```

长期拆分：

- `RuleCatalog`：已加载的规则定义，偏静态。
- `ActiveRuleSet`：当前对局启用的规则，随获得技能、失去技能、装备变化而变化。

本计划第一阶段可以先在 `FSGSRuleRegistry` 内部实现单一 Store，但结构应预留两层拆分。

### 3.5 ResolutionStack 与 Frame

将 Driver 中 pending 规则状态下沉：

```text
FSGSResolutionFrame
  FrameHandle
  ParentFrameHandle
  SourceRuleName
  SourceCommandId
  ActorSeat
  SourceSeat
  TargetSeats
  ProcessingCardIds
  CurrentWindow
  ContinuationTag
  Payload / FrameState
```

```text
FSGSResolutionStack
  PushFrame
  PopFrame
  FindFrame
  GetCurrentFrame
  OpenResponseWindow
  ResumeAfterResponse
  CancelFrame
  CheckInvariants
```

迁移目标：

- `PendingWindowName`、`PendingRequiredCardName`、`PendingEffectSourceSeat`、`PendingEffectTargetSeat`、`PendingProcessingCard`、`PendingDyingResponders` 等不再散落在 `USGSGameDriver`。
- Driver 保留对局推进、决策请求分发、Pump 非重入保护等编排职责。
- RuleRuntime 通过 Stack 打开窗口、记录 continuation、恢复父结算。

### 3.6 Trigger 与 Modifier 系统

新增 RuleKind：

```text
Action
Response
Trigger
Modifier
ViewAs
GameRule
```

Trigger 流程：

```text
EffectPipeline 或 Driver 到达 TimingPoint
  -> RuleRuntime::PublishTiming
  -> Registry 查询 TriggerRule 候选
  -> TriggerRule::CanTrigger / Cost / Execute
  -> 可选打开 DecisionRequest
  -> 产生新的 RuleInvocation 或 EffectStep
```

Modifier 流程：

```text
TargetQuery / DistanceQuery / UseLimitQuery / MaxCardsQuery
  -> 构造 ModifierQuery
  -> Registry 查询 ModifierRule
  -> 按 priority 应用
  -> 返回最终 query result
```

首批可落地的 modifier：

- Slash 目标数修正。
- Slash 距离限制修正。
- 使用次数 residue 修正。
- 禁止成为某类牌目标。

本计划可以先建立接口与一两个测试 modifier，不要求完整技能库。

### 3.7 ViewAs / 技能牌设计

借鉴 QSanguosha 的 `ViewAsSkill`，但使用 typed SGS 数据：

- `ViewAsRule` 不直接修改状态，只提供“可否选择这些牌”和“生成什么虚拟规则意图”。
- UI / AI 通过 DecisionRequest 获取可选 ViewAs 入口。
- 生成结果仍变成 `FSGSRuleInvocation`，由 ActionRule / ResponseRule 执行。

示例：

```text
响应 Slash.Dodge 窗口
  -> 普通 Dodge ResponseRule 可响应
  -> 某技能 ViewAsRule 允许把一张红牌视为 Dodge
  -> Command 提交选择
  -> CommandType 翻译为 ViewAs / Respond invocation
  -> Rule 执行消耗与响应结果
```

### 3.8 对现有基础牌的迁移

现有规则迁移目标：

- `FSGSPassRule` -> `FSGSPassActionRule` / `FSGSPassResponseRule`，按窗口或 frame 区分语义。
- `FSGSSlashRule` -> `FSGSSlashCardRule`，继承 `FSGSCardActionRuleBase`。
- `FSGSDodgeResponseRule` -> `FSGSDodgeResponseRule`，继承 `FSGSResponseRuleBase`。
- `FSGSPeachRule` -> `FSGSPeachCardRule`。
- `FSGSDyingPeachRule` -> `FSGSDyingPeachResponseRule`，其 responder 队列进入 `ResolutionFrame`。

行为必须保持 Plan 0005 / 0013 等价。

## 4. 工程质量门

### 4.1 真实使用入口

本计划完成后，以下路径必须真实接入新结构：

- AI / scripted agent / LocalUI 提交 `UseCard`、`RespondCard`、`Pass`。
- `CommandRouter` 校验命令并构造 typed `RuleInvocation`。
- `RuleRegistry` 用 IndexedStore 查询候选 Rule。
- 基础牌规则通过 typed Rule 派生类执行。
- `ResolutionStack` 管理 Slash 响应窗口和 Dying.Peach 求桃窗口。
- 状态变更仍走 `EffectPipeline` 标准步骤。

### 4.2 关键不变量

- 未通过 Router 校验的 Command 不得进入 RuleInvocation。
- RuleInvocation payload 类型必须和 Rule 期望类型一致。
- Rule 不直接调用 `USGSGameContext` mutation；状态变更必须进入 `EffectPipeline`。
- Registry 查询结果必须稳定、可排序、可复现；同 priority 使用注册顺序打破平局。
- ActiveRuleSet 中的 Rule handle 失效后不能继续触发。
- ResponseWindow 必须绑定 frame、request、seat、window name 和 required pattern。
- ResolutionStack push/pop 必须成对，结算结束时不得遗留悬空 frame。
- ReplayLog 能关联 Command、Invocation、EffectStep 和 TimingPoint。

### 4.3 失败路径

- RuleInvocation 构造失败：记录 Rejected，不改变状态。
- Registry 找不到规则：记录 `SGS.Rule.NotFound`，按当前场景 fallback pass 或终止 frame。
- Rule payload 类型错误：记录 invariant violation，不执行规则。
- Response window 的迟到 / 错请求 / 错窗口命令：Router 拒绝或 fallback pass，不污染当前 frame。
- TriggerRule 执行失败：记录错误，按触发点语义决定是否中断当前 frame。
- ModifierRule 异常：该 modifier 不应用，记录诊断；不得破坏 base query。

### 4.4 未来扩展

- 锦囊：通过 CardActionRule + Trigger/Response/Modifier 接入。
- 装备：装备牌本身是 ActionRule，装备效果多为 TriggerRule / ModifierRule。
- 武将技：一个技能可以注册多个内部 Rule，包括可见技能、隐藏记录 Rule、modifier Rule、view-as Rule。
- AI：通过 RuleDescriptor / DecisionRequest 暴露可行动作和响应选择，不为 AI 写平行规则。
- 网络：RPC 只传 Command 和选择结果，不传 Rule 内部状态；服务器根据 frame 验证。

## 5. 任务拆解

- [x] 定义 `FSGSRuleKind` 开放标识方案、`FSGSRuleDescriptor`、`FSGSRuleLookupKey` 和 `FSGSRuleEntry`，避免使用封闭玩法 enum。
- [x] 引入 `FSGSRuleInvocation` 与首批 typed payload：Pass、UseCard、RespondCard。
- [x] 将 `ISGSCommandType::BuildRuleRequest` 迁移为 `BuildRuleInvocation`，保留兼容 shim 仅用于过渡测试。
- [x] 重构 `FSGSRuleRegistry`：使用 `TSGSIndexedStore<FSGSRuleEntry>` 注册 Rule，并提供 descriptor 索引查询、wildcard 查询、priority 排序和去重。
- [x] 新增 typed Rule 基类：`TSGSTypedRule<TPayload>`、`FSGSCardActionRuleBase`、`FSGSResponseRuleBase`。
- [x] 迁移 Pass / Slash / Dodge / Peach / DyingPeach 到 typed Rule 基类，保持现有行为等价。
- [x] 新增 `FSGSResolutionFrame` 与 `FSGSResolutionStack`，把 Slash.Dodge 和 Dying.Peach 的 pending 状态迁出 `USGSGameDriver`。
- [x] 改造 `FSGSDriverRuleRuntime`，使其通过 ResolutionStack 打开响应窗口、恢复 continuation、完成 frame。
- [ ] 新增 TriggerRule 基础接口和最小 Timing 发布入口，但只接入一条低风险事件用于验证。
- [ ] 新增 ModifierRule 基础接口和最小 query modifier 验证，例如 Slash 距离或目标数修正。
- [ ] 新增 ViewAsRule 设计骨架和一个隔离测试用视为响应样例，不进入正式默认牌库。
- [ ] 补齐 ReplayLog 关联字段，至少能从日志追踪 Command -> Invocation -> Rule -> EffectStep。
- [ ] 更新自动化测试：现有 Plan 0005 / 0013 回归，新 Registry 候选排序测试，ResolutionStack 嵌套 / 清理测试，Modifier / Trigger 最小验证。
- [ ] 运行 Development 构建、相关自动化和 `graphify update .`。

## 6. 验收标准

必须全部满足：

- Development 构建通过：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Unreal.ps1 -Action Build -Configuration Development
```

- 现有回归保持通过：
  - `SGS.Plan0005.BasicCards`
  - `SGS.Plan0013.RulePipeline.*`
  - `SGS.Plan0011M1.LocalUI.DecisionBridge`
  - `SGS.Plan0004.GameContextPrimitives`
- 新增自动化 `SGS.Plan0014.RuleInvocation.TypedPayload`：Pass / UseCard / RespondCard 均构造 typed invocation，payload 类型错误不会执行 Rule。
- 新增自动化 `SGS.Plan0014.RuleRegistry.IndexedLookup`：Registry 使用 descriptor 和 IndexedStore 查询候选，priority / registration order 可复现，wildcard 规则可被正确纳入。
- 新增自动化 `SGS.Plan0014.ResolutionStack.BasicWindows`：Slash.Dodge 与 Dying.Peach 由 ResolutionStack 管理，结算结束后无悬空 frame。
- 新增自动化 `SGS.Plan0014.RulePipeline.Regression`：Slash / Dodge / Peach / DyingPeach 行为与 Plan 0005 一致，ReplayLog 与 GameContext invariants 通过。
- 新增自动化 `SGS.Plan0014.Modifier.Minimal`：至少一个测试 modifier 能改变查询结果，并且未启用时结果恢复原状。
- 新增自动化 `SGS.Plan0014.Trigger.Minimal`：至少一个 TriggerRule 能在 TimingPoint 被收集并执行，失败路径有日志且不破坏主结算。
- `USGSGameDriver` 中不再保存基础牌结算专用的 pending card / dying responder 队列；这些状态由 ResolutionStack 持有。
- `FSGSRuleRequest` 不再是新增规则字段的主要扩展点；新增规则必须通过 typed invocation payload 或 descriptor 接入。
- 修改后运行 `graphify update .`，图谱能查询到新的 RuleInvocation、RuleRegistry、ResolutionStack 关系。

## 7. 进度与决策记录

- 2026-07-03：创建计划。基于 Plan 0013 完成后的讨论，决定先重构 Rule 层长期结构，再继续推进锦囊、装备和武将技。核心方向为 typed `RuleInvocation`、`TSGSIndexedStore` 驱动的 `RuleRegistry`、显式 `ResolutionStack`，并借鉴 QSanguosha 的 Trigger / ViewAs / Modifier 分类，但不引入 Lua runtime 或弱类型状态修改。
- 2026-07-03：完成第一阶段地基实现：新增 typed `FSGSRuleInvocation` 与 Pass / UseCard / RespondCard payload；`CommandType / CommandRouter` 输出 Invocation，旧 `FSGSRuleRequest` 改为兼容 shim；`FSGSRuleRegistry` 切换到 `TSGSIndexedStore<FSGSRuleEntry>` 并按 descriptor / lookup key 查询排序；基础牌 Rule 补齐 descriptor，执行逻辑仍通过 legacy request 保持行为等价。本阶段未迁移 Driver pending 状态，也未引入 Trigger / Modifier / ViewAs / ResolutionStack。
- 2026-07-03：创建第二阶段子计划 `0014-M2-typed-rules-resolution-stack.md`。M2 决策为完全替代 legacy request：删除 `FSGSRuleRequest` / `BuildRuleRequest` / `SGSLegacyRuleRequests` 运行时代码路径，迁移 BasicCardRules 到 typed Rule，并将 Driver 基础牌 pending 状态迁入 `ResolutionStack`。
- 2026-07-03：完成 M2 核心代码实现并更新状态。`FSGSRuleRequest` / `BuildRuleRequest` / `SGSLegacyRuleRequests` 已从运行时代码删除；新增 typed Rule 基类与 `FSGSResolutionStack`；BasicCardRules 已改为 typed payload；Dying.Peach / Slash.Dodge 响应窗口状态由 frame state 管理；Development 构建、legacy 静态删除检查、`SGS.Plan0005.BasicCards` 和 `graphify update .` 已完成。父计划仍保持 `In Progress`：M2 仍需修正 Plan0014 旧测试预期并补专项自动化，Trigger / Modifier / ViewAs 仍未实施。
- 2026-07-03：创建 M3 子计划 `0014-M3-resolution-stack-resume-timing-event.md`。M3 决策为将 `ResolutionStack` 从响应窗口容器升级为可恢复结算栈，并建立受控 TimingEvent / TriggerRule 接缝；Modifier / ViewAs 继续留到后续阶段。
- 2026-07-03：完成 M3。`FinishCurrentResolution` 不再 reset 整栈，而是 complete current frame 并按 parent continuation resume；基础牌 Runtime 临时接口已删除；`FSGSRuleEventPayload` 与 `FSGSTriggerRuleBase<TPayload>` 已建立。验证范围：Development build、legacy 静态删除检查、Plan0005、Plan0013、Plan0014、`graphify update .`。父计划仍保持 `In Progress`，下一阶段重点是完整 TriggerRule 调度、Modifier / ViewAs 和更多牌 / 技能内容。
