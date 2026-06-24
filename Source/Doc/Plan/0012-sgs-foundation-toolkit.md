# 0012 — SGS 基础工具库开发计划

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-06-23 |
| 最近更新 | 2026-06-23 |
| 关联需求 | `0000-RawRequirements.md` 中的第 7 条 |
| 关联代码 | `Source/SGS/Core/`、`Source/SGS/Logic/`（未来按里程碑落地） |

---

## 1. 目标

本计划为 SGS 规则层建立一套一次性规划、分阶段落地的基础工具库，避免未来为每个基础工具反复创建独立计划。工具库目标是：

- 足够有用：直接服务 Command、随机审计、目标查询、持续效果、卡牌效果管线与回放。
- 足够健壮：关键工具具备明确边界、稳定标识、不变量检查和可编译验证路径。
- 足够一般：未来应用侧只围绕具体需求做小迭代；只有在抽象能力明显不足时才新开专项工具计划。
- 足够贴合 SGS：不复刻 Boost，不追求通用库完整性；优先建立回合制卡牌规则真正需要的能力。

本计划完成后，后续卡牌、技能、反应、AI、UI 操作提示、联机 RPC 与回放都应能复用同一套工具语言与基础设施。

## 2. 背景与约束

本计划来自对 Boost.Outcome、Boost.MultiIndex、Boost.CircularBuffer、Boost.Bimap、Boost.ICL，以及动作命令系统、效果管线、目标查询、随机审计、回放工具库等方向的讨论。

稳定约束：

- 当前项目已决定**引入 GAS**：启用 `GameplayAbilities` / `GameplayTasks` / `GameplayTags`，但只作为属性、标签、GameplayEffect、GameplayCue / 表现桥接与可复用效果载体底座。
- 使用 UE 容器、UE 类型与 C++17；不引入 Boost。
- 错误返回优先使用 UE 原生 `TValueOrError`，不自研 Result 容器。
- 工具库不进入 UE 反射边界，除非某项工具明确需要与 UPROPERTY/UFUNCTION/RPC 对接。
- 代码结构由 graphify 维护；本文只记录意图、边界和验收，不手写最终类图。
- 本周期不新建 `Source/SGSTests` 测试模块，仅要求 Development Editor 编译验证与运行期不变量诊断。

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

### 3.3 动作命令系统

Command 是服务器权威逻辑的唯一玩家意图入口，位于决策代理、RPC、AI、回放与规则执行之间。

能力边界：

- 表达玩家/AI 意图：出牌、响应、弃牌、选择目标、使用主动技能、确认/取消等。
- 服务器校验合法性后执行，不信任客户端 Command。
- Command 只描述意图，不直接执行卡牌效果。
- Command 产生可回放的输入日志。

设计要求：

- `CommandType` 可以是封闭稳定枚举；具体卡牌、技能、目标实体用 `FName`、RowName、Handle。
- Command 需要稳定 `CommandId`，用于日志、RPC 应答去重、回放定位。
- Validate 和 Execute 分离：Validate 输出 `FSGSStatus`，Execute 进入规则/效果管线。
- `ISGSDecisionAgent` 的返回值应逐步收敛到 Command 或可转换为 Command 的结构。

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

### 3.5 Stable Handle / IndexedStore

为规则实体提供稳定引用和多索引查询底座，避免直接暴露数组下标或维护散落的 `TMap/TMultiMap`。

能力边界：

- Stable Handle 使用 Index + Generation，防止删除后旧引用误命中新对象。
- Store 持有对象，统一 Add / Remove / Modify，外部不得绕过 Store 修改影响索引的 key 字段。
- Unique Index：`Key -> Handle`。
- NonUnique Index：`Key -> Handles`。
- 支持 RebuildIndexes() 与 CheckInvariants()。

第一批服务对象：

- 卡牌实例。
- 座位/角色。
- 未来的持续效果、命令记录、触发器注册项。

不做：

- 不复刻 Boost.MultiIndex 的声明式模板语法。
- 不追求通用 STL 容器兼容。
- 不直接暴露 mutable 迭代器破坏索引一致性。

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
- 第一版内部可以线性扫描 `TArray`，API 稳定优先于算法复杂度。

### 3.8 GAS Attribute / Modifier Adapter

本计划不优先自研通用 Attribute / Modifier 容器，而是建立 SGS 规则到 GAS Attribute / GameplayEffect / GameplayTag 的适配层。

能力边界：

- 表达体力、护甲、手牌上限、攻击范围、距离修正、元素印记层数、技能封禁、额外摸牌数、伤害加减等。
- 优先映射到 GAS Attribute / GameplayEffect Modifier / GameplayTag。
- SGS 自研规则层负责决定何时应用、移除、审计和回放这些效果。
- Modifier 可绑定 ActiveEffect Handle，随持续效果失效自动移除。

只有 GAS 无法表达 SGS 特定离散时序或结算语义时，才在 SGS 侧补最小适配结构。

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

## 4. 任务拆解

### M0：GAS 决策记录

- [x] 对比 GAS 与自研工具在 Attribute、Duration、Effect、Target、Replay、RandomAudit、Command 上的覆盖关系。
- [x] 输出明确结论：引入 GAS，但不外包 SGS 规则核心。
- [x] 同步更新 `Rulers.md`、`ProjectBrief.md`、`SGS.Build.cs`、`SGS.uproject` 与本计划任务边界。

### M1：Core Utility

- [ ] 建立 `FSGSError`、`TSGSResult<T>`、`FSGSStatus`。
- [ ] 建立基础 ID / Handle 类型与统一命名规范。
- [ ] 明确 `check()` / `ensure()` / `FSGSError` 的使用边界。
- [ ] 编译验证。

### M2：Command + RandomAudit

- [ ] 建立 Command 数据结构、CommandId、Validate / Execute 语义。
- [ ] 将 `ISGSDecisionAgent` 返回决策到规则执行之间的路径收敛到 Command。
- [ ] 建立 RandomAudit，替换规则层直接业务随机。
- [ ] 让洗牌、判定预留接入 RandomAudit。
- [ ] 编译验证。

### M3：IndexedStore + TargetQuery

- [ ] 实现 Stable Handle（Index + Generation）。
- [ ] 实现 Store Add / Remove / Modify / RebuildIndexes / CheckInvariants。
- [ ] 实现 Unique / NonUnique 索引能力。
- [ ] 建立第一版 TargetQuery，覆盖存活角色、距离内角色、区域卡牌、可见性/权限上下文。
- [ ] 编译验证。

### M4：Timing + ActiveEffect + Attribute/Modifier

- [ ] 定义 `FSGSTimingPoint` 与排序规则。
- [ ] 定义 DurationSpec，覆盖本回合、本阶段、目标下回合、本轮、N 次事件等常见持续时间。
- [ ] 实现 ActiveEffectTimeline：Add / Expire / QueryActive / CheckInvariants。
- [ ] 实现 SGS-GAS Attribute / GameplayEffect / GameplayTag 适配层，不优先自研通用 Modifier 容器。
- [ ] 编译验证。

### M5：EffectPipeline + ReplayLog

- [ ] 建立 EffectContext / EffectResult / EffectStep / EffectQueue 概念。
- [ ] 将伤害、移牌、属性反应、连锁结算纳入统一管线的设计边界。
- [ ] 建立 CommandLog / RandomLog / EventLog 的最小记录结构。
- [ ] 用现有 GameDriver / GameContext / AutoPass 路径做 smoke 编译验证。
- [ ] 更新 graphify。

### P2 按需小迭代

- [ ] `TSGSBimap`：仅在出现严格一对一运行时映射时实现。
- [ ] `TSGSBoundedHistory`：仅在调试日志/最近操作缓存出现实际需求时封装 UE `TRingBuffer`。
- [ ] 完整 RangeMap / IntervalMap：仅当 ActiveEffectTimeline 的线性结构无法支撑规则需求时专项评估。

## 5. 验收标准

- `0012-sgs-foundation-toolkit.md` 状态为 `In Progress`，并被 `Source/Doc/Plan/README.md` 索引。
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
