# Plan 系统说明

本文件夹是 SGS 项目的**开发计划与需求档案库**。配合根目录 `ProjectBrief.md` 使用：

- `ProjectBrief.md` → 项目的长期稳定信息与当前总状态。
- 本文件夹 → 每一次具体开发的方案、进度，以及用户的原始需求记录。

---

## 1. 文件命名约定

```text
NNNN-短横线命名.md
```

- `NNNN`：4 位零填充的递增序号，按创建顺序分配（如 `0001`、`0002`）。
- `0000-RawRequirements.md` 是**保留文件**：专门按时间顺序追加记录用户键入的原始需求。
- `_Template.md` 是**计划模板**：新建计划时复制它，不要直接修改模板本身。
- `Archive/` 是**已完成计划归档目录**：状态为 `Archived` 的计划放在这里，主目录只保留当前工作面。

示例：
```text
Source/Doc/Plan/
├── README.md                  # 本说明
├── _Template.md               # 计划模板
├── 0000-RawRequirements.md    # 原始需求档案（持续追加）
├── 0001-table-skeleton.md     # 活跃计划：桌面骨架
├── 0002-turn-loop.md          # 活跃计划：回合循环
└── Archive/
    └── 0000-example-done.md   # 已完成并归档的计划
```

---

## 2. 新建一个计划的流程

1. 在 `0000-RawRequirements.md` 顶部追加本次用户的原始需求条目（保留原话）。
2. 复制 `_Template.md` 为 `NNNN-短名.md`，填写方案。
3. 在下方 **活跃计划索引** 表格中登记新条目。
4. 实施过程中持续更新该计划文档的 `状态` 与 `进度`。
5. 完成后：先把计划 `状态` 改为 `Done` 并完成验证；确认没有后续执行项后，移动到 `Archive/`，把 `状态` 改为 `Archived`，从活跃索引移到归档索引，并回写根目录 `ProjectBrief.md` 第 5 节「当前项目状态」。

### 2.1 计划质量门

计划文档不是脚手架清单。一个 Plan 进入 `Ready` 前，至少要回答：

- 实际游戏中谁会使用它：规则层、AI、UI、网络、数据资产、回放或调试工具中的哪个真实入口会调用它。
- 它如何接入现有架构：是否复用 Command、RandomAudit、IndexedStore/TargetQuery、ActiveEffectTimeline、EffectPipeline、ReplayLog、GAS Adapter 等既有入口。
- 它的关键不变量和失败路径是什么：非法输入、过期请求、空数据、缺失资产、网络迟到、随机重放不一致等情况怎么处理。
- 它如何支持后续扩展：新卡、新技能、新阶段、新目标规则、新数据来源接入时是否需要推倒重来。
- 哪些是临时占位：若使用 fake data、placeholder、占位 AI、占位资产或 smoke-only 路径，必须写清原因、隔离范围、替换条件和后续落点。

`Done` 的验收不接受“文件已创建 / 类已存在 / 能走假数据演示”作为唯一依据；必须包含至少一条真实游戏路径或可解释的 smoke 路径。

---

## 3. 计划状态取值

| 状态 | 含义 |
|---|---|
| `Draft` | 草拟中，方案未定 |
| `Ready` | 方案已定，待实施 |
| `In Progress` | 实施中 |
| `Done` | 已完成并验证 |
| `Archived` | 已完成、已归档，不再作为当前工作面 |
| `Abandoned` | 已废弃（保留存档，注明原因） |

---

## 4. 活跃计划索引

> 新建计划后在此登记。最新的放在最上面。状态为 `Archived` 的计划不得留在本表。

| 序号 | 标题 | 状态 | 说明 |
|---|---|---|---|
| 0011 | Native Code-first UI 架构 | Ready | Slate/UMG/CommonUI 按需组合 + SGSUI 薄封装；不使用 WebView/React/Vue 作为主 UI，不自研 Gameface |
| 0004 | 对局数据模型 | In Progress | 卡牌/牌区/玩家状态 + 通用移牌/摸牌/弃牌/伤害/回复/距离原语 + 事件（`USGSGameContext`）|
| 0003 | 权威对局骨架 | In Progress | 服务器侧回合阶段机闭环 + 决策代理 + 事件总线 + 占位 AI + GameMode 入口 |
| 0002 | 核心逻辑：架构/分层/路线图 | In Progress | 服务器权威 + 多人/AI 并存 + 异步结算；分层规范 + `Core/` 基础层 + 后续 Plan 路线图 |
| 0001 | 导入太阳神三国杀素材 | In Progress | QSanguosha 卡牌/武将/配音/音乐/UI 素材导入 `Content/ImportedAssets/` |
| 0000 | 原始需求档案 | — | 用户原始需求的时间线记录 |

---

## 5. 归档计划索引

> 归档计划位于 `Source/Doc/Plan/Archive/`。需要追溯历史决策时读取；不要直接在归档计划里继续追加新实现任务，新的实质工作应新建或使用活跃 Plan。

| 序号 | 标题 | 状态 | 位置 | 说明 |
|---|---|---|---|---|
| 0012 | SGS 基础工具库开发计划 | Archived | `Archive/0012-sgs-foundation-toolkit.md` | P0+P1 基础工具库：错误/结果、Command、RandomAudit、IndexedStore、TargetQuery、Timing/ActiveEffect、EffectPipeline、ReplayLog 地基；P2 工具按需小迭代 |

<!-- 新计划登记示例：
| 0001 | 桌面骨架 | Ready | 4 人桌面 + 牌堆/弃牌堆/手牌区 |
-->
