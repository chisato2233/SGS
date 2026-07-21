# 0017-M3 — 标准对局流程与牌类结算

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-21 |
| 最近更新 | 2026-07-21 |
| 关联需求 | `0000-RawRequirements.md` 中的第 33 条 |
| 关联代码 | `Source/SGS/Server/Engine/`、`Source/SGS/Server/Rules/`、`Source/SGS/Server/Timing/` |

---

## 1. 目标

在 GameDriver、TimingEvent、ResolutionStack 与 EffectPipeline 上补齐标准阶段和基础牌以外的牌类结算，使阶段触发、连续响应、濒死、死亡与胜负可以在同一权威流程中组合。

## 2. 实施切片

- 每个阶段发布 Before、Begin、End、After 四个稳定时机；具体阶段内容仍由 GameDriver 蹦床推进。
- 用通用卡牌结算帧承载多目标游标、响应游标和取消状态，不按卡牌另建事件机。
- 普通锦囊接入无懈可击响应；群体牌按稳定座位顺序逐个结算。
- 装备统一进入装备区并替换同槽牌；距离、攻击范围和防具效果通过 Modifier/Trigger 接口实现。
- 延时锦囊进入判定区，在判定阶段执行权威判定并移动到弃牌堆或下一目标。
- 濒死、死亡奖惩和身份胜负沿用现有 ResolutionStack/EffectPipeline 链路，并补齐回放可见的取消与替换结果。

## 3. 验收

- 阶段和同一时机触发器按优先级、注册顺序稳定执行。
- 多目标锦囊与多次响应不会递归爆栈，Pass 与有效响应均能恢复父结算。
- 装备和判定区成为 GameContext 的真实牌区消费者，不在 UI 保存权威状态。
- 不新增或运行测试代码；只做生产代码编译和静态检查。
