# 0013 — Command 到 Rule 到 Effect 管线

| 字段 | 值 |
|---|---|
| 状态 | `Done` |
| 创建日期 | 2026-07-01 |
| 最近更新 | 2026-07-01 |
| 关联需求 | `0000-RawRequirements.md` 中的第 13 条 |
| 关联代码 | `Source/SGS/Shared/Commands`、`Source/SGS/Server/Commands`、`Source/SGS/Server/Rules`、`Source/SGS/Server/Engine`、`Source/SGS/Server/Effects` |

---

## 1. 目标

建立服务器权威的 `Command -> Rule -> EffectPipeline -> GameContext` 规则管线：

- `CommandRouter` 负责命令信封、payload、座位、阶段、窗口等通用校验与生命周期审计。
- `Rule` 负责卡牌 / 响应 / 后续技能的规则语义：合法目标、消耗方式、响应窗口、后续结算。
- `EffectPipeline` 负责所有权威状态变更：移牌、摸牌、伤害、回复、淘汰、持续效果生命周期和 ReplayLog 事件。
- `USGSGameDriver` 退回 orchestration 职责：推进阶段、等待决策、提交命令、执行规则结果、刷新挂起窗口；不再直接写具体牌名分支。

完成后，Plan 0005 的杀 / 闪 / 桃 / 濒死求桃玩法保持行为一致，但规则分发从 Driver 内联分支迁移到 Rule 注册表。

## 2. 背景与约束

- 现有 Command 已有 typed payload 和 CommandType 唯一事实源；本轮删除 `ISGSCommandType::Execute` 旧执行接口，由 RuleRegistry 成为唯一规则入口。
- `USGSGameDriver` 目前直接分发 `Slash`、`Peach`、`Dodge`、`Dying.Peach`，继续扩展会让武将技、锦囊、装备和响应窗口挤进同一个类。
- `EffectPipeline`、`StandardEffectSteps`、`ReplayLog` 已存在，规则状态变更必须复用这些入口，不能绕回 Driver 或 GameContext 写平行 mutation。
- Rule 概念必须开放扩展：卡牌名、技能名、响应窗口、效果类型使用 `FName` / `GameplayTag` / 注册表，不使用封闭玩法 enum。

## 3. 方案设计

新增 `Server/Rules` 作为 Command 与 Effect 之间的规则层。

核心类型：

- `FSGSRuleExecutionContext`：一次规则解析的上下文，包含 `GameContext`、`Command`、`CommandExecutionContext`、`EffectPipeline`、`ReplayLog`、`ActiveEffectTimeline`、当前 TimingPoint，以及只读查询辅助。
- `ISGSRuleRuntime`：Rule 与 Driver 之间的受控运行门面，提供 `RunEffectStep`、`OpenResponseWindow`、`FinishCardResolution`、`AdvanceAfterPhase` 等能力；Rule 不直接调用 GameContext mutation。
- `FSGSRuleRequest`：由 CommandType 从 typed payload 构造出的规则请求，包含命令意图、CardId、目标座位、响应窗口、待恢复的 continuation。
- `ISGSRule`：规则接口，负责 `CanHandle`、`Validate`、`Execute`。`Execute` 只通过 `FSGSRuleRuntime` 排入 EffectStep 或请求响应窗口。
- `FSGSRuleRegistry`：Rule 注册表。默认注册基础牌规则与响应规则；后续卡牌、技能、装备按同一入口扩展。

默认规则：

- `FSGSPassRule`：结束当前出牌阶段或当前响应窗口。
- `FSGSSlashRule`：移动杀到处理区，打开 `Slash.Dodge` 响应窗口，记录 continuation。
- `FSGSDodgeResponseRule`：响应成功则弃置闪和处理中杀；响应失败则弃置处理中杀并造成伤害。
- `FSGSPeachRule`：弃置桃并回复目标。
- `FSGSDyingPeachRule`：濒死窗口内使用桃救援；失败时推进下一个求桃响应者或淘汰。

Driver 改造：

- `OnPlayActionDecided` 和 `OnResponseActionDecided` 调用 `ResolveCommandThroughRules`；该入口内部先做 Router 校验 / fallback，再构造 RuleRequest 并执行 RuleRegistry。
- 校验通过后，Driver 调用 `ResolveCommandThroughRules`，由 CommandType 构造 `FSGSRuleRequest`，再交给 `FSGSRuleRegistry`。
- Driver 只保存当前 pending continuation / response window，不保存具体牌规则分支。
- `RunEffectStep` 保留为 RuleRuntime 内部实现细节，外部规则不直接调用 Driver 私有 helper。

## 4. 工程质量门

- 真实入口：AI、真人、未来 RPC 都仍通过 `ISGSDecisionAgent -> FSGSCommand -> FSGSCommandRouter -> RuleRegistry -> EffectPipeline`。
- 不变量：
  - Command 未通过 Router 校验不得进入 Rule。
  - Rule 不直接修改 `GameContext`；状态变更必须通过 `EffectPipeline` 和标准 / 新增 EffectStep。
  - 响应窗口必须绑定 `CommandId`、`RequestId`、`WindowName`、目标座位和 continuation，过期 / 错窗口命令拒绝或 fallback。
  - ReplayLog 与 CommandLog 在成功 / fallback / 失败路径都保持可审计。
- 失败路径：
  - 未注册 Rule、Rule 校验失败、EffectStep 失败，都记录错误并按当前 fallback pass 语义收束。
  - 响应者不存在或无合法响应代理时走 Pass continuation，不阻塞 Pump。
- 扩展边界：
  - 后续锦囊、装备、武将技通过新增 Rule / EffectStep / ActiveEffect 接入。
  - 本计划不做完整技能触发系统，但 RuleRuntime 要预留 TimingPoint 和 ActiveEffect 查询入口。

## 5. 任务拆解

- [x] 新增 `Server/Rules` 基础类型：Rule request、runtime、registry、rule interface。
- [x] 扩展 CommandType：从 typed payload 构造 `FSGSRuleRequest`，保留 CommandType 作为命令类型事实源。
- [x] 将 Slash / Peach / Dodge / DyingPeach 从 `USGSGameDriver` 内联分支迁移为默认 Rule。
- [x] 改造 Driver：只负责编排阶段推进、等待窗口、pending continuation 和 RuleRuntime 调用。
- [x] 补齐 EffectStep：新增淘汰标准步骤，移牌 / 伤害 / 回复 / 淘汰均走 EffectPipeline。
- [x] 扩展自动化测试，覆盖成功、响应、错误窗口、ReplayLog / invariant。
- [x] 运行构建、验收和 `graphify update .`。

## 6. 验收标准

- 构建通过：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Unreal.ps1 -Action Build -Configuration Development
```

- 现有验收保持通过：
  - `SGS.Plan0005.BasicCards`
  - `SGS.Plan0011M1.LocalUI.DecisionBridge`
  - `SGS.Plan0004.GameContextPrimitives`

- 新增验收：
  - `SGS.Plan0013.RulePipeline.BasicCards`：杀 / 闪 / 桃 / 濒死求桃均通过 RuleRegistry 执行，Driver 不再按牌名直接分支。
  - `SGS.Plan0013.RulePipeline.InvalidWindow`：错 `WindowName` / 过期 `RequestId` 拒绝或 fallback，且 CommandLog 有 Rejected / Fallbacked 记录。
  - `SGS.Plan0013.RulePipeline.ReplayInvariants`：CommandLog、ReplayLog、GameContext `CheckInvariants()` 在 typed payload + Rule 执行后通过。

## 7. 进度与决策记录

- 2026-07-01：创建计划。决定先迁移 Plan 0005 基础牌纵切，建立管线边界；不在本计划内实现完整武将技触发系统。
- 2026-07-01：实施完成。删除 Command legacy mirror 字段、`SyncLegacyMirror`、`ISGSCommandType::Execute` / `ExecuteTyped` 和 Driver 内联 `Resolve*` 分支；新增 `FSGSRuleRegistry`、基础牌规则与 `MakeEliminateSeatStep`。构建通过，`SGS.Plan0005.BasicCards`、`SGS.Plan0011M1.LocalUI.DecisionBridge`、`SGS.Plan0004.GameContextPrimitives`、`SGS.Plan0013.RulePipeline.*` 均通过命令行自动化。
