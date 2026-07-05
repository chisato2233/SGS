# Content Authoring Rules

本文件规定新增 Card、Skill 和内容包时如何划分数据与规则。它不维护卡牌或技能清单；查询当前内容结构用 graphify。

---

## 内容包

一组相关卡牌、技能或模式规则应归入内容包，例如：

- `BasicCards`
- `Tricks`
- `Equipment`
- `StandardSkills`

内容包可以拥有：

- 数据定义。
- Rule 注册入口。
- 内容包共享 helper。
- 多个 RuleKind 目录下的具体 Rule。

默认通过内容包注册入口接入默认规则集。全局默认注册函数不应直接 include 每个具体 Rule header。

## Card 定义

Card 的静态事实优先放入数据：

- 卡牌名。
- 花色、点数、类型。
- 展示和资源引用。
- 可数据化的基础属性。

Card 的行为不应塞进 `USGSCard` 运行态实例。`USGSCard` 表示一张物理牌的运行态标识与当前静态字段快照；玩法效果由 Rule / EffectPipeline 承担。

新增卡牌时优先判断：

- 是否能复用已有 Action / Response Rule。
- 是否只需要新增数据。
- 是否需要新增 Rule payload 或 frame state。
- 是否需要新增 Trigger / Modifier / ViewAs Rule。

不要为每张普通牌默认创建独立 C++ 类。只有现有规则抽象无法表达时，才新增具体 Rule。

## Skill 定义

Skill 是内容概念，不等于单个 Rule。

一个 Skill 未来可以注册多个内部规则：

- TriggerRule：响应 timing event。
- ModifierRule：修正距离、目标数、使用次数等查询。
- ViewAsRule：把牌或选择转化为新的规则意图。
- ActionRule：主动发动技能。
- GameRule：少量隐藏记录或全局 bookkeeping。

Skill 不应直接修改 `USGSGameContext`。所有状态变化仍通过 RuleRuntime 和 EffectPipeline。

## 数据与代码分界

优先数据化：

- 静态名称、类型、标签。
- 展示文本、资源引用。
- 简单数值、标签配置。
- 内容包启用 / 禁用配置。

优先代码化：

- 需要时序、响应窗口、嵌套结算的行为。
- 需要复杂目标选择、修正查询或 view-as 的行为。
- 需要严格审计、ReplayLog、随机审计的行为。

若一个内容既需要数据也需要代码，数据只提供静态参数，Rule 负责解释和执行。

## 新增内容的最小流程

新增 Card / Skill 时先回答：

- 它属于哪个内容包。
- 它是否需要新 CommandType，通常答案应该是“不需要”。
- 它是否需要 Action / Response / Trigger / Modifier / ViewAs 中的一个或多个 Rule。
- 它是否需要新的 Rule payload。
- 它是否需要 Resolution frame state。
- 它的状态变更是否全部经过 EffectPipeline。
- 它是否需要 AI / UI 暴露新的 decision option。

实现后：

- 补最小自动化覆盖真实游戏路径。
- 更新相关 Plan 的状态或进度记录。
- 修改代码后运行 `graphify update .`。
