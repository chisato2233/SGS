# Rule Authoring Rules

本文件规定新增 Rule、Rule payload、Rule descriptor 与 Resolution frame state 的写法。它不描述现有 Rule 清单；查询当前结构用 graphify。

---

## 先判断 Rule 类型

新增 Rule 前，先选择它属于哪类职责：

- `Action`：主动动作，例如使用牌、装备牌、发动主动技能。
- `Response`：响应窗口内的出牌或放弃。
- `Trigger`：时机事件触发，后续技能系统接入。
- `Modifier`：修正查询结果，例如距离、目标数、使用次数。
- `ViewAs`：视为 / 转化，生成新的规则意图或响应能力。
- `GameRule`：少量全局规则或 bookkeeping，不应滥用。

不要用一个 Rule 同时承担多种职责。若一个技能既触发又修正距离，应注册多个内部 Rule。

## 文件落点

Rule 文件按 `RuleKind + 内容包` 放置：

- 主动规则：`Source/SGS/Server/Rules/Actions/<ContentPackage>/`
- 响应规则：`Source/SGS/Server/Rules/Responses/<ContentPackage>/`
- 触发规则：`Source/SGS/Server/Rules/Triggers/<ContentPackage>/`
- 修正规则：`Source/SGS/Server/Rules/Modifiers/<ContentPackage>/`
- 视为规则：`Source/SGS/Server/Rules/ViewAs/<ContentPackage>/`
- 内容包共享 helper / 注册入口：`Source/SGS/Server/Rules/<ContentPackage>/`

通用框架只放在 `Rules/Core`、`Rules/Payloads`、`Rules/Resolution`。

## Payload 选择

优先复用已有 typed payload。只有当现有 payload 不能表达服务器翻译后的规则事实时，才新增 payload。

新增 payload 时：

- 放在 `Rules/Payloads/` 下的 focused header。
- 只表达 RuleInvocation 所需的内部规则事实，不直接复用 shared Command payload。
- 提供 `ToPayloadLogString()`。
- 在 `SGSRulePayloads.h` 中聚合 include，但不要把定义写回 umbrella。

Command payload 是外部意图协议；Rule payload 是服务器校验和上下文翻译后的内部调用数据。这两者不能混用。

## Descriptor

每个 Rule 必须提供稳定 `RuleName` 和 `FSGSRuleDescriptor`。

默认要求：

- `RuleKindTag` 体现职责分类。
- `IntentTag` 表达命令或事件意图。
- `SubjectName` 表达卡名、技能名、规则对象或 `NAME_None` wildcard。
- `WindowName` 只用于响应窗口匹配。
- `Priority` 只解决同一候选集内的顺序，不应作为隐藏控制流。

新增 wildcard 时必须有明确理由。全局 wildcard Rule 数量应保持很少。

## TriggerRule

TriggerRule 只通过受控 `TimingEvent` 发布入口执行，不直接订阅裸 observer 后修改 `GameContext`。

M5 后默认映射：

- `RuleKindTag = SGS.RuleKind.Trigger`
- `IntentTag = FSGSRuleEventPayload.EventTag`
- `SubjectName = None`，除非后续技能系统引入明确 subject。
- `WindowName = FSGSRuleEventPayload.TimingPoint.Step`
- payload 固定使用 `FSGSRuleEventPayload`

Registry 对 Trigger 使用 `DispatchAll` 语义：无候选是成功 no-op；命中的 Trigger 按 priority 降序、注册顺序升序同步执行；`Validate` / `Execute` 出错会中断当前事件发布。

## Resolution Frame State

只有跨命令、跨响应窗口或需要 resume 的结算事实才进入 frame state。

规则：

- 具体玩法 state 放在对应 Rule 模块里，不放进 `Rules/Resolution/SGSResolutionStack.*`。
- `ResolutionStack` 只保存通用 frame、stack、window、continuation。
- frame state 不直接修改 `USGSGameContext`。
- 状态变更仍必须通过 `EffectPipeline`。

若只是一次 Rule 执行内的临时变量，不要创建 frame state。

## 执行边界

Rule 不直接修改权威游戏状态。允许的路径是：

```text
Rule -> ISGSRuleRuntime -> EffectPipeline -> USGSGameContext
```

Rule 可以打开响应窗口、push / complete resolution frame、发布受控 timing event，但不能绕过 Runtime 直接操作 Driver pending 状态。

## 最小验收

新增 Rule 至少考虑：

- payload 类型错误不会执行 Rule。
- descriptor 能被 Registry 查询到，排序稳定。
- 非法输入返回可诊断错误码。
- 状态变更经过 EffectPipeline。
- 若 push frame，结算结束后 frame 被 pop 或明确 abort。
- 修改代码后运行相关自动化与 `graphify update .`。
