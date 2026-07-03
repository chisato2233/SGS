# 0014-M3 — ResolutionStack Resume 与受控 TimingEvent 地基

| 字段 | 值 |
|---|---|
| 状态 | `Done` |
| 创建日期 | 2026-07-03 |
| 最近更新 | 2026-07-03 |
| 关联需求 | `0000-RawRequirements.md` 中的第 16 条 |
| 父计划 | `0014-rule-layer-long-term-refactor.md` |
| 前置计划 | `0014-M2-typed-rules-resolution-stack.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Engine/`、`Source/SGS/Server/Effects/`、`Source/SGS/Server/Timing/` |

---

## 1. 目标

M3 将 M2 的 `ResolutionStack` 从“响应窗口状态容器”升级为真正可恢复的结算栈：

1. 子结算完成时只 pop 当前 frame，并按 parent frame 的符号化 continuation 恢复父结算。
2. `FinishCurrentResolution` 不再默认 `Reset()` 整栈；只有 abort / 对局重置才清空整栈。
3. `ISGSRuleRuntime` 删除基础牌专用状态接口，改为通用 push frame、complete frame、open response window、abort stack、访问 stack。
4. 基础牌迁移到 resume 语义：Slash frame 可等待 Dodge，也可因命中推入 DyingPeach 子 frame；DyingPeach 完成后恢复 Slash parent。
5. 建立最小受控 TimingEvent / TriggerRule 接缝，为后续技能触发铺路；本阶段不落完整 Trigger / Modifier / ViewAs 系统。

本阶段仍保持状态变更只能通过 `EffectPipeline -> GameContext`。`ResolutionStack` 只保存结算事实、窗口事实和恢复点。

## 2. 范围

### 2.1 本阶段要做

- 新增 M3 子计划并同步父计划 / ProjectBrief 状态。
- `FSGSResolutionStack` 增加通用操作：
  - `CompleteCurrentFrame`
  - `AbortAllFrames`
  - `GetParentFrame`
  - `FindLatestDyingFrameForSeat`
  - 对已有 `FindLatestFrameWithState` 的稳定使用
- `FSGSResolutionFrame` 增加 `OnChildCompletedContinuation`，使用 `FName` 表达恢复语义，不保存 C++ callback。
- `FSGSDyingPeachResolutionState` 增加 health recheck 标记，避免同座位重复 Dying frame。
- `ISGSRuleRuntime` 收敛为通用 Runtime 门面。
- 基础牌规则使用 stack frame state 完成 Slash.Dodge、Slash hit、Dying.Peach、active Peach。
- 增加最小 `FSGSRuleEventPayload` 与 `FSGSTriggerRuleBase<TPayload>`。

### 2.2 本阶段不做

- 不完整实现 TriggerRule 调度队列、cost / effect 流程或技能选择。
- 不实现 Modifier / ViewAs。
- 不实现多个不同座位同时进入濒死的通用队列；AoE / 群体伤害阶段补。
- 不改 UI、AI、网络协议，除非编译适配新 Runtime 接口。
- 测试修改和运行放到本阶段实现最后统一处理。

## 3. 设计要点

### 3.1 Frame Completion

`CompleteCurrentFrame` 只完成栈顶 frame：

```text
child frame complete
  -> pop child
  -> if parent exists: inspect parent.OnChildCompletedContinuation
  -> resume parent or continue waiting
  -> if stack empty: return to phase pump
```

`AbortAllFrames` 用于规则失败、对局重置或不可恢复错误。普通卡牌结算结束不得调用整栈 reset。

### 3.2 Continuation

Continuation 用开放 `FName`，首批只需要：

- `SGS.Resolution.Continuation.FinishParentCardResolution`
- `SGS.Resolution.Continuation.ResumeDyingPeach`

Rule 只声明父 frame 在子 frame 完成后该做什么；Driver / Runtime 负责解释少量核心 continuation，避免把 C++ 回调存进 frame。

### 3.3 嵌套濒死策略

- 因果嵌套按 LIFO 处理：后触发的 Dying frame 先结算。
- 同一座位已有未完成 Dying frame 时，不重复 push；只标记该 frame 需要 health recheck。
- Dying frame 被恢复时先检查当前体力 / 存活状态，再决定继续求桃、完成 frame 或淘汰。
- 多座位同时濒死暂不排队；等群体伤害 / AoE 进入后再引入通用 pending dying queue。

### 3.4 TimingEvent 接缝

M3 只建立受控事件入口：

```text
FSGSRuleEventPayload
  EventTag
  EventName
  SourceSeat
  TargetSeat
  SourceCommandId
  TimingPoint
  EventData
```

未来 TriggerRule 只能通过 RuleRegistry / RuleRuntime 接入。裸 observer 不允许直接修改 `GameContext`，否则会绕开 EffectPipeline、ReplayLog 和随机审计。

## 4. 任务拆解

- [x] 创建 M3 子计划文档，并同步父计划 / ProjectBrief 状态。
- [x] 扩展 `FSGSResolutionStack`：complete / abort / parent / latest dying / continuation / recheck。
- [x] 重构 `ISGSRuleRuntime`：删除基础牌专用接口，增加通用 stack 与 timing event API。
- [x] 迁移 Slash / Dodge / Peach / DyingPeach 到 resume 语义。
- [x] 清理 Driver 中 `FinishCurrentResolution` 的整栈 reset 语义。
- [x] 新增 `FSGSRuleEventPayload` 与 `FSGSTriggerRuleBase<TPayload>` 地基。
- [x] 最后统一更新测试：Plan0014 Registry 预期、ResolutionStack resume / nested / duplicate same-seat / invalid current-frame；错窗口响应边界继续由 Plan0013 覆盖。
- [x] 最后统一验证：Development build、legacy 静态删除检查、Plan0005、Plan0013、Plan0014、`graphify update .`。

## 5. 验收标准

- `FinishCurrentResolution` 普通完成路径不再清空整栈。
- DyingPeach 子 frame 完成后能恢复 Slash parent，并由 parent continuation 决定是否完成自身。
- 嵌套 Dying frame 按 LIFO 恢复。
- 同座位重复 Dying 不产生重复 frame。
- 运行时代码无 legacy request 符号回归。
- 状态变更仍只走 EffectPipeline。
- `graphify update .` 后可以查询到 `ResolutionStack resume` 与 `TimingEvent` 新接缝。

## 6. 进度记录

- 2026-07-03：创建 M3 计划。决策：优先修正结算栈语义，Trigger 只做受控事件入口，不让观察者模式直接拥有状态修改权。
- 2026-07-03：完成 M3 实现。`ResolutionStack` 新增 current-frame completion、abort、parent 查询、latest dying 查询和 continuation 字段；Driver 的完成路径改为 pop 当前 frame 并 resume parent；基础牌 Runtime 接口已通用化；Slash / Dodge / Peach / DyingPeach 迁移到 resume 语义；新增 `FSGSRuleEventPayload` 与 `FSGSTriggerRuleBase<TPayload>` 地基。验证：Development build 通过，legacy / M2 临时 Runtime API 静态检查无命中，`SGS.Plan0005.BasicCards`、`SGS.Plan0013.RulePipeline`、`SGS.Plan0014` 均通过，`graphify update .` 已运行。
