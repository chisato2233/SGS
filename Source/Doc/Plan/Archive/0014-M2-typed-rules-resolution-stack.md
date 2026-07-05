# 0014-M2 — Typed Rule 完全替代与 ResolutionStack 迁移

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-07-03 |
| 最近更新 | 2026-07-03 |
| 关联需求 | `0000-RawRequirements.md` 中的第 15 条 |
| 父计划 | `0014-rule-layer-long-term-refactor.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Commands/`、`Source/SGS/Server/Engine/`、`Source/SGS/Server/Effects/`、`Source/SGS/Server/Timing/` |

---

## 1. 目标

第二阶段在第一阶段 `RuleInvocation + Indexed RuleRegistry` 地基上，做一次干净的结构替换：

1. 完全删除 legacy `FSGSRuleRequest` 路径，不保留为了老接口服务的兼容层。
2. 引入 typed Rule 基类，让 Rule 直接读取 `FSGSRuleInvocation` 的 typed payload，不再手动从万能 request 字段猜语义。
3. 引入 `FSGSResolutionStack / FSGSResolutionFrame`，把 Slash.Dodge、Dying.Peach 等响应窗口与后续 continuation 从 `USGSGameDriver` 迁出。
4. 迁移 Pass / Slash / Dodge / Peach / DyingPeach 到 typed Rule + ResolutionStack 路径，保持 Plan 0005 / 0013 行为等价。
5. `USGSGameDriver` 回到编排职责：推进阶段、提交 Command、分发 DecisionRequest、运行 EffectPipeline；不再保存基础牌结算专用 pending 状态。

本阶段接受破坏旧内部接口。项目仍处于早期开发阶段，优先获得长期清晰结构，不为已经废弃的第一阶段过渡 API 付出复杂度。

## 2. 背景与约束

第一阶段已经完成：

- `CommandType / CommandRouter` 能构造 `FSGSRuleInvocation`。
- `FSGSRuleRegistry` 已基于 `TSGSIndexedStore<FSGSRuleEntry>` 和 descriptor 查询候选 Rule。
- 基础牌 Rule 已补 descriptor，但执行仍通过 legacy `Context.RuleRequest` 读取参数。
- `USGSGameDriver` 仍持有 `PendingWindowName`、`PendingProcessingCard`、`PendingDyingResponders` 等基础牌结算状态。

第二阶段必须直接移除这些过渡点：

- 删除 `FSGSRuleRequest` 与 `SGSLegacyRuleRequests::FromInvocation`。
- 删除 `ISGSCommandType::BuildRuleRequest`、`FSGSCommandRouter::BuildRuleRequest`。
- 删除 `FSGSRuleExecutionContext::RuleRequest`。
- 删除 BasicCardRules 对 `RuleRequest` 字段的依赖。
- 删除 Driver 中只服务基础牌结算的 pending card / pending window / dying responder 状态。

归档状态（2026-07-04）：上述核心替换已完成。运行时代码中 legacy request 关键符号已清空，基础牌规则已改为 typed payload，`USGSGameDriver` 的基础牌 pending 状态已迁入 `FSGSResolutionStack`；旧 Plan0014 测试预期、ResolutionStack resume 语义和 Rule 模块布局已由 M3 / M4 后续阶段完成并验证。

不做的事：

- 不引入 Trigger / Modifier / ViewAs 正式能力。
- 不改 UI、AI 决策协议、网络入口或牌库数据入口，除非为了编译适配 renamed API。
- 不引入 Lua 或脚本 runtime。
- 不为了保持旧内部接口而新增 adapter、mirror、兼容 shim。

## 3. 方案设计

### 3.1 Typed Rule 基类

保留 `ISGSRule` 作为 Registry 统一接口，但新增 typed 基类收束 payload 检查：

```text
ISGSRule
  GetDescriptor()
  CanHandle()
  Validate()
  Execute()

TSGSTypedRule<TPayload>
  GetExpectedPayloadStruct()
  CanHandlePayload()
  ValidatePayload()
  ExecutePayload()
```

基类职责：

- 检查 `Context.RuleInvocation.Payload` 是否为 `TPayload`。
- 在 payload 类型不匹配时返回统一错误：`SGS.Rule.PayloadTypeMismatch`。
- 提供 `GetPayloadChecked` 或同等受控访问，避免具体 Rule 重复写 `GetPayload<T>()` 分支。
- 默认 `CanHandle` 只做 payload 与 descriptor 层面的基础匹配，玩法差异留给 `CanHandlePayload`。

首批派生方向：

- `FSGSActionRuleBase<TPayload>`：主动动作，如普通 Pass、UseCard。
- `FSGSCardActionRuleBase<TPayload>`：需要卡牌在合法区域、拥有者正确、目标列表可检查的动作。
- `FSGSResponseRuleBase<TPayload>`：响应窗口内的 RespondCard / Pass。

本阶段只落 Action / CardAction / Response，Trigger / Modifier / ViewAs 后续再接。

### 3.2 RuleInvocation payload 扩展

第一阶段已有 Pass / UseCard / RespondCard payload。本阶段需要补足它们用于完全替代 request 的信息：

- `FSGSPassRulePayload`
  - `WindowName`
  - `RequiredCardName`
  - `EffectSourceSeat`
  - `EffectTargetSeat`
  - `bIsResponsePass` 或等价语义，区分普通阶段 Pass 与窗口 Pass
- `FSGSUseCardRulePayload`
  - `CardId`
  - `CardName`
  - `TargetSeatIndices`
  - `EffectSourceSeat`
  - `EffectTargetSeat`
- `FSGSRespondCardRulePayload`
  - `CardId`
  - `CardName`
  - `TargetSeatIndices`
  - `WindowName`
  - `RequiredCardName`
  - `EffectSourceSeat`
  - `EffectTargetSeat`

如果 `ResolutionFrame` 已经能表达 source / target / processing card，Rule payload 可以只保留输入事实，运行期事实从 frame 读取；但同一字段不得同时有两个可变事实源。

### 3.3 ResolutionStack

新增 `FSGSResolutionStack` 和 `FSGSResolutionFrame`，建议放在 `Source/SGS/Server/Rules/`：

```text
FSGSResolutionFrame
  FrameHandle
  ParentFrameHandle
  SourceRuleName
  SourceCommandId
  ActorSeat
  SourceSeat
  TargetSeatIndices
  ProcessingCardIds
  WindowName
  RequiredCardName
  EffectSourceSeat
  EffectTargetSeat
  ContinuationName
  FrameState: FInstancedStruct
```

```text
FSGSResolutionStack
  PushFrame()
  PopFrame()
  GetCurrentFrame()
  FindFrame()
  OpenResponseWindow()
  CloseResponseWindow()
  ResumeCurrentFrame()
  CancelCurrentFrame()
  CheckInvariants()
```

存储建议：

- 使用 `TSGSIndexedStore<FSGSResolutionFrame>` 保存 frame，保留 stable handle。
- 使用 `TArray<FSGSStableHandle>` 维护实际栈顺序。
- 注册索引：BySourceCommandId、ByWindowName、ByActorSeat；第一阶段不需要复杂 ordered index。

首批 frame state：

- `FSGSSlashResolutionState`
  - Slash 牌 id
  - Source seat
  - Target seat
  - 是否已被 Dodge 抵消
- `FSGSDyingPeachResolutionState`
  - Dying seat
  - Responder queue
  - Next responder index

这些 state 替代 `USGSGameDriver` 的 `PendingProcessingCard`、`PendingDyingResponders`、`PendingDyingResponderIndex`。

### 3.4 Runtime 边界

`ISGSRuleRuntime` 继续作为 Rule 到 Driver / EffectPipeline 的受控门面，但接口应改为围绕 ResolutionStack：

- `RunEffectStep` 保留。
- `OpenResponseWindow` 改为创建或更新当前 frame 的窗口事实，并返回 DecisionRequest 是否已发出。
- `FinishCardResolution` 改为完成当前 frame 或让 stack resume parent。
- `SetProcessingCard / GetProcessingCard / GetEffectSourceSeat / GetEffectTargetSeat` 不再从 Driver pending 字段读写，改为 frame 访问或删除。
- `InitializeDyingPeachResponders / RequestNextDyingPeachResponder` 改为 DyingPeach frame state 操作，或移动成 DyingPeach Rule 内部 helper。

迁移完成后，`FSGSDriverRuleRuntime` 不保存玩法状态，只负责把 Runtime 调用桥接到：

- `USGSGameDriver::RunEffectStep`
- `USGSGameDriver::MakeResponseRequest`
- `USGSGameDriver::DeferResponseRequest`
- `USGSGameDriver::DispatchDeferredResponseRequest`
- `FSGSResolutionStack`

### 3.5 Driver 改造

`USGSGameDriver` 保留：

- `PendingCommandId`
- `PendingRequestId`
- `CurrentSeatIndex`
- `bWaitingForDecision`
- `DeferredResponseRequest`
- `DeferredResponseAgent`
- phase / turn pump 状态

`USGSGameDriver` 删除：

- `PendingWindowName`
- `PendingRequiredCardName`
- `PendingEffectSourceSeat`
- `PendingEffectTargetSeat`
- `PendingProcessingCard`
- `PendingDyingResponders`
- `PendingDyingResponderIndex`

`MakeCommandExecutionContext` 从 `ResolutionStack` 当前 response window 读取窗口语义；若 stack 没有打开窗口，则输出普通 play-phase context。

### 3.6 基础牌迁移

迁移目标：

- `FSGSPassRule`
  - 普通 Pass 读取 `FSGSPassRulePayload`，无窗口时推进阶段。
  - 窗口 Pass 不再由普通 PassRule 处理，而由对应 ResponseRule 处理。
- `FSGSSlashRule`
  - 读取 `FSGSUseCardRulePayload`。
  - 执行时创建 Slash resolution frame，移动 Slash 到 Processing，打开 Slash.Dodge window。
- `FSGSDodgeResponseRule`
  - 读取 `FSGSRespondCardRulePayload` 或 `FSGSPassRulePayload`。
  - Dodge 成功则弃 Dodge、弃 Processing Slash、关闭 Slash frame。
  - Pass 则 resume Slash hit continuation。
- `FSGSPeachRule`
  - 读取 `FSGSUseCardRulePayload`，治疗目标来自 payload /规则上下文。
- `FSGSDyingPeachRule`
  - 读取 `FSGSRespondCardRulePayload` 或 `FSGSPassRulePayload`。
  - responder queue 与当前求桃进度全部存在 DyingPeach frame state 中。

允许为响应 Pass 建立更清晰的 typed rule 分支，例如 `FSGSDodgeResponseRule` 同时接受 Pass/Respond payload，或拆为 `FSGSDodgePassRule` 与 `FSGSDodgeCardRule`。优先选择代码更清楚、descriptor 更精确的方案，不为保留旧类名牺牲结构。

## 4. 工程质量门

### 4.1 真实使用入口

M2 完成后，以下真实路径必须走新结构：

```text
ScriptedDecisionAgent / LocalUI / future RPC
  -> FSGSCommand
  -> FSGSCommandRouter::SubmitCommand
  -> BuildRuleInvocation
  -> FSGSRuleRegistry::Resolve
  -> typed Rule
  -> FSGSResolutionStack / ISGSRuleRuntime
  -> FSGSEffectPipeline
  -> USGSGameContext
```

不存在 `Command -> RuleRequest -> BasicCardRules` 的旁路。

### 4.2 关键不变量

- `FSGSRuleExecutionContext` 必须持有有效 `RuleInvocation`，不得持有 legacy request。
- Rule 执行前 payload 类型必须匹配 typed Rule 基类期望类型。
- `ResolutionStack` push/pop 成对；游戏回到普通阶段时不得遗留 response window frame。
- 每个打开的 response window 必须绑定 frame、seat、command id、request id、window name、required card。
- Processing card 的权威 zone 仍由 `EffectPipeline` 移动，frame 只保存引用 id，不直接改牌区。
- Driver 不保存基础牌规则状态；若需要 continuation，必须进 frame state。
- Registry 查询结果仍按 descriptor priority + registration order 稳定排序。

### 4.3 失败路径

- Invocation payload 类型错误：Registry 找到 Rule 后，typed base 返回 `SGS.Rule.PayloadTypeMismatch`，不执行 Rule。
- Stack 无当前 frame 却收到响应命令：Router 或 Rule 返回 `SGS.Rule.MissingResolutionFrame`，Driver fallback pass 不污染状态。
- Response window 迟到 / 错窗口 / 错 request：CommandRouter 拒绝，或 Driver fallback pass 当前窗口。
- Frame state 类型错误：返回 `SGS.Rule.FrameStateMismatch`，并中止当前 frame。
- EffectPipeline 步骤失败：Rule 返回错误，Driver 记录 rejected 并保持 ReplayLog 可追踪。

### 4.4 可删除性要求

本阶段的代码质量门包含静态删除检查：

```powershell
rg -n "FSGSRuleRequest|RuleRequest|BuildRuleRequest|SGSLegacyRuleRequests" Source\SGS -g "*.h" -g "*.cpp"
```

验收时除历史计划文档、注释迁移说明或测试名称外，运行时代码中不应再出现这些符号。若某个名字必须保留，必须在本计划进度记录中说明为什么它不是兼容层。

## 5. 任务拆解

- [x] 新增 `SGSTypedRule.h`：实现 `TSGSTypedRule<TPayload>` 与 Action / CardAction / Response 基类。
- [x] 删除 legacy request API：移除 `FSGSRuleRequest`、`SGSLegacyRuleRequests::FromInvocation`、`BuildRuleRequest`、`FSGSRuleExecutionContext::RuleRequest`。
- [x] 扩展 / 清理 RuleInvocation payload，使 Pass / UseCard / RespondCard 覆盖基础牌所需全部 typed 数据。
- [x] 新增 `SGSResolutionStack.h/.cpp`：实现 frame store、stack order、response window、frame state、invariants。
- [x] 改造 `ISGSRuleRuntime` 与 `FSGSDriverRuleRuntime`，使玩法运行状态全部通过 ResolutionStack 访问。
- [x] 改造 `USGSGameDriver`：删除基础牌 pending 字段，`MakeCommandExecutionContext` 从 stack 当前窗口构造上下文。
- [x] 迁移 Pass / Slash / Peach 到 typed action/card rules。
- [x] 迁移 DodgeResponse / DyingPeach 到 typed response rules，并把 Slash.Dodge / Dying.Peach continuation 迁入 frame state。
- [x] 更新 `FSGSRuleRegistry::Resolve` 的错误日志，使 NotFound / PayloadMismatch 能打印 invocation 与候选 rule name。
- [x] 更新自动化测试，覆盖 typed rules、ResolutionStack、legacy 删除和 Plan 0005 / 0013 回归。
- [x] 运行 Development 构建、legacy 静态删除检查、`SGS.Plan0005.BasicCards` 和 `graphify update .`。
- [x] 运行 Plan0013 / Plan0014 完整相关回归，并修正旧测试预期。

## 6. 验收标准

必须全部满足：

- Development 构建通过：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Unreal.ps1 -Action Build -Configuration Development
```

- 现有回归通过：
  - `SGS.Plan0005.BasicCards`
  - `SGS.Plan0013.RulePipeline.*`
  - `SGS.Plan0014.RuleInvocation.BasicPayloads`
  - `SGS.Plan0014.RuleRegistry.IndexedLookup`

- 新增自动化通过：
  - `SGS.Plan0014.TypedRules.BasicCards`：Slash / Dodge / Peach / DyingPeach 全部通过 typed payload 执行，不读取 legacy request。
  - `SGS.Plan0014.ResolutionStack.BasicWindows`：Slash.Dodge 与 Dying.Peach frame 正确 push/pop，结算结束无悬空 frame。
  - `SGS.Plan0014.ResolutionStack.InvalidResponse`：错窗口 / 缺 frame / 错 frame state 不执行 Rule，错误码可诊断。
  - `SGS.Plan0014.LegacyRemoval.StaticBoundary`：运行时代码中无 `FSGSRuleRequest`、`BuildRuleRequest`、`SGSLegacyRuleRequests`。

- 行为验收：
  - Dodge 仍能抵消 Slash。
  - Dodge window pass 仍造成 Slash 伤害。
  - Dying.Peach 中有 Peach 可救回目标。
  - 无 Peach 时濒死目标被淘汰。
  - Invalid command fallback pass 不污染 ResolutionStack。

- 结构验收：
  - `USGSGameDriver` 不再保存 `PendingWindowName`、`PendingProcessingCard`、`PendingDyingResponders` 等基础牌规则状态。
  - Rule 状态变更仍只通过 `EffectPipeline`。
  - `graphify update .` 后能查询到 `RuleInvocation -> TypedRule -> ResolutionStack -> EffectPipeline` 路径。

### 6.1 归档验收状态

已完成：

- Development 构建通过。
- 静态删除检查无运行时代码命中：`FSGSRuleRequest|RuleRequest|BuildRuleRequest|SGSLegacyRuleRequests`。
- `SGS.Plan0005.BasicCards` 通过。
- `SGS.Plan0013.RulePipeline` 通过。
- `SGS.Plan0014` 通过。
- 旧 `DyingPeachPass` 测试预期已在 M3 阶段修正。
- M2 中未单独新增的专项自动化已由 M3 的 ResolutionStack resume / nested / duplicate same-seat 和 Plan0014 registry / payload 测试覆盖。
- `graphify update .` 已运行。

## 7. 进度与决策记录

- 2026-07-03：创建 M2 计划。决策：早期开发阶段不保留 legacy compatibility；第二阶段以完全替代 `FSGSRuleRequest`、typed Rule 基类、ResolutionStack 迁移为目标。Trigger / Modifier / ViewAs 留到后续阶段，避免与基础响应结算重构相互缠绕。
- 2026-07-03：完成 M2 核心代码实现。删除 legacy request 运行时代码路径；新增 `TSGSTypedRule` 与 `FSGSResolutionStack`；基础牌规则改为 typed payload；响应 Pass 拆为 `DodgePass` / `DyingPeachPass`；`USGSGameDriver` 删除基础牌 pending window / processing card / dying responder 状态。验证：Development 构建通过，legacy 静态删除检查无命中，`SGS.Plan0005.BasicCards` 通过，`graphify update .` 已运行。状态保持 `In Progress`，等待测试预期修正与 M2 专项自动化补齐。
- 2026-07-04：完成 M2 收尾并归档。`FSGSRuleRegistry::Resolve` 已补强 NotFound / PayloadTypeMismatch 诊断，错误上下文包含 invocation、候选 Rule 名称与期望 payload；Plan0013 / Plan0014 相关回归通过，legacy 静态删除检查无命中，Development build 与 `graphify update .` 已完成。M3 / M4 已覆盖并验证 M2 后续的 ResolutionStack resume 与目录组织任务。
