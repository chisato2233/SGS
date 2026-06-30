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
- `NNNN-MN-短横线命名.md`：已有父计划的实施子里程碑（如 `0011-M1-...`）。只在父计划已经是长期路线 / 架构计划，且需要把实际落地拆成可验收切片时使用。
- `0000-RawRequirements.md` 是**保留文件**：专门按时间顺序追加记录用户键入的原始需求。它是历史档案，后期可能非常冗长；不要默认全文读取，只在追溯原始需求时按需检索 / 局部阅读。
- `_Template.md` 是**计划模板**：新建计划时复制它，不要直接修改模板本身。
- `Archive/` 是**已完成计划归档目录**：状态为 `Archived` 的计划放在这里，主目录只保留当前工作面。计划是否活跃由路径区分：`Source/Doc/Plan/*.md`（排除 README、模板、RawRequirements）是当前工作面；`Source/Doc/Plan/Archive/*.md` 是历史，默认不读。

示例：
```text
Source/Doc/Plan/
├── README.md                  # 本说明
├── _Template.md               # 计划模板
├── 0000-RawRequirements.md    # 原始需求档案（持续追加）
├── 0001-table-skeleton.md     # 活跃计划：桌面骨架
├── 0002-turn-loop.md          # 活跃计划：回合循环
├── 0011-M1-minimal-ui.md      # 父计划 0011 的实施子里程碑
└── Archive/
    └── 0000-example-done.md   # 已完成并归档的计划
```

---

## 2. 新建一个计划的流程

1. 在 `0000-RawRequirements.md` 顶部追加本次用户的原始需求条目（保留原话）。
2. 复制 `_Template.md` 为 `NNNN-短名.md`，填写方案。
3. 将计划文件放在 `Source/Doc/Plan/` 主目录；路径本身表示它属于当前工作面，不再维护单独索引表。
4. 实施过程中持续更新该计划文档的 `状态` 与 `进度`。
5. 完成后：先把计划 `状态` 改为 `Done` 并完成验证；确认没有后续执行项后，移动到 `Archive/`，把 `状态` 改为 `Archived`，并回写根目录 `ProjectBrief.md` 第 5 节「当前项目状态」。

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

## 4. 计划发现与读取规则

本目录不维护活跃 / 归档索引表，避免 README 随计划数量膨胀并与文件状态漂移。后续 Agent 按路径判断计划状态：

- `Source/Doc/Plan/*.md`：当前工作面。排除 `README.md`、`_Template.md`、`0000-RawRequirements.md` 后，剩余计划即活跃或 Ready 计划；按当前任务只读取相关文件。
- `Source/Doc/Plan/Archive/*.md`：历史计划。默认不读；只有追溯决策、验收证据或历史原因时，才按文件名 / git / graphify / 搜索结果读取具体归档文件。
- `Source/Doc/Plan/0000-RawRequirements.md`：原始需求历史。只追加，不整理；可能非常长。默认不全文读取，只有需要核对用户原话或需求来源时再检索并局部阅读。
- `ProjectBrief.md` 第 5 节只记录项目级总状态和下一步，不承担计划索引职责。

常用发现命令：

```powershell
rg --files Source\Doc\Plan -g "*.md" -g "!README.md" -g "!_Template.md" -g "!0000-RawRequirements.md" -g "!**/Archive/**"
rg --files Source\Doc\Plan\Archive -g "*.md"
```

需要理解计划与代码影响面时，优先使用 graphify query/path/explain，而不是把索引信息手写进 README。
