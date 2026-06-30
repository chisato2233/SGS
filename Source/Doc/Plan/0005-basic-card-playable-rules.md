# 0005 — 基础牌可玩规则纵切

| 字段 | 值 |
|---|---|
| 状态 | `Done` |
| 创建日期 | 2026-06-28 |
| 最近更新 | 2026-06-28 |
| 关联需求 | `0000-RawRequirements.md` 中的第 12 条 |
| 关联代码 | `Source/SGS/Logic/{Cards,Commands,Effects,Engine,Players,Queries}/`、`Source/SGS/AI/`、`Source/SGS/Tests/` |

---

## 1. 目标

实现 SGS 第一条真正可玩的服务器权威规则纵切：**杀 / 闪 / 桃 + 濒死 / 求桃**。

做完后，即使还没有 UI，也必须能通过自动化测试或命令行 smoke 证明以下真实游戏路径成立：

1. 出牌阶段，行动座位使用一张 `Slash` 指定合法目标。
2. 目标进入响应窗口，可以用 `Dodge` 响应；未响应则受到 1 点伤害。
3. 角色体力降至 0 时进入濒死 / 求桃流程；可由自己或其他合法座位使用 `Peach` 救回。
4. 所有玩家 / AI / 未来 RPC 输入都以 `FSGSCommand` 进入 `FSGSCommandRouter`，效果通过 `FSGSEffectPipeline` 执行并写入 `FSGSReplayLog`。

本计划的完成标准不是“牌名存在”或“类已创建”，而是基础牌能在服务器权威规则闭环中改变真实对局状态。

---

## 2. 背景与约束

当前项目已经完成：

- `0003`：权威对局骨架，能推进阶段并向决策代理请求出牌阶段动作。
- `0004`：对局数据模型，包含牌、牌区、座位、摸牌、弃牌、伤害、回复、距离、查询原语。
- `0012`：Command、RandomAudit、IndexedStore / TargetQuery、Timing / ActiveEffect、EffectPipeline、ReplayLog 地基。

当前仍不可玩，因为出牌阶段只有 `Pass`，没有真实用牌、目标选择、响应窗口、濒死求桃和胜负推进。本计划补上最小但真实的一组核心规则。

约束：

- 不实现完整标准包、锦囊、装备、武将技、网络复制、正式 UI。
- 不为 `Slash` / `Dodge` / `Peach` 写死封闭枚举；牌名使用 `FName` / RowName，动作与窗口使用 GameplayTag / 开放标识。
- 不绕过 `CommandRouter`、`EffectPipeline`、`GameContext`、`ReplayLog` 直接改状态。
- 不把测试专用牌库塞进正式对局路径。自动化测试可以构造测试牌，但运行时牌库入口必须是可替换的真实数据源 / 牌库定义。

---

## 3. 方案设计

### 3.1 数据入口

建立最小基础牌数据定义，覆盖：

- `Slash`：基本牌，出牌阶段使用，目标为距离内一名其他存活角色。
- `Dodge`：基本牌，只能在 `Slash` 响应窗口中作为响应使用。
- `Peach`：基本牌，出牌阶段可治疗自己；濒死 / 求桃窗口中可治疗濒死角色。

牌库不应写成 `GameDriver` 里的硬编码数组。可接受的长期入口：

- 以 `FSGSCardDef` 为卡牌定义来源。
- 新增最小牌库定义结构，例如 `FSGSDeckEntry` / `USGSDeckDefinition` / DataTable 行，用来描述牌名、花色、点数和数量。
- 自动化测试可以用测试 helper 生成确定性牌库，但 helper 只能位于测试路径或明确隔离的开发辅助路径。

### 3.2 Command 与动作类型

在现有 `Pass` 基础上，补齐最小动作：

- `SGS.PlayAction.UseCard`：出牌阶段主动使用手牌。
- `SGS.PlayAction.RespondCard`：在响应 / 求桃窗口中打出或使用一张牌。
- `SGS.PlayAction.Pass`：继续保留，表示放弃当前决策。

`FSGSCommand` 已具备 `CommandId`、`RequestId`、`SeatIndex`、`CardIds` / `CardHandles`、`TargetSeatIndices` / `TargetHandles`、`SourceChannel` 等字段。本计划优先复用它，不另造平行输入结构。若发现字段不足，只做面向长期的最小扩展，并补齐不变量与日志字符串。

### 3.3 决策请求

现有 `ISGSDecisionAgent::RequestPlayPhaseAction` 只覆盖出牌阶段。需要把决策抽象扩展到最小可玩规则需要的两个场景：

- 出牌阶段动作请求：可用牌、合法目标、是否允许 Pass。
- 响应窗口请求：窗口名、响应原因、目标牌 / 效果、可响应牌条件、超时 / 放弃语义。

可选实现方式：

- 在现有接口旁新增 `RequestResponseAction` / `RequestDyingPeachAction`。
- 或引入统一 `FSGSDecisionRequest` / `FSGSDecisionResult`，再让出牌阶段和响应窗口成为不同请求类型。

无论采用哪种方式，规则层只面向 `ISGSDecisionAgent`；AI、未来 UI、未来 RPC 都走同一决策边界。

### 3.4 规则服务与效果管线

新增最小基础牌规则服务，职责是把 Command 解析成规则效果，而不是直接修改世界：

- 校验 `Slash`：手牌归属、出牌阶段、目标存活、目标不是自己、距离合法、当前阶段是否允许使用。
- 处理 `Slash`：移动 / 标记正在结算的牌，打开目标 `Dodge` 响应窗口，未响应则排入伤害效果。
- 校验 `Dodge`：只能在对应响应窗口中由目标打出，且响应的牌名 / 类型匹配。
- 处理 `Peach`：治疗自己或濒死目标，不超过体力上限。
- 濒死流程：伤害导致体力为 0 时，不立即死亡；按座位顺序请求桃，全部放弃后再进入死亡 / 出局。

效果执行必须进入 `FSGSEffectPipeline`。本计划允许补强现有 `ReactionWindow` placeholder，使其能真正挂起、恢复并记录 EventLog。

### 3.5 AI 与测试代理

`USGSAutoPassAgent` 保留为 smoke 兜底，但本计划需要新增可测试代理：

- Scripted agent：按测试脚本返回指定 Command，用于覆盖 Slash / Dodge / Peach / Pass 路径。
- Basic AI agent：在 smoke 中可用简单策略，例如有 `Slash` 且有合法目标则出杀；有 `Dodge` 则闪；濒死有 `Peach` 则救。

AI 策略可以很朴素，但必须走真实 `ISGSDecisionAgent` 和 `FSGSCommand`，不能直接调用 `GameContext`。

---

## 4. 工程质量门

- **真实使用入口**：`USGSGameDriver` 的出牌阶段、响应窗口和濒死流程会调用本计划能力；自动化测试和后续 UI 都复用同一入口。
- **既有架构接入**：玩家意图进入 `FSGSCommandRouter`；状态改变走 `USGSGameContext` 原语与 `FSGSEffectPipeline`；随机走 `FSGSRandomAudit`；事件和结果写入 `FSGSReplayLog`。
- **关键不变量**：
  - 非当前请求 / 过期 `RequestId` / 错误 `CommandId` 的应答必须拒绝或忽略。
  - 客户端 / UI / AI 声称的目标合法性不可信，服务器必须重新校验。
  - 牌只能从其当前真实区域移动，不能重复使用、跨座位偷用或凭空响应。
  - 私密手牌查询必须遵守 viewer 权限；测试不得通过破坏可见性获得合法性。
  - 濒死流程未完成前不得提前判定死亡或继续后续阶段。
- **失败路径**：
  - 非法用牌：记录 Rejected，不改变规则状态，可按场景 fallback 为 Pass。
  - 响应窗口超时 / 放弃：记录 Pass，继续结算原效果。
  - 牌库为空：不崩溃；摸牌原语保持当前 0004 语义。
  - 濒死无人救：进入死亡 / 出局占位流程，并在 Plan 中记录后续胜负规则落点。
- **未来扩展**：
  - `Slash`、`Dodge`、`Peach` 不作为特殊分支扩散到 Driver；后续锦囊、装备、武将技应接入同一用牌 / 响应 / 效果管线。
  - 本计划可以添加最小死亡 / 出局状态，但完整胜负、奖惩、武将技能触发进入后续计划。
- **不可接受的实现**：
  - 只在 UI 或测试里模拟掉血 / 回血。
  - 在 `GameDriver` 中按牌名写一大段不可复用流程。
  - 为了演示直接调用 `ApplyDamage` / `Heal`，绕过 Command 与 Pipeline。
  - 创建空类、空函数、假 DataTable 或只打印日志就标记完成。

---

## 5. 任务拆解

- [x] 补充基础牌与动作 GameplayTag / RowName 约定，确保使用开放标识而非封闭枚举。
- [x] 建立最小基础牌牌库入口：正式路径使用可替换牌库定义，测试路径使用隔离 helper。
- [x] 扩展决策请求模型，覆盖出牌阶段、`Dodge` 响应窗口、濒死 / 求桃窗口。
- [x] 扩展 `FSGSCommandRouter`：注册 `UseCard` / `RespondCard` handler，补齐通用校验和生命周期日志。
- [x] 实现 `Slash` 规则：目标查询、距离校验、用牌消耗、响应窗口、未闪伤害。
- [x] 实现 `Dodge` 响应：只在对应窗口合法，响应成功后阻止本次 `Slash` 伤害。
- [x] 实现 `Peach`：出牌阶段治疗自己；濒死 / 求桃窗口治疗目标。
- [x] 实现最小死亡 / 出局状态与事件：濒死无人救后角色不可继续行动；完整胜负留后续。
- [x] 让 `USGSGameDriver` 的出牌阶段使用真实可用动作请求，而不是只等 Pass。
- [x] 新增 scripted agent 和 basic AI agent，覆盖自动化与 smoke。
- [x] 新增自动化测试，覆盖成功、失败、边界和回放 / 随机稳定性。
- [x] 更新相关 Plan / ProjectBrief 状态与 graphify。

---

## 6. 验收标准

必须全部满足：

- 自动化测试 `SGS.Plan0005.BasicCards.SlashDodgeDamage` 通过：A 对 B 使用 `Slash`，B 有 `Dodge` 时可响应并避免伤害；B 放弃时受到 1 点伤害。
- 自动化测试 `SGS.Plan0005.BasicCards.PeachAndDying` 通过：伤害使角色进入濒死，合法座位使用 `Peach` 后目标恢复到 1 点体力；无人出桃时角色进入死亡 / 出局状态。
- 自动化测试覆盖非法命令：错误座位、错误 RequestId、使用不在手牌的牌、目标非法、响应窗口外打出 `Dodge` / `Peach` 均不会改变权威状态，并写入拒绝日志。
- 命令行 smoke 可用 basic AI 或 scripted agents 跑完至少一局包含 `Slash` / `Dodge` / `Peach` 的对局片段，日志能看到 Command 生命周期、响应窗口和伤害 / 回复事件。
- `FSGSReplayLog` 能关联本计划的 CommandLog、EventLog 和必要的随机记录；重复 seed / 脚本输入下结果稳定。
- `CheckInvariants()` 在每个关键测试末尾通过。
- 没有 UI 依赖、网络依赖或测试专用捷径进入正式规则路径。

---

## 7. 进度与决策记录

- 2026-06-28：创建计划。定位为最小可玩规则纵切，先让服务器权威规则真实支持杀 / 闪 / 桃和濒死 / 求桃，再接 Plan 0011-M1 的最小 UI。
- 2026-06-28：完成实现并验证。新增 `UseCard` / `RespondCard` Command、基础牌牌库定义、Scripted / Basic AI、杀 / 闪 / 桃与濒死求桃流程、最小出局事件和自动化测试。`Development` 构建通过；`SGS.Plan0005.BasicCards` 3 个自动化测试全部 `Success`；无人值守 game smoke 走 `LocalHuman=false` 并在 8 回合后结束。
