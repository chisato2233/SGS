# 0014 — Rule 层长期重构：Typed Invocation、索引注册表与结算栈

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-07-03 |
| 最近更新 | 2026-07-04 |
| 关联需求 | `0000-RawRequirements.md` 中的第 14、18、19 条 |
| 子计划 | 已归档：`0014-M2-typed-rules-resolution-stack.md`、`0014-M3-resolution-stack-resume-timing-event.md`、`0014-M4-rule-module-layout.md`、`0014-M5-trigger-rule-dispatch.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Commands/`、`Source/SGS/Server/Engine/`、`Source/SGS/Server/Effects/`、`Source/SGS/Server/Timing/` |

---

## 1. 完成边界

本计划原本用于重构 Plan 0013 建立的最小 `Command -> Rule -> EffectPipeline` 管线，目标是先完成 Rule 层骨架，而不是继续推进完整玩法、距离系统、目标修正、更多卡牌或武将技能。

2026-07-04 决策：Rule 骨架阶段到此为止。距离计算、目标合法性、Modifier、ViewAs、具体技能、更多卡牌逻辑和更细 ReplayLog 因果追踪，都在后续新的 gameplay / skill / card 计划中单独展开，不继续作为 Plan 0014 的未完成项。

本计划已完成的骨架能力：

- typed `FSGSRuleInvocation` 与 Pass / UseCard / RespondCard / Event payload。
- `CommandType / CommandRouter` 只输出 typed RuleInvocation，不再走 legacy RuleRequest。
- `FSGSRuleRegistry` 使用 `TSGSIndexedStore<FSGSRuleEntry>`，支持 descriptor、lookup key、wildcard、priority 与 registration order。
- `TSGSTypedRule<TPayload>`、Action / CardAction / Response / Trigger 基类地基。
- 基础牌 Pass / Slash / Dodge / Peach / DyingPeach 迁移到 typed Rule。
- `FSGSResolutionStack / FSGSResolutionFrame` 承载 Slash.Dodge 与 Dying.Peach 的响应窗口、frame state 和 resume。
- 受控 `TimingEvent -> TriggerRule` 同步 dispatch 地基。
- Rule 工程目录按 Core / Payloads / Resolution / Actions / Responses / BasicCards 拆分。

## 2. 不继续纳入 0014 的事项

以下内容是后续真实玩法计划，不再视为 0014 偏移或未完成：

- 距离计算、目标合法性、坐骑与技能修正。
- ModifierRule 正式能力和 query modifier 验证。
- ViewAsRule 正式能力和视为响应样例。
- 更多基础牌、锦囊、装备和武将技。
- Command -> Invocation -> Rule -> EffectStep 的完整 ReplayLog 因果追踪。
- 技能触发的 cost / optional decision / event queue / ActiveRuleSet。

后续建议拆分：

- `0015-gameplay-distance-targeting.md`：真实距离、目标合法性、坐骑/技能修正。
- `0016-card-content-basic-expansion.md`：更多基础牌 / 锦囊 / 装备。
- `0017-skill-system-first-slice.md`：第一个武将技纵切，再决定 Modifier / ViewAs 的正式落地方式。
- `0018-replay-trace-causality.md`：Command -> Invocation -> Rule -> EffectStep 的调试追踪。

## 3. 验收记录

M2 / M3 / M4 / M5 均已完成并归档。最近一次 M5 验证记录：

- Development build 通过。
- `SGS.Plan0005.BasicCards`：3/3 Success。
- `SGS.Plan0013.RulePipeline`：3/3 Success。
- `SGS.Plan0014`：7/7 Success。
- legacy 删除检查无 `FSGSRuleRequest` / `RuleRequest` / `BuildRuleRequest` / `SGSLegacyRuleRequests`。
- `graphify update .` 已执行。

本次归档只收束文档边界，不修改运行时代码，不新增自动化，不要求重新运行完整测试。

## 4. 进度与决策记录

- 2026-07-03：创建计划。决定先重构 Rule 层长期结构，再继续推进锦囊、装备和武将技。核心方向为 typed `RuleInvocation`、`TSGSIndexedStore` 驱动的 `RuleRegistry`、显式 `ResolutionStack`，并借鉴 QSanguosha 的 Trigger / ViewAs / Modifier 分类，但不引入 Lua runtime 或弱类型状态修改。
- 2026-07-03：完成第一阶段地基。新增 typed `FSGSRuleInvocation` 与 Pass / UseCard / RespondCard payload；`CommandType / CommandRouter` 输出 Invocation；`FSGSRuleRegistry` 切换到 IndexedStore；基础牌 Rule 补齐 descriptor。
- 2026-07-03：完成 M2。删除 legacy request 运行时代码路径；基础牌迁移到 typed Rule；Slash.Dodge / Dying.Peach 响应窗口状态由 ResolutionStack frame state 管理。
- 2026-07-03：完成 M3。`ResolutionStack` 支持 complete current frame 并 resume parent；基础牌 Runtime 临时接口删除；建立 `FSGSRuleEventPayload` 与 `FSGSTriggerRuleBase<TPayload>` 地基。
- 2026-07-03：完成 M4。Rule 核心、payload、resolution、基础牌 action / response 目录拆分落地；`SGSRulePayloads.h` 成为 umbrella include。
- 2026-07-04：完成 M5。`FSGSRuleRegistry::DispatchAll` 与 `USGSGameDriver::PublishTimingEvent` 接通受控 Trigger dispatch；默认规则集不注册正式 TriggerRule，基础牌行为保持不变。
- 2026-07-04：根据用户决策收束 0014：Rule 骨架阶段到此为止，距离计算、目标修正、Modifier、ViewAs、具体技能与更多卡牌逻辑另起后续计划。本计划归档。
