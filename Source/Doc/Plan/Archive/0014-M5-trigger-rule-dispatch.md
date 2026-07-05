# 0014-M5 — TriggerRule Dispatch 地基

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-07-04 |
| 最近更新 | 2026-07-04 |
| 关联需求 | `0000-RawRequirements.md` 中的第 18 条 |
| 父计划 | `0014-rule-layer-long-term-refactor.md` |
| 前置计划 | `0014-M4-rule-module-layout.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Engine/`、`Source/SGS/Tests/` |

---

## 1. 目标

M5 只完成 `TimingEvent -> TriggerRule` 的受控调度闭环，不引入 Modifier / ViewAs，也不新增正式技能规则：

1. `FSGSRuleEventPayload` 可以映射为 `RuleKind=Trigger` 的 typed `FSGSRuleInvocation`。
2. `FSGSRuleRegistry` 保留 command `Resolve` 的单规则语义，并新增 Trigger 专用 `DispatchAll` 多规则语义。
3. `USGSGameDriver::PublishTimingEvent` 不再空实现，而是构造 Trigger invocation 并交给 Registry 同步调度。
4. 默认规则集不注册正式 TriggerRule，因此当前基础牌行为保持不变。
5. 自动化覆盖 Trigger 排序执行、空候选 no-op 和 payload mismatch 诊断。

## 2. 方案设计

Trigger invocation 映射固定为：

```text
RuleKindTag = SGS.RuleKind.Trigger
IntentTag   = EventTag
SubjectName = None
WindowName  = TimingPoint.Step
Payload     = FSGSRuleEventPayload
```

`DispatchAll` 使用和 `Resolve` 相同的 descriptor / lookup key / wildcard / priority 排序机制，但语义不同：

- 无候选或候选全部 `CanHandle=false` 时返回成功。
- payload 类型全部不匹配时返回 `SGS.Rule.PayloadTypeMismatch`。
- 任一 Trigger 的 `Validate` 或 `Execute` 出错时立即中断并向发布方冒泡。

`PublishTimingEvent` 仍只能通过 `ISGSRuleRuntime` 进入 Driver，TriggerRule 不直接修改 `GameContext`；状态变化继续走 `EffectPipeline`。

## 3. 任务拆解

- [x] `FSGSRuleEventPayload` 增加最小 invariant：合法 EventTag、座位形状、SourceCommandId 形状和有效 TimingPoint。
- [x] `FSGSRuleRegistry` 新增 `DispatchAll`，复用既有索引、wildcard 和排序。
- [x] `USGSGameDriver::PublishTimingEvent` 构造 Trigger invocation、synthetic command context，并同步调用 `DispatchAll`。
- [x] 新增 Trigger 最小自动化：排序执行、无候选 no-op、payload mismatch。
- [x] 更新父计划、ProjectBrief 和 Rule Authoring 文档。
- [x] 运行 Development build、Plan0005、Plan0013、Plan0014、legacy 静态删除检查与 `graphify update .`。

## 4. 验收标准

- Development build 通过。
- 自动化通过：
  - `SGS.Plan0005.BasicCards`
  - `SGS.Plan0013.RulePipeline`
  - `SGS.Plan0014`
- `SGS.Plan0014.Trigger.DispatchAll` 验证多个 Trigger 按 priority / registration order 稳定执行。
- `SGS.Plan0014.Trigger.NoCandidateNoOp` 验证无候选 Trigger 不影响主结算。
- `SGS.Plan0014.Trigger.PayloadMismatch` 验证错误 payload 不执行 Trigger 且返回统一诊断。
- legacy 静态删除检查无 `FSGSRuleRequest` / `RuleRequest` / `BuildRuleRequest` / `SGSLegacyRuleRequests`。
- `graphify update .` 已运行。

## 5. 进度记录

- 2026-07-04：创建并完成 M5。决策：本阶段只接通 Trigger dispatch，不从基础牌流程自动发布标准事件，不注册正式技能 TriggerRule；Modifier、ViewAs、事件队列、可选触发询问和技能费用留到后续阶段。
