# Command Authoring Rules

本文件规定新增 Command、Command payload 与 CommandType 的写法。Command 是外部意图协议，不是规则执行本身。

---

## Command 的职责

Command 只表达玩家、AI、未来 RPC 或回放提交了什么意图。

Command 不应该：

- 直接改变游戏状态。
- 保存 Rule 结算状态。
- 复用 Rule payload 作为外部协议。
- 为 AI、UI、RPC 分别创建平行入口。

所有外部输入都应进入 `CommandRouter`，再由 `CommandType` 翻译为 `FSGSRuleInvocation`。

## 什么时候新增 CommandType

优先复用已有 CommandType。只有满足以下情况之一，才新增 CommandType：

- 外部意图形态不同，例如选择牌、选择目标、响应窗口、发动技能的输入结构确实不同。
- 校验规则属于外部输入协议，而不是具体 Rule 行为。
- UI / AI / RPC / Replay 都需要稳定识别这一类意图。

不要因为新增一张牌或一个技能就新增 CommandType。多数卡牌和技能应复用通用使用牌、响应牌、发动技能等 CommandType。

## Command Payload

Command payload 是不可信输入，只描述选择结果。

默认要求：

- 字段保持接近输入事实，例如 card id、target seat、selected ids。
- 不放服务器推导事实，例如当前窗口来源、最终伤害、处理中的 frame state。
- 不放 Rule descriptor、priority、effect step 等内部规则概念。
- 必须能被 CommandRouter 和 CommandType 校验。

服务器上下文事实应来自 `FSGSCommandExecutionContext`，再进入 `BuildRuleInvocation`。

## BuildRuleInvocation

CommandType 的长期职责是：

```text
Command + CommandExecutionContext -> RuleInvocation + typed Rule payload
```

构造时应明确：

- `RuleKindTag`
- `IntentTag`
- `SubjectName`
- `ActorSeat`
- `WindowName`
- `SourceCommandId`
- `SourceRequestId`
- typed payload

不要重新引入万能 request 或 shared mutable 参数包。

## 校验边界

Common validation 处理所有 Command 都必须满足的条件。CommandType validation 处理该外部意图的输入合法性。

Rule validation 处理具体玩法合法性，例如卡牌名、目标数量、距离、窗口是否匹配等。

不要把具体牌 / 技能规则塞进 CommandType。CommandType 只负责把意图翻译成规则调用。

## 最小验收

新增 CommandType 至少考虑：

- 合法 Command 能构造 typed RuleInvocation。
- 错 seat、错 request、错 phase、错 window 会被拒绝。
- payload 缺失或类型错误能返回可诊断错误。
- AI、UI、未来 RPC 可以复用同一 Command。
- ReplayLog 能关联 CommandId 与后续 RuleInvocation。
