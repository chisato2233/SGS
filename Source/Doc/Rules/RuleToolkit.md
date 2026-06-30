# Rule Toolkit Rules

本文件规定 Plan 0012 工具库的使用边界，不描述具体类实现细节。需要代码结构和调用关系时使用 graphify。

---

## 默认入口

规则层新代码优先使用：

- CommandRouter：玩家、AI、RPC、回放输入的统一意图入口。
- RandomAudit：唯一业务随机入口。
- IndexedStore / StableHandle：稳定实体与索引事实源。
- TargetQuery：目标和实体候选查询。
- ActiveEffectTimeline：持续效果生命周期。
- EffectPipeline：规则效果执行、挂起、恢复和连锁入口。
- ReplayLog：CommandLog、RandomLog、EventLog 的聚合与校验地基。

---

## 禁止平行体系

- 不重新创建独立牌堆容器作为规则事实源。
- 不在规则层直接调用裸随机源。
- 不绕过 Command 执行玩家/AI/RPC 意图。
- 不把目标选择规则散落在 UI、AI 或单张卡效果里。
- 不让效果直接修改规则世界而不经过 Context / Pipeline / Log 边界。

---

## 牌区事实源

牌区状态以权威对局状态上下文中的 Store / 有序牌区索引语义为事实源。文档只记录这个边界，不维护具体 Store 结构；具体类、字段和影响面由 graphify 查询。
