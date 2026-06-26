# ProjectBrief — SGS（三国杀卡牌桌面游戏）

> **本文件是所有 Agent 的唯一起点。** 任何 Agent 在开始任何工作前，必须先完整阅读本文件。
> 本文件只记录**长期稳定**的信息：项目概览、技术栈、文档导航、当前状态、协作约定。
> 具体某一次开发的细节方案放在 `Source/Doc/Plan/`；已完成计划归档到 `Source/Doc/Plan/Archive/`。不要把计划细节写进本文件。

---

## 1. 项目概览

- **目标**：实现一个三国杀（SGS）卡牌桌面游戏。
- **形态**：回合制、UI 驱动的卡牌对战。**服务器权威（server-authoritative）**架构，**多人联机与 AI 玩家并存**——真人与 AI 通过**同一决策接口**接入同一套权威逻辑。先把规则与流程在权威逻辑层跑通，再完善演出。
- **引擎选择结论**：留在 **Unreal Engine + C++**。三国杀的核心复杂度在**卡牌/技能的结算逻辑**，与引擎无关；因此架构上要求**逻辑层与表现层彻底解耦**，逻辑层用 C++、服务器权威执行，不反向依赖 UI 与具体网络实现。

---

## 2. 技术栈

| 项 | 内容 |
|---|---|
| 引擎 | Unreal Engine **5.7** |
| 语言 | C++（核心逻辑） + 少量蓝图（仅用于美术/UI 绑定） |
| 主模块 | `SGS`（Runtime，`Source/SGS/`） |
| 模块依赖 | `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayAbilities`, `GameplayTasks`, `GameplayTags` |
| 网络 | **服务器权威**：UE 复制（GameState/PlayerState）+ 可靠 RPC（决策请求/应答通道）。Engine 自带网络；监听服务器起步，预留专用服务器。 |
| AI | 服务器侧**决策代理**，与真人共用 `ISGSDecisionAgent` 接口接入；AI 逻辑**参考 QSanguosha（概念移植，非代码拷贝）**。 |
| UI | Native Code-first UI（Slate/UMG 纯代码；SGSUI 薄封装；CommonUI 按需评估；不使用 Designer 拖控件） |
| 输入 | Enhanced Input |
| 渲染 | Substrate + DX12 SM6（UE5.7 默认；卡牌游戏对渲染要求低，不重点投入） |
| 编辑器目标 | `SGS.Target.cs` / `SGSEditor.Target.cs`（BuildSettings V6，IncludeOrder Unreal5_7） |
| 插件 | `GameplayAbilities`（运行时，GAS 底座）、`ModelingToolsEditorMode`（编辑器专用，默认带的） |

> 改动技术栈（新增模块依赖、插件、第三方库）时，必须同步更新本表。

---

## 3. 目标架构（概览）

**服务器权威 + 表现解耦**。游戏逻辑只在服务器执行，客户端只显示与采集输入；真人与 AI 通过同一决策接口接入：

```text
客户端表现层 (Slate/UMG wrapper / Actor) 只显示与采集输入；收复制状态、发玩家指令
   ↑ 复制状态/事件   ↓ 玩家指令(RPC)
─────────────────  网络边界（仅在此跨网）  ─────────────────
服务器权威逻辑层 (C++ / UObject)   回合阶段机、技能结算、判定、事件总线（核心，单一真相源）
   ↑ 请求决策 / ↓ 返回决策（统一接口 ISGSDecisionAgent）
决策代理：真人代理（转发到 PlayerController，UI 收集）｜ AI 代理（服务器侧计算，参考 QSanguosha）
   ↑
数据层 (DataTable / DataAsset：卡牌·武将·技能定义)
```

核心设计要点（详见对应 Plan 文档）：
- **服务器权威**：所有结算只在服务器进行；客户端不持有可信状态，只做表现。手牌等私密信息**只复制给拥有者**。
- **决策代理抽象**：逻辑需要输入时只面向 `ISGSDecisionAgent` 请求决策；真人=网络/UI 应答，AI=服务器侧计算，二者对逻辑层透明 → 多人与 AI 天然并存。
- **异步、非阻塞**：等待玩家决策时逻辑层挂起而非阻塞游戏线程，应答或超时（→默认/AI 托管）后恢复。这是与 QSanguosha 阻塞式 RoomThread 的关键差异。
- **回合阶段机**：准备 → 判定 → 摸牌 → 出牌 → 弃牌 → 结束。
- **事件总线 + 触发器**：技能 = 「触发时机 + 条件 + 效果」，统一注册到事件系统，避免把逻辑写死。
- **GAS 边界**：引入 GAS 作为 Attribute、GameplayEffect、GameplayTag、GameplayCue / 表现桥接与可复用效果载体；动作命令、随机审计、回放日志、牌区移动、响应窗口和三国杀式结算顺序仍由 SGS C++ 规则管线控制。
- **数据驱动**：卡牌/武将静态定义走 `DataTable`/`DataAsset`；属性、标签与持续/即时状态优先映射到 GAS，卡牌结算语义走 SGS C++ 效果管线与适配层。

> 这里只放**稳定的架构愿景**。具体类设计、文件清单、接口签名都放进 `Source/Doc/Plan/` 的对应计划文档；代码结构由 graphify 维护。

---

## 4. 文档与代码结构导航（Agent 必读）

Agent 开始工作时按以下顺序获取上下文：

0. **`AGENTS.md`（Codex 专用）** — 只作为自动发现/工具适配层；它必须指回本文件，不承载项目事实。
1. **本文件 `ProjectBrief.md`** — 了解项目全貌、技术栈、当前状态。
2. **`Source/Doc/Rulers.md`** — 项目级编码约束与工作流，所有 Agent 必读。
3. **`Source/Doc/Plan/README.md`** — 了解计划系统的使用方式与索引。
4. **`Source/Doc/Plan/` 下的相关计划文档** — 找到与当前任务对应的活跃计划，按其执行；历史决策按 README 索引到 `Source/Doc/Plan/Archive/` 查询。
5. **代码结构图谱** — 由 **graphify** 维护，项目级产物位于 `graphify-out/`。需要理解既有代码结构/依赖关系时，以 graphify 的产物为准，**不要在文档里重复手写代码结构**，避免与代码脱节。

职责边界：
- `ProjectBrief.md` → 长期稳定信息（变化慢）。
- `AGENTS.md` → Codex 入口适配与 graphify 使用规则（指回 ProjectBrief，不另立事实源）。
- `Source/Doc/Plan/*` → 当前活跃开发的具体方案与进度（变化快）。
- `Source/Doc/Plan/Archive/*` → 已完成计划的历史决策与验收记录（只追溯，不作为当前工作面）。
- graphify → 代码层面的结构/依赖（随代码变化，自动维护）。

---

## 5. 当前项目状态

- **阶段**：阶段 1 — 核心逻辑开发（**进行中**）。架构方向：服务器权威 + 多人/AI 并存（见第 3 节）。
- **代码现状**：模块入口 `Source/SGS/SGS.h`、`SGS.cpp` + `Core/` 基础层（日志分类、通用类型）。游戏逻辑按 Plan 路线图分期实现。
- **已完成**：
  - [x] 确认引擎与技术栈（UE 5.7 + C++，模块 `SGS`）。
  - [x] 建立文档系统（本文件 + `Source/Doc/Plan/` + `Source/Doc/Rulers.md`）。
  - [x] 导入 QSanguosha 素材（`Content/ImportedAssets/`，CC BY-NC-ND，非商用）。见 Plan 0001。
  - [x] **架构定调**：服务器权威 + 决策代理（真人/AI 并存）+ 异步非阻塞结算。见 Plan 0002。
  - [x] 代码分层骨架 + `Core/` 基础层（`SGSLogChannels`、`SGSTypes`）。见 Plan 0002。
  - [x] **UI 路线定调**：Native Code-first UI（Slate/UMG/CommonUI 按需组合 + SGSUI 薄封装），不使用 WebView/React/Vue 作为主 UI，不自研 Gameface。见 Plan 0011。
  - [x] Codex 入口适配：`AGENTS.md` 接入项目文档启动协议，graphify 图谱初始化到 `graphify-out/`。
  - [x] **GAS 策略改定**：引入 GameplayAbilities/GameplayTasks/GameplayTags 作为属性、标签、GameplayEffect 与 Cue 底座；SGS 规则核心仍自研。见归档 Plan `Source/Doc/Plan/Archive/0012-sgs-foundation-toolkit.md`。
  - [x] **SGS 基础工具库 P0/P1**：M1 Core Utility、M2 Command + RandomAudit、M3 IndexedStore + TargetQuery、M4 Timing/ActiveEffect/GAS Adapter、M5 EffectPipeline + ReplayLog 已完成；P2 工具保留为按需小迭代。见归档 Plan `Source/Doc/Plan/Archive/0012-sgs-foundation-toolkit.md`。
- **进行中**：
  - [~] Plan 0003：权威对局骨架（回合阶段机闭环 + 决策代理 + 事件总线 + 占位 AI + GameMode）。复制/RPC 拆到 0003N（需可编译 UE 环境）。
  - [~] Plan 0004：对局数据模型（卡牌/牌区/玩家状态 + 移牌/摸牌/弃牌/伤害/回复/距离原语 + 事件，`USGSGameContext`）。
- **下一步（按 Plan 0002 路线图）**：
  - [ ] 0005 杀/闪/桃（濒死/求桃）→ 0006 判定/延时锦囊 → 0007 即时锦囊 → 0008 装备 → 0009 武将技 → 0010 AI 代理 → 0011 Native Code-first UI → 0003N 网络层 → 0013 联机打磨。
  - [ ] 待接入可编译 UE 环境后补编译验证 + 标准牌库 DataTable 内容。

> **每次有实质进展后，负责的 Agent 都应更新本节**（阶段、已完成项、下一步）。这是跨 Agent 状态同步的关键。

---

## 6. 协作约定（所有 Agent 遵守）

1. **先读后做**：开工前按第 4 节顺序读文档。
2. **计划先行**：动手写代码前，应在 `Source/Doc/Plan/` 有对应计划文档（新需求先建计划）。
3. **工程质量优先**：Plan 和实现默认面向真实游戏使用路径，必须考虑健壮性、可扩展性、失败路径与后续接入点；不得用脚手架、空实现或假数据冒充完成。
4. **状态回写**：完成工作后更新第 5 节「当前项目状态」，并把对应 Plan 文档状态改为 `Done`/`In Progress`；已完成且不再作为当前工作面的 Plan 要按 `Source/Doc/Plan/README.md` 移入 `Archive/` 并标记为 `Archived`。
5. **原始需求留档**：用户键入的原始需求记录在 `Source/Doc/Plan/0000-RawRequirements.md`，按时间追加，不要修改历史条目。
6. **不重复 graphify**：代码结构相关交给 graphify，文档里只写「意图/方案/为什么」。
