# Rules README — 按需规则路由

本目录存放任务相关领域细则。Agent 启动时先读 `Source/Doc/Rulers.md`，再读本文件，并只按任务选择具体领域规则。

架构、编码、工作流规则已经并入 `Rulers.md`，因为它们属于每个 Agent 都必须知道的宪法内容。本目录不再维护第二套架构/编码/工作流正文。

这些文件与 graphify 正交：规则文件只写约束、边界、默认做法和禁止事项；不写代码结构、类图、调用关系、文件清单或“某类现在做了什么”。需要这些事实时使用 graphify。

---

## 读取路由

| 任务 | 读取 |
|---|---|
| 架构、C++ / UE 编码、构建、Git、LFS、graphify | 已并入 `../Rulers.md` |
| 卡牌/技能/阶段/座次/花色等规则建模 | `GameplayModeling.md` |
| UI / Slate / UMG / CommonUI | `UI.md` |
| GAS / GameplayEffect / GameplayTag / Attribute | `GAS.md` |
| Command、RandomAudit、Store、TargetQuery、EffectPipeline、ReplayLog | `RuleToolkit.md` |

只读取和当前任务有关的文件。不要默认读取整个目录。

---

## 规则文件写作约束

- 每个文件只写某一类长期规则，不记录开发流水账。
- 规则应说明“必须 / 禁止 / 默认优先级 / 何时例外”，避免叙述具体代码功能。
- 架构、编码、工作流这类高频通用规则必须进入 `Rulers.md`，不要在本目录另开正文。
- 不在规则文件里维护类清单、调用链、依赖图或模块地图；这些由 graphify 提供。
- 不把历史拆成大量 decision 文件；历史细节保留在 git、归档 Plan、RawRequirements 和 graphify 中。
- 若规则来自某个 Plan，只在规则里保留最终结论；详细理由由归档 Plan 或 git 追溯。
