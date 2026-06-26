# 0012 — SGS 基础工具库开发计划

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-06-23 |
| 最近更新 | 2026-06-27 |
| 关联需求 | `0000-RawRequirements.md` 中的第 7 条、第 10 条、第 11 条 |
| 关联代码 | `Source/SGS/Core/`、`Source/SGS/Logic/`（未来按里程碑落地） |

---

## 1. 目标

本计划为 SGS 规则层建立一套一次性规划、分阶段落地的基础工具库，避免未来为每个基础工具反复创建独立计划。**本计划的目标是本周期把基础工具库核心形态一步到位，而不是先做“可侵入 / 低侵入第一版”再把关键抽象留给后续大改。**

工具库目标是：

- 足够有用：直接服务 Command、随机审计、目标查询、持续效果、卡牌效果管线与回放。
- 足够健壮：关键工具具备明确边界、稳定标识、不变量检查和可编译验证路径。
- 足够一般：未来应用侧只围绕具体需求做小迭代；只有在抽象能力明显不足时才新开专项工具计划。
- 足够贴合 SGS：不复刻 Boost，不追求通用库完整性；优先建立回合制卡牌规则真正需要的能力。

本计划完成后，后续卡牌、技能、反应、AI、UI 操作提示、联机 RPC 与回放都应能复用同一套工具语言与基础设施。未来允许围绕具体业务需求做小迭代，但不得把本计划 P0/P1 工具的核心形态留成“临时薄封装”或“先扫描数组以后再说”。

## 2. 背景与约束

本计划来自对 Boost.Outcome、Boost.MultiIndex、Boost.CircularBuffer、Boost.Bimap、Boost.ICL，以及动作命令系统、效果管线、目标查询、随机审计、回放工具库等方向的讨论。

稳定约束：

- 当前项目已决定**引入 GAS**：启用 `GameplayAbilities` / `GameplayTasks` / `GameplayTags`，但只作为属性、标签、GameplayEffect、GameplayCue / 表现桥接与可复用效果载体底座。
- 使用 UE 容器、UE 类型与 C++17；不引入 Boost。
- 错误返回优先使用 UE 原生 `TValueOrError`，不自研 Result 容器。
- 工具库不进入 UE 反射边界，除非某项工具明确需要与 UPROPERTY/UFUNCTION/RPC 对接。
- 代码结构由 graphify 维护；本文只记录意图、边界和验收，不手写最终类图。
- 本周期不新建 `Source/SGSTests` 测试模块，仅要求 Development Editor 编译验证与运行期不变量诊断。
- P0/P1 工具的完成标准是“后续系统可以直接依赖”，不是“先有一个低侵入过渡版”。若当前实现只是过渡原语，必须在任务状态中明确标记为未完成 / 需重构。
- “一步到位”适用于 M1-M5 全部里程碑：M1 的错误/ID、M2 的 Command/RandomAudit、M3 的 Store/TargetQuery、M4 的 Timing/ActiveEffect/GAS Adapter、M5 的 EffectPipeline/ReplayLog 都必须形成稳定核心形态。

### 2.1 与 GAS 的关系

M0 已完成：用户明确要求引入 GAS。本计划后续按 SGS-GAS 混合路线执行。

已同步更新范围：

- `Source/Doc/Rulers.md` 硬约束 #1。
- `ProjectBrief.md` 技术栈与当前状态。
- `Source/SGS/SGS.Build.cs` 模块依赖。
- `SGS.uproject` 插件配置。
- 本计划 M4/M5 中 Attribute、持续效果、标签条件、效果载体的任务边界。

GAS 最可能替代或降低优先级的部分：

- 轻量 Attribute / Modifier 系统。
- 一部分 Duration / Stacking / ActiveEffect。
- 一部分标签条件判断。
- 一部分表现事件桥接。

GAS 不替代的部分：

- 动作命令系统。
- 随机审计系统。
- 卡牌/牌区/座位模型。
- TargetQuery 的 SGS 规则语义。
- IndexedStore / 稳定实体句柄。
- CommandLog / RandomLog / EventLog 回放地基。

## 3. 方案设计

### 3.1 优先级总览

| 优先级 | 工具 | 本计划处理方式 |
|---|---|---|
| P0 | `FSGSError` / `TSGSResult<T>` / `FSGSStatus` | 必做，作为所有后续工具的错误返回基础 |
| P0 | 动作命令系统 | 必做，统一真人、AI、RPC、回放的玩家意图 |
| P0 | 随机审计系统 | 必做，统一所有业务随机入口 |
| P1 | Stable Handle / IndexedStore | 必做，支撑目标查询、多索引与稳定引用 |
| P1 | TargetQuery | 必做，服务规则校验、AI、UI 提示 |
| P1 | Timing / Duration / ActiveEffectTimeline | 必做，负责 SGS 离散时序；持续效果载体优先适配 GAS GameplayEffect |
| P1 | EffectPipeline | 必做，统一卡牌效果、伤害、反应、连锁 |
| P1 | ReplayLog 地基 | 必做，只记录三类日志，不做播放器 |
| P2 | Bimap | 按需小迭代，严格一对一关系时再做 |
| P2 | BoundedHistory / CircularBuffer 薄封装 | 按需小迭代，优先用 UE `TRingBuffer` |
| P2 | 完整 RangeMap / IntervalMap | 按需专项，先用业务 Timeline |

### 3.2 Core Utility

使用 UE 原生 `TValueOrError` 建立项目别名：

- `FSGSError`：统一错误结构，至少包含错误码、面向开发者的上下文、可选面向 UI/日志的消息。
- `TSGSResult<T>`：`TValueOrError<T, FSGSError>` 的项目别名。
- `FSGSStatus`：`TValueOrError<void, FSGSError>` 的项目别名。

设计要求：

- 错误码用 `FName`，避免用可扩展内容枚举。
- 错误结构不绑定 UI，不直接承载本地化策略。
- 工具层失败优先返回 `FSGSError`；不可恢复不变量用 `check()`；可继续但需诊断用 `ensure()`。

M1 完成形态：

- 所有 P0/P1 工具公开失败路径统一使用 `FSGSError` / `TSGSResult<T>` / `FSGSStatus`，不得各自返回散落的 `bool + OutError` 或裸 `FString`。
- `FSGSError` 的 `Code` 命名必须遵循 `SGS.<Domain>.<Reason>`，Context 承载开发者诊断信息，Message 仅作为可选显示文本。
- ID / Handle 类型必须具备统一 invalid 值、相等比较、hash、日志可读形式和 `CheckInvariants()` 可用语义；不得在跨工具边界继续裸传临时数组下标。
- M1 可保持小实现，但不是临时版：后续 Command、RandomAudit、IndexedStore、EffectPipeline 不应再重新定义另一套 Result / Error / Handle。

### 3.3 动作命令系统

Command 是服务器权威逻辑的唯一玩家意图入口，位于决策代理、RPC、AI、回放与规则执行之间。

能力边界：

- 表达玩家/AI 意图：出牌、响应、弃牌、选择目标、使用主动技能、确认/取消等。
- 服务器校验合法性后执行，不信任客户端 Command。
- Command 只描述意图，不直接执行卡牌效果。
- Command 产生可回放的输入日志。

设计要求：

- `CommandType` 使用 GameplayTag / 开放 ID；具体卡牌、技能、目标实体用 `FName`、RowName、Handle。
- Command 需要稳定 `CommandId`，用于日志、RPC 应答去重、回放定位。
- Validate 和 Execute 分离：Validate 输出 `FSGSStatus`，Execute 进入规则/效果管线。
- `ISGSDecisionAgent` 的返回值应逐步收敛到 Command 或可转换为 Command 的结构。

M2 Command 完成形态：

- Command 是所有玩家/AI/RPC/回放输入的唯一入口，不只是出牌阶段 Pass 的临时结构。
- Command 数据模型必须覆盖动作类型、请求/响应配对、发起座位、阶段/时机、卡牌 Handle、目标 Handle、技能/牌名来源、附加参数与来源通道。
- Command Validate / Execute 不得散落在 `USGSGameDriver` 的单个回调里；必须有规则层统一的 Command Router / Registry / Handler 入口。
- Command 生命周期必须可记录：Created、Received、Validated、Rejected、Executed、Fallbacked；失败使用 `FSGSError`。
- CommandLog 必须记录原始输入、服务器校验结果、fallback 决策与执行顺序，能够作为 M5 ReplayLog 的输入源。

### 3.4 随机审计系统

所有业务随机必须通过统一 RandomAudit，禁止在规则层散落直接 `Rng.RandRange()`。

能力边界：

- 封装 `FRandomStream`，保留可复现 seed。
- 每次随机记录 step、purpose/channel、输入范围、输出值、业务上下文。
- 支持洗牌、判定、随机目标、随机牌、AI 随机 tie-break。
- 日志进入 ReplayLog 的 RandomLog。

设计要求：

- `purpose` 用 `FName` 或轻量文本标识，不用内容枚举。
- 业务调用方必须传入足够定位问题的上下文。
- 随机入口返回值必须可被审计日志完整重放。
- RandomAudit 是规则层唯一随机源；除 RandomAudit 内部外，规则层不得持有或调用 `FRandomStream`。
- RandomAudit 需要支持生成模式与校验/重放模式：重放时按 step、purpose、range 校验输入并返回日志输出。
- RandomLog 条目必须稳定承载 seed、step、purpose、range、output、context 和关联 Command/Event 信息。
- 洗牌、判定、随机目标、随机牌、AI 随机 tie-break 都必须接入同一入口；尚未实现的业务路径也要预留明确调用点。

### 3.5 Stable Handle / IndexedStore

为规则实体提供稳定引用和多索引查询底座，避免直接暴露数组下标或维护散落的 `TMap/TMultiMap`。SGS 卡牌世界天然需要按拥有者、区域、牌堆、顺序、点数、花色、牌名、牌类、状态与组合条件查询，因此本计划需要的是**SGS 版内建多索引 Store**，而不是外围轻量索引工具。

M3 完成形态：

- Stable Handle 使用 Index + Generation，防止删除后旧引用误命中新对象。
- Store 持有对象，是实体生命期、索引一致性和查询入口的事实源。
- Store 内建 IndexRegistry：索引注册到 Store，Add / Remove / Modify / Move 等操作自动维护索引。
- Store 支持 Unique、NonUnique、Ordered/Sequenced 语义；顺序语义服务牌堆顶/底/第 N 张等需求。
- Store 支持显式 RebuildIndexes() 与 CheckInvariants()，用于加载、调试和兜底修复。
- 外部不得绕过 Store 修改影响索引 key 的字段；所有会改变索引 key 的业务操作必须通过 Store/Context 的受控 API。

卡牌 Store 必须覆盖的首批索引：

- `CardId -> Handle` 唯一索引。
- `OwnerSeat -> Handles` 非唯一索引。
- `Zone -> Handles` 非唯一索引。
- `PileId / ZoneOwner -> Ordered Handles`，表达牌堆、手牌、判定区、弃牌堆等区域内顺序。
- `CardName -> Handles`、`CardType -> Handles`、`Suit -> Handles`、`Number -> Handles`。
- 高频组合索引至少包含 `OwnerSeat + Zone`、`Zone + CardName`、`Zone + CardType`；更复杂条件由最窄索引候选集再过滤。

第一批服务对象：

- 卡牌实例：必须进入多索引 Store，不得只保留 `AllCards + Pile` 后外围扫描。
- 座位/角色：可先用 Store 表达 seat handle 与存活/身份/阵营索引。
- 未来的持续效果、命令记录、触发器注册项。

不做：

- 不复刻 Boost.MultiIndex 的完整声明式模板语法。
- 不追求通用 STL 容器兼容。
- 不直接暴露 mutable 迭代器破坏索引一致性。
- 不为所有组合条件都建索引；只内建高频 / 事实源索引，低频组合通过候选集过滤完成。

### 3.6 TargetQuery

目标查询服务规则校验、AI 决策和 UI 提示，避免目标选择逻辑散落在卡牌效果、AI 和 UI 里。

能力边界：

- 查询存活角色、距离范围内角色、拥有特定状态/印记/效果的角色。
- 查询指定区域的卡牌、可见卡牌、可操作卡牌。
- 支持组合条件：座位、阵营/身份、距离、区域、牌类型、状态、效果。
- 输出稳定 Handle 或只读视图，不直接暴露内部容器。

设计要求：

- TargetQuery 只回答“候选是谁”和“为什么合法/不合法”，不执行效果。
- 同一个查询 API 同时服务 UI 可选提示、AI 搜索、服务器 Validate。
- 私密信息查询必须由调用方传入视角/权限上下文。
- TargetQuery 必须建立在 IndexedStore 的索引入口上；不得以扫描 `AllCards`、遍历所有座位、手写临时过滤作为 M3 完成形态。可在索引候选集上做最终业务过滤与拒绝原因收集。

### 3.7 Timing / Duration / ActiveEffectTimeline

SGS 时序不是 wallclock，而是离散的回合、座位、阶段、时机点。持续效果需要基于离散时序表达生命周期。

能力边界：

- `FSGSTimingPoint`：可排序的离散时序点。
- DurationSpec：表达“直到本回合结束”“直到目标下个回合结束后”“本阶段内”“本轮内”“接下来 N 次事件”等业务持续时间。
- ActiveEffectTimeline：添加、过期、查询当前有效效果。
- 可用 UE `TRange` / `TRangeSet` 表达简单连续区间；不复刻 Boost.ICL 完整区间映射。

设计要求：

- 阶段 Begin / End / After 的排序必须明确。
- 持续效果必须有来源、拥有者、优先级、叠加策略和失效条件。
- 内部算法可以先使用线性结构，但公开模型必须稳定，不能把 Duration / Timing / Effect 生命周期留成后续大改。
- Timing 必须适配开放阶段/回合模型：插入阶段、跳过阶段、额外回合、他人回合中插入自己的阶段都不能依赖封闭 enum。
- ActiveEffect 必须以 Stable Handle 管理来源、拥有者、目标、创建时机、持续时间、叠加策略、优先级、失效原因与审计事件。
- ActiveEffectTimeline 必须提供 Add / Expire / QueryActive / QueryByOwner / QueryByTag / CheckInvariants，并为 M5 EffectPipeline 提供统一效果生命周期入口。

### 3.8 GAS Attribute / Modifier Adapter

本计划不优先自研通用 Attribute / Modifier 容器，而是建立 SGS 规则到 GAS Attribute / GameplayEffect / GameplayTag 的适配层。

能力边界：

- 表达体力、护甲、手牌上限、攻击范围、距离修正、元素印记层数、技能封禁、额外摸牌数、伤害加减等。
- 优先映射到 GAS Attribute / GameplayEffect Modifier / GameplayTag。
- SGS 自研规则层负责决定何时应用、移除、审计和回放这些效果。
- Modifier 可绑定 ActiveEffect Handle，随持续效果失效自动移除。

只有 GAS 无法表达 SGS 特定离散时序或结算语义时，才在 SGS 侧补最小适配结构。

M4 完成形态：

- `FSGSTimingPoint` 是 SGS 离散结算顺序的稳定排序键，不依赖 wallclock，不依赖封闭阶段 enum。
- DurationSpec 覆盖本回合、本阶段、目标下回合、本轮、N 次事件、直到指定 TimingPoint、直到主动失效等常见持续时间。
- GAS Adapter 明确 SGS ActiveEffect Handle 与 GAS GameplayEffect / GameplayTag / Attribute 修改之间的所有权和失效关系。
- Attribute / Modifier 不再另起自研通用容器；若必须补 SGS 侧结构，必须证明是 GAS 难以表达的离散规则语义。

### 3.9 EffectPipeline

EffectPipeline 统一执行卡牌效果、伤害、属性反应、连锁反应和持续效果触发。

能力边界：

- EffectContext：发起者、来源 Command、目标、相关卡牌/技能、随机入口、日志入口。
- EffectResult：成功、失败、产生的事件、后续步骤、是否等待决策。
- EffectQueue / EffectStep：支持同步步骤、异步决策挂起、恢复与连锁插入。
- 反应体系支持优先级、后发先致、残留印记、连锁反应。

设计要求：

- 效果只通过 Context 访问规则世界，避免直接绕过 Context 改状态。
- 会触发随机的效果必须使用 RandomAudit。
- 会改变可回放状态的效果必须产出 EventLog。
- 效果管线不依赖 UI。
- EffectPipeline 必须是卡牌效果、伤害、濒死/求桃、判定、反应窗口、连锁和持续效果触发的统一执行入口。
- EffectStep 必须支持同步执行、异步决策挂起、恢复、插队/连锁、失败回滚边界和审计日志。
- EffectContext 必须携带 Command、RandomAudit、TargetQuery、ActiveEffect、ReplayLog 写入入口，效果不得绕过 Context 直接修改规则世界。
- 反应体系必须基于开放 Timing / Tag / Priority，不得把“杀闪桃”等具体牌写成管线分支。

### 3.10 ReplayLog 地基

本周期只建立回放所需数据源，不做播放器、时间轴 UI、断点调试器。

三类日志：

- CommandLog：玩家/AI/RPC/回放输入的 Command。
- RandomLog：所有随机审计记录。
- EventLog：规则系统产生的事实事件，如移牌、伤害、回复、阶段切换、效果开始/结束。

设计要求：

- 日志结构服务复现、debug、公平性检查。
- EventLog 是事实记录，不是 UI 动画脚本。
- ReplayLog 不替代服务器权威状态，只用于重放与诊断。
- CommandLog / RandomLog / EventLog 必须有稳定关联键：CommandId、EffectStepId 或 TimingPoint，能够还原输入、随机和事实事件之间的因果关系。
- ReplayLog 地基必须支持确定性重放校验：Command 顺序一致、RandomAudit step 一致、EventLog 事实一致。
- 本周期不做播放器 UI，但必须建立后续播放器/调试器可以消费的数据模型，不能只散落在各系统数组里。

M5 完成形态：

- EffectPipeline 成为规则状态变化的统一入口；移牌、伤害、回复、判定、持续效果触发都通过 EffectStep / Context 产生日志。
- ReplayLog 聚合 CommandLog、RandomLog、EventLog，能按一局游戏导出 / 查询，且不依赖 UI。
- 现有 GameDriver / GameContext / AutoPass smoke 路径通过 EffectPipeline + ReplayLog 地基跑通，而不是保留旁路执行。

## 4. 任务拆解

### M0：GAS 决策记录

- [x] 对比 GAS 与自研工具在 Attribute、Duration、Effect、Target、Replay、RandomAudit、Command 上的覆盖关系。
- [x] 输出明确结论：引入 GAS，但不外包 SGS 规则核心。
- [x] 同步更新 `Rulers.md`、`ProjectBrief.md`、`SGS.Build.cs`、`SGS.uproject` 与本计划任务边界。

### M1：Core Utility

- [x] 建立 `FSGSError`、`TSGSResult<T>`、`FSGSStatus`。
- [x] 建立基础 ID / Handle 类型与统一命名规范。
- [x] 明确 `check()` / `ensure()` / `FSGSError` 的使用边界。
- [x] 按一步到位标准复核所有 P0/P1 工具公共边界，确保没有平行的错误/ID/Handle 体系。
- [x] 编译验证。

### M2：Command + RandomAudit

- [x] 建立 Command 数据结构、CommandId 基础原语。
- [x] 将 `ISGSDecisionAgent` 出牌阶段 Pass 应答收敛到 Command。
- [x] 建立统一 Command Router / Registry / Handler，Validate / Execute 不得散落在单个 Driver 回调中。
- [x] Command 生命周期日志覆盖 Created / Received / Validated / Rejected / Executed / Fallbacked。
- [x] 建立 RandomAudit 基础原语，替换现有洗牌直接业务随机。
- [x] RandomAudit 支持生成模式与校验/重放模式，RandomLog 条目关联 Command/Event。
- [x] 洗牌、判定、随机目标、随机牌、AI 随机 tie-break 全部接入 RandomAudit 或预留明确入口。
- [x] 编译验证。

### M3：IndexedStore + TargetQuery

- [x] 实现 Stable Handle（Index + Generation）。
- [x] 实现 Store Add / Remove / Modify / RebuildIndexes / CheckInvariants 的基础原语。
- [x] 将 Store 升级为内建 IndexRegistry 的多索引容器，索引随 Add / Remove / Modify / Move 自动维护。
- [x] 建立卡牌 Store 事实源，覆盖 CardId、OwnerSeat、Zone、Pile/ZoneOwner 顺序、CardName、CardType、Suit、Number 与高频组合索引。
- [x] 将座位/角色纳入 Store 或等价稳定索引入口，覆盖存活、身份/阵营、座次查询。
- [x] 迁移 `USGSGameContext` 的卡牌位置、牌堆顺序、移牌/摸牌/弃牌原语，使其通过 Store 维护索引一致性。
- [x] TargetQuery 基于 IndexedStore 候选集查询，覆盖存活角色、距离内角色、区域卡牌、可见性/权限上下文；不得以扫描数组作为完成形态。
- [x] 编译验证。

### M4：Timing + ActiveEffect + Attribute/Modifier

- [x] 定义 `FSGSTimingPoint` 与稳定排序规则，支持开放阶段、插入阶段、额外回合和反应窗口。
- [x] 定义 DurationSpec，覆盖本回合、本阶段、目标下回合、本轮、N 次事件、指定 TimingPoint、主动失效等常见持续时间。
- [x] 实现 ActiveEffectTimeline：Add / Expire / QueryActive / QueryByOwner / QueryByTag / CheckInvariants。
- [x] 建立 ActiveEffect Handle、来源、拥有者、目标、优先级、叠加策略、失效原因与审计事件。
- [x] 实现 SGS-GAS Attribute / GameplayEffect / GameplayTag 适配层，明确 ActiveEffect 与 GAS Effect 的所有权和失效关系。
- [x] 编译验证。

### M5：EffectPipeline + ReplayLog

- [x] 建立 EffectContext / EffectResult / EffectStep / EffectQueue，并支持同步、异步挂起、恢复、插队/连锁和失败边界。
- [x] 将移牌、伤害、回复、判定、反应窗口、持续效果触发纳入统一管线，不保留规则状态变化旁路。
- [x] 建立 ReplayLog 聚合器，统一 CommandLog / RandomLog / EventLog，并用 CommandId / EffectStepId / TimingPoint 建立因果关联。
- [x] 支持确定性重放校验：Command 顺序、RandomAudit step、EventLog 事实一致。
- [x] 用现有 GameDriver / GameContext / AutoPass 路径通过 EffectPipeline + ReplayLog 地基做 smoke 编译验证。
- [x] 更新 graphify。

### P2 按需小迭代

- [ ] `TSGSBimap`：仅在出现严格一对一运行时映射时实现。
- [ ] `TSGSBoundedHistory`：仅在调试日志/最近操作缓存出现实际需求时封装 UE `TRingBuffer`。
- [ ] 完整 RangeMap / IntervalMap：仅当 ActiveEffectTimeline 的线性结构无法支撑规则需求时专项评估。

## 5. 验收标准

- `0012-sgs-foundation-toolkit.md` 已完成并归档，状态为 `Archived`，并被 `Source/Doc/Plan/README.md` 的归档索引登记。
- `0000-RawRequirements.md` 顶部追加第 7 条原始需求记录。
- 工具库实现阶段完成 M0-M5 后，所有规则层随机、玩家意图、目标查询、持续效果、效果执行和回放地基都有统一入口。
- 不引入 Boost。
- 不新建 `Source/SGSTests`；每个里程碑至少通过一次 Development Editor 编译。
- 每个可维护索引/时间线/效果容器都提供 `CheckInvariants()` 或等价诊断能力。
- 修改代码后运行 `graphify update . --no-cluster`；提交前运行 `Tools/CheckLargeFiles.ps1 -Scope Staged`。

## 6. 进度与决策记录

- 2026-06-23：根据工具库路线讨论创建本计划。范围锁定 P0+P1；P2 工具只作为后续按需小迭代。
- 2026-06-23：计划创建阶段只写文档，不实现工具代码。
- 2026-06-23：测试策略定为仅编译验证，不新建 `Source/SGSTests`。
- 2026-06-23：用户明确要求引入 GAS。M0 决策完成：启用 `GameplayAbilities` / `GameplayTasks` / `GameplayTags`，但 SGS Command、随机审计、回放日志、牌区移动、响应窗口和三国杀式结算顺序仍由自研规则管线控制。
- 2026-06-23：根据“默认避免封闭枚举建模规则概念”约束，现有花色、牌色、牌类、阶段、势力、装备槽、牌区、游戏事件、出牌动作由 `enum class` 迁移为 `FGameplayTag` / SGS 内置 Native GameplayTags；标准标签只是默认基线，不代表规则全集。
- 2026-06-24：完成 M1-M2 纵切：新增错误/结果别名、CommandId、StableHandle、Command、RandomAudit；`ISGSDecisionAgent` 出牌阶段应答收敛为 Command，`USGSGameDriver` 增加 Command Validate / Execute 与 fallback Pass，`USGSGameContext` 洗牌随机接入 RandomAudit。M3-M5 尚未开始。
- 2026-06-24：M3 状态纠偏：当前 `TSGSIndexedStore`、`TSGSUniqueIndex`、`TSGSNonUniqueIndex` 与 `USGSGameContext` 第一版 TargetQuery 只能算基础原语 / 过渡实现，不满足 Plan 0012 “一步到位”的完成标准。M3 必须重构为 SGS 版内建多索引 Store：卡牌 Store 成为事实源，索引随实体修改自动维护，TargetQuery 基于 Store 索引候选集而不是扫描数组。
- 2026-06-24：按一步到位标准补齐 M1 Core Utility：`FSGSError` 增加错误码格式校验、日志字符串与不变量检查，错误码统一为 `SGS.<Domain>.<Reason>`；`FSGSCommandId` / `FSGSStableHandle` 增加统一 invalid、日志字符串、hash 与 `CheckInvariants()` 语义；`TSGSIndexedStore` 的 `Remove` / `Modify` / `RebuildIndexes` 公开失败路径改为 `FSGSStatus`，避免 P1 工具边界继续使用裸 `bool` 表达错误。
- 2026-06-25：按一步到位标准补齐 M2：新增 `FSGSCommandRouter` / `ISGSCommandHandler` / Pass handler，Command Validate / Execute 从 `USGSGameDriver` 迁入统一 Router；`FSGSCommandLogEntry` 记录 Created / Received / Validated / Rejected / Executed / Fallbacked 生命周期；`FSGSCommand` 扩展 Handle、来源通道、技能名和参数字段；`FSGSRandomAudit` 增加生成 / 重放校验模式、`TryRandRange` / `TryRandIndex`、CommandId / EventName 关联字段；现有洗牌和弃牌洗回随机入口记录 RandomEvent。Development Editor 编译通过。
- 2026-06-25：按一步到位标准补齐 M3：`TSGSIndexedStore` 升级为内建 IndexRegistry，Store 持有 Unique / NonUnique / Ordered 索引并在 Add / Remove / Modify 后自动重建；`USGSGameContext` 新增 `FSGSCardState` 卡牌事实源，覆盖 CardId、OwnerSeat、Zone、Pile/ZoneOwner 顺序、CardName、CardType、Suit、Number、Owner+Zone、Zone+CardName、Zone+CardType 索引；摸牌、弃牌、洗牌、弃牌堆洗回和通用移牌改为通过 Store 改状态，再同步只读牌堆视图；座位查询使用 All / Alive / Faction 等价索引；`QueryCards` 基于 Store 索引候选集和可见性上下文过滤。Development Editor 编译通过。
- 2026-06-25：实现 M4 Timing + ActiveEffect + GAS Adapter：新增开放式 `FSGSTimingPoint`、`FSGSDurationSpec`，支持本回合、本阶段、目标下回合、本轮、N 次事件、指定 TimingPoint、主动失效；新增 `FSGSActiveEffectTimeline`，以 Stable Handle 管理 ActiveEffect，支持 Add / Expire / QueryActive / QueryByOwner / QueryByTarget / QueryByTag / CheckInvariants；新增 `FSGSGASActiveEffectAdapter`，记录 SGS ActiveEffect Handle 与 GAS GameplayEffect / GrantedTags / Attribute 修改的所有权绑定。Development Editor 编译通过。
- 2026-06-25：完成 M5 EffectPipeline + ReplayLog：新增 `FSGSEffectPipeline`、`FSGSEffectContext`、`FSGSEffectStep`、`FSGSEffectQueue` 与 `FSGSEffectResult`，支持同步执行、挂起/恢复、插队 follow-up step、失败边界和 EventLog 审计；新增 `FSGSReplayLog` 聚合 CommandLog / RandomLog / EventLog，并用 CommandId / EffectStepId / TimingPoint 建立因果关联；新增 `SGSStandardEffectSteps`，将 MoveCards、DrawCards、Damage、Heal、Judgement placeholder、ReactionWindow suspend、ActiveEffect expire 作为标准 EffectStep；`USGSGameDriver` 起始手牌与摸牌阶段 smoke 路径改为通过 EffectPipeline 执行并同步 ReplayLog。Development Editor 编译通过，graphify 已更新。
- 2026-06-27：删除旧牌堆 UObject：`USGSCardPile` 不再存在，`USGSSeat` 不再持有 Hand / JudgementZone 牌堆对象，`USGSGameContext` 不再持有 DrawPile / DiscardPile 视图；牌区完全由 CardStore / `CardsByPile` 表达，对外读取通过 Store 查询 API，规则层牌区变化只能通过 `USGSGameContext` 的 Store 原语、RandomAudit 与 EffectPipeline 进入。
- 2026-06-27：计划已完成并归档到 `Source/Doc/Plan/Archive/`；状态由 `Done` 调整为 `Archived`，后续只作为历史事实与约束来源，不再作为当前工作面。
