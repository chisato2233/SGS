# Authoring Rules README

本目录存放“新增重要游戏工程概念时怎么写”的作者指南。它回答的是写法、边界和选择标准，不维护现有代码清单。

这些文件与 graphify 分工如下：

- 作者指南：说明新增内容的落点、不变量、禁止事项、最小验收。
- graphify：查询当前有哪些类、文件关系、调用路径、依赖影响面。
- Plan：记录一次具体改造的方案、取舍、进度和验收。

---

## 读取路由

| 任务 | 读取 |
|---|---|
| 新增 / 修改 Rule、Rule payload、Rule descriptor、Resolution frame state | `RuleAuthoring.md` |
| 新增 / 修改 Command、Command payload、CommandType、外部意图入口 | `CommandAuthoring.md` |
| 新增 / 修改 Card、Skill、内容包、内容注册入口 | `ContentAuthoring.md` |

只读取和当前任务有关的作者指南。不要把本目录当代码索引；需要事实查询时使用 graphify。

## 扩展标准

只有当一个工程概念满足以下条件时，才新增作者指南：

- 会被频繁扩展。
- 写错会破坏服务器权威、规则结算、数据边界或测试可追溯性。
- 需要开发者在多个落点之间做选择。

不为单个具体类、单张牌、单个技能或一次性工具创建作者指南。
