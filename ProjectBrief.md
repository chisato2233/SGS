# ProjectBrief — SGS（三国杀卡牌桌面游戏）

> 本文件是所有 Agent 的第一入口，也是项目驾驶舱。
> 它只记录长期稳定且需要快速把控的信息：项目概述、技术栈、活跃 Plan 状态、操作入口、全局方向和 graphify 使用边界。
> 具体架构 / 编码 / 工作流底线属于 `Source/Doc/Rulers.md`；领域细则属于 `Source/Doc/Rules/*`；具体任务方案属于 `Source/Doc/Plan/*`。

---

## 1. 项目概述

- **目标**：实现一个三国杀（SGS）卡牌桌面游戏。
- **形态**：回合制、UI 驱动的卡牌对战；支持多人联机与 AI 玩家并存。
- **当前阶段**：阶段 1 — 核心逻辑开发。优先把权威规则、牌区状态、结算流程和可验收的自动化路径跑通，再逐步完善 UI、网络和演出。
- **核心取舍**：留在 Unreal Engine + C++。三国杀的主要复杂度在规则结算，项目以服务器权威逻辑层为单一真相源，表现层只显示状态和采集输入。
- **素材来源**：QSanguosha 素材已导入 `Content/ImportedAssets/`，按其 CC BY-NC-ND 授权仅用于非商业学习 / 原型用途。

---

## 2. 技术栈

| 项 | 内容 |
|---|---|
| 引擎 | Unreal Engine **5.7** |
| 语言 | C++ 为主；蓝图仅用于美术 / UI 绑定或调试可视化 |
| 主模块 | `SGS` Runtime（`Source/SGS/`） |
| 主要依赖 | `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayAbilities`, `GameplayTasks`, `GameplayTags`, `Slate`, `SlateCore`, `UMG` |
| 网络方向 | 服务器权威；UE 复制 + RPC；监听服务器起步，预留专用服务器 |
| AI 方向 | 服务器侧决策代理；真人与 AI 共用同一决策抽象 |
| UI 方向 | Native Code-first UI：Slate/UMG 纯代码，CommonUI 按需评估；不以 WebView/React/Vue/Noesis/Gameface 作为主 UI |
| 输入 | Enhanced Input |
| 插件 | `GameplayAbilities`（运行时底座）、`ModelingToolsEditorMode`（编辑器默认工具） |
| 目标文件 | `SGS.Target.cs` / `SGSEditor.Target.cs` |

改动技术栈、模块依赖、插件或第三方库时，同步更新本表；具体准入规则看 `Rulers.md` 和相关 Plan。

---

## 3. 全局把控

- **先做可运行规则，再做漂亮表现**：核心验收优先看真实规则路径、自动化测试、命令行 smoke 和可追溯日志。
- **真人与 AI 同源**：AI、真人、回放和未来网络输入都应进入同一套权威逻辑，不为某个入口写平行规则。
- **逻辑与表现解耦**：UI、动画、音效、摄像机不承载规则事实；规则层输出状态、事件和决策请求。
- **扩展面向卡牌 / 技能 / 武将**：新增内容应通过数据、标签、规则管线和既有工具库接入，避免把可扩展概念写死。
- **代码结构交给 graphify**：文档写意图、边界、状态和验收；类关系、调用路径、依赖关系、影响面以 `graphify-out/` 为准。

---

## 4. 当前状态与活跃 Plan

### 已完成基线

- `0001` 素材导入、`0002` 核心逻辑路线、`0003` 权威对局骨架、`0004` 对局数据模型、`0005` 基础牌可玩规则、`0012` 基础工具库、`0013` Command -> Rule -> Effect 管线、`0014` Rule 层骨架长期重构均已完成并归档到 `Source/Doc/Plan/Archive/`。
- `0003` 已通过命令行 smoke，`0004` 已通过自动化验收 `SGS.Plan0004.GameContextPrimitives`。
- 规则层已有核心地基：Command、RuleRegistry、RandomAudit、IndexedStore/TargetQuery、Timing/ActiveEffect、EffectPipeline、ReplayLog、`USGSGameContext`、`USGSGameDriver`。Plan 0005 已补齐杀 / 闪 / 桃与濒死求桃的最小可玩规则纵切；Plan 0013 已将基础牌结算迁移到 `Command -> Rule -> EffectPipeline` 单一管线。代码细节不要在本文件展开，使用 graphify 查询。

### 当前工作面

- `Source/Doc/Plan/0011-native-code-first-ui.md` — `Ready`。UI 长期技术路线父计划。
- `Source/Doc/Plan/0011-M1-minimal-code-first-table-ui.md` — `In Progress`。最小代码优先牌桌 UI 纵切代码已实现，PIE 默认进入 8 人 Nova 牌桌；摄像机、全视口背景、武将面板和压叠手牌底栏已按 NoName 布局纠偏，1280x720 离屏 UI 截图通过人工检查；`SGS.Plan0011M1.LocalUI.DecisionBridge` 自动化通过，等待 PIE / Standalone 真实点击与主观视觉验收。
- `Source/Doc/Plan/0011-M2-react-inspired-ui-foundation-events.md` — `In Progress`。M2.2-M2.6 生产实现已完成：业务无关 state/signal/lifetime/context、Table Controller/Shell/叶组件、Host adapter、精准区域更新以及真实 Toast/Focus 消费者已经接入；Development Editor 编译通过。按用户要求未运行自动化，等待独立测试模型完成 Core、交互与多 LocalPlayer 验收。
- 当前主要 `In Progress` 工作面：`0011-M1` 视觉 / 点击验收与 `0011-M2` 独立测试验收。Rule 层骨架已在 `0014` 收束，后续真实玩法逻辑应另起 gameplay / card / skill 计划。
- `Source/Doc/Plan/0000-RawRequirements.md` 是原始需求历史，不算活跃 Plan，默认不要全文读取。

### 下一步建议

- 在 PIE / Standalone 中验收 `0011-M1` 的 8 人 Nova HUD、窗口缩放与 Slash / Dodge / Peach / Pass 点击操作；通过后将 `0011-M1` 按流程归档或推进父计划 0011 的下一阶段。
- 由独立测试模型验收 `0011-M2` 的 observable/batch/selector、lifetime teardown、typed signal、Table 真实操作链和多 LocalPlayer 隔离；通过后再将 M2 标记 Done。Motion 等两个真实消费者出现后再提炼，CommonUI / 菜单栈按明确需求另立计划。
- 新建真实玩法计划：优先明确距离 / 目标合法性 / 坐骑与技能修正等游戏逻辑边界，再推进更多卡牌、锦囊、装备和首个武将技纵切。

---

## 5. 操作入口

新会话或切换任务时，按这个最小路径建立上下文：

1. 读本文件，确认项目状态和下一步。
2. 读 `Source/Doc/Rulers.md`，获得全局规则。
3. 读 `Source/Doc/Rules/README.md`，只打开当前任务需要的领域细则。
4. 读 `Source/Doc/Plan/README.md`，再按路径读取相关 Plan。
5. 需要理解代码结构、依赖关系或影响面时，先用 graphify。

Plan 读取边界：

- `Source/Doc/Plan/*.md` 主目录中，排除 `README.md`、`_Template.md`、`0000-RawRequirements.md` 后，剩余文件就是当前工作面。
- `Source/Doc/Plan/Archive/*` 是历史计划，默认不读；只有追溯决策或验收证据时读取具体文件。
- `0000-RawRequirements.md` 可能非常长，只在核对用户原话或需求来源时检索 / 局部阅读。

常用操作入口：

```powershell
# 构建
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Unreal.ps1 -Action Build -Configuration Development

# 查询代码 / 文档图谱
graphify query "<问题>"
graphify path "<A>" "<B>"
graphify explain "<概念>"

# 修改代码后更新图谱
graphify update .

# 生成人类可读图视图后过滤 UE/Core 噪声节点
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\GraphifyVisualFilter.ps1
```

更细的构建、测试、提交、LFS、状态回写规则以 `Rulers.md` 为准。

---

## 6. 维护原则

- 本文件只放项目级总状态和长期方向，不沉淀任务细节、类图、接口清单或完整规约。
- 每次完成实质性 Plan 或改变项目阶段时，更新第 4 节；不要把 Plan README 变成索引表。
- 如果内容开始像“所有 Agent 必须遵守的规则”，移到 `Rulers.md`；如果内容只服务某个领域，移到 `Source/Doc/Rules/*`；如果内容是一次开发的方案或验收，留在对应 Plan。

Last reviewed: 2026-07-11
