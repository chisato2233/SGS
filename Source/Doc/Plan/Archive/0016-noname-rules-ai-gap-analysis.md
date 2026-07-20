# 0016 — NoName 规则引擎与 AI 架构对照评估

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-07-20 |
| 最近更新 | 2026-07-20 |
| 关联需求 | `0000-RawRequirements.md` 中的第 29 条 |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/AI/`、`Source/SGS/Server/Engine/`；本地参考 `QSgsRef/NoName/noname-for-dummies-main/` |

---

## 1. 目标与口径

本计划回答两个问题：NoName 的核心规则引擎和 AI 为什么能支撑大量卡牌、技能与模式；SGS 当前已经具备哪些对应地基，距离可持续扩展还缺什么。

结论采用“框架能力、真实消费者、内容覆盖”三层口径，不以文件数量或武将数量直接判断架构优劣。成熟度定义如下：

- **成熟**：存在稳定框架，并已被多类真实玩法复用。
- **部分具备**：已有正确接口或单个纵切，但覆盖面、组合能力或扩展入口不足。
- **缺失**：没有通用入口，或当前只能通过具体卡牌/模式硬编码完成。

版本基准：上游 `libccy/noname` 的 `master` 在调研日标注为 `1.10.16`，本地快照为 `1.10.11.3`。上游核心的 `GameEvent`、`Basic AI` 与本地快照保持同一总体结构，因此本地快照用于精确源码定位；扩展目录和懒人包 UI 魔改不作为 NoName 核心架构证据。

上游证据：

- 仓库与核心目录：[libccy/noname](https://github.com/libccy/noname)。
- 当前版本：[game/update.js](https://raw.githubusercontent.com/libccy/noname/master/game/update.js)。
- 当前通用选择器：[noname/ai/basic.js](https://raw.githubusercontent.com/libccy/noname/master/noname/ai/basic.js)。

本计划只吸收设计思想。NoName 采用 GPL-3.0；SGS 不复制其实现代码。

## 2. 总体判断

### 2.1 一句话结论

NoName 是“动态事件解释器 + 高密度内容钩子 + 分散式启发 AI”的成熟内容平台；SGS 是“服务器权威 Command/Rule/Effect 管线 + 强类型状态与审计”的早期规则平台。SGS 的底层方向更适合可靠联机和长期维护，但它目前只证明了基础牌纵切，尚未把时机、修正、内容作者入口和 AI 语义抽象到能低成本承载复杂技能的程度。

### 2.2 各自优势

**NoName 的优势**

- 一个 `GameEvent` 模型覆盖父子事件、前后队列、响应选择、触发技能和异步内容，新增内容可以直接组合已有事件。
- 卡牌与技能在定义处携带过滤、执行、修正和 AI 评价，内容覆盖增长时不必持续扩写中央 AI 分支。
- `effect/attitude/order/useful/threaten` 形成共享评价词汇，简单贪心选择器也能复用大量内容知识。
- 身份局 AI 会依据公开行为暴露度调整态度，而不只是使用固定阵营表。

**SGS 的优势**

- 客户端、真人和 AI 都只能提交 Command；规则合法性与状态修改留在服务器。
- Payload、Rule、ResolutionFrame 与 EffectStep 都有明确类型和不变量，失败能携带结构化原因。
- RandomAudit 和 ReplayLog 为确定性、复盘与未来回放验证提供了 NoName 核心代码中不够集中的边界。
- Public/Private Snapshot、DecisionAgent 与规则层分离，适合约束隐藏信息和网络可信边界。

### 2.3 核心风险

- 若 SGS 直接照搬 NoName 的全局事件对象、动态脚本和 UI 选择状态，会破坏服务器权威、类型安全和可审计性。
- 若 SGS 继续为每张牌增加专用 Rule、continuation 与 BasicAI 分支，则复杂技能出现后会迅速形成组合爆炸。
- 当前 BasicAI 直接读取权威 Seat.Identity。它在 0015 的固定 Demo 中是明确的占位策略，但不能作为未来隐藏身份 AI 的信息模型。

## 3. 职责分层与运行链路

本节只描述运行职责和行为顺序；具体类依赖继续以两个仓库各自的 graphify 图谱为准。

### 3.1 NoName 核心职责

| 层 | 主要职责 | 证据 |
|---|---|---|
| GameEvent | 保存 `parent/next/after/step/result`，支持 finish/goto/redo、父链查询和触发派生 | `noname/library/element/gameEvent.js:15,301,378,382,512,787` |
| Game loop | 选择当前事件，执行 next/after，处理 traditional step、Promise、Generator、AsyncFunction，并在暂停后恢复 | `noname/game/index.js:6042-6438` |
| Trigger arranger | 收集全局与玩家技能，按 priority、座次和 firstDo/lastDo 排列触发 | `noname/library/element/gameEvent.js:730-929` |
| Choice content | 把 use/respond/button/card/target 选择变成事件，真人 UI 与 AI 走同一选择状态 | `noname/library/element/player.js:4470,4522`；`noname/library/element/content.js:3871,4159` |
| Modifier | 在查询距离、次数、目标合法性、手牌上限等事实时遍历技能修正 | `noname/game/index.js:7967-8005`；`character/standard.js:275,1328,1408,1524` |
| AI selector | 对当前合法 button/card/target 逐个评分并贪心加入选择集 | `noname/ai/basic.js:13,81,168` |
| AI evaluator | 汇总卡牌/技能 AI 元数据、双方技能、态度、威胁、连环和局面状态 | `noname/get/index.js:5162,5224,5408,5447,5659` |
| Identity model | 用 `ai.shown`、`identity_mark` 和公开行动调整未知身份态度 | `mode/identity.js:3055-3410,3671-4302` |

典型事件顺序是：内容创建 `GameEvent` → 加入当前事件 `next` → loop 进入子事件 → 自动触发 `Before/Begin` → content 可能继续创建选择或效果子事件 → finish 后触发 `End/After` → 执行 `after` 队列 → 回到 parent。`game.pause/resume` 和 Promise/Generator 路径共同承担等待玩家或动画的挂起恢复。

### 3.2 SGS 核心职责

| 层 | 主要职责 | 当前真实程度 | 证据 |
|---|---|---|---|
| DecisionAgent | 真人和 AI 共用异步请求接口，返回 Command 而非直接修改状态 | 成熟接口、两类真实代理 | `Source/SGS/Shared/Decisions/SGSDecisionAgent.h:25-38` |
| CommandRouter | 校验共同信封、记录生命周期、构造 typed RuleInvocation | 已进入全部基础牌输入 | `Source/SGS/Server/Engine/SGSGameDriver.cpp:329-402` |
| RuleRegistry | 按 kind/intent/subject/window 索引，按 priority 和注册顺序稳定选择或广播 | 成熟地基；默认仅注册基础牌 | `Source/SGS/Server/Rules/Core/SGSRuleRegistry.cpp:158,226,378-410` |
| TimingEvent | 以开放 EventTag、EventName、TimingPoint 生成 Trigger Invocation | 接口已接通；当前无默认 TriggerRule 消费者 | `Source/SGS/Server/Engine/SGSGameDriver.cpp:783-854`；规则目录无 `SGSRuleKinds::Trigger()` 注册 |
| ResolutionStack | 保存 typed FrameState、父子 Frame 和响应窗口，恢复卡牌结算 | 杀闪、濒死求桃真实使用 | `Source/SGS/Server/Rules/Resolution/SGSResolutionStack.cpp:78,101,176` |
| EffectPipeline | 顺序执行 EffectStep，可挂起/恢复并写 Replay Event | 抽牌、伤害、移动等真实使用 | `Source/SGS/Server/Engine/SGSGameDriver.cpp:169-176,860-881` |
| ActiveEffectTimeline | 表达本阶段/本回合等持续状态并按 TimingPoint 过期 | SlashUsed、酒状态真实使用 | `Source/SGS/Server/Rules/Actions/BasicCards/SGSSlashRule.cpp:104-113`；`Source/SGS/Server/Engine/SGSGameDriver.cpp:899-905,1074-1085` |
| Random/Replay | 统一业务随机，记录 Command、Random 与 Effect/Event 日志 | 已进入开局、洗牌与基础规则 | `Source/SGS/Shared/Core/SGSRandomAudit.cpp:21-149`；`Source/SGS/Server/Engine/SGSGameDriver.cpp:885-890` |
| BasicAI | 读取合法 Option，按固定牌名和真实身份返回 Command | 0015 真实消费者，但仅占位策略 | `Source/SGS/Server/AI/SGSBasicAIAgent.cpp:12-255` |

SGS 当前典型顺序是：DecisionAgent 返回 Command → CommandRouter 校验并构造 RuleInvocation → RuleRegistry 选择一个 Action/Response Rule → Rule Validate/Execute → Rule 通过 Runtime 压入 ResolutionFrame、打开响应窗口或运行 EffectStep → 子 Command 完成后 GameDriver 按 continuation 恢复父 Frame → ReplayLog 同步审计记录。

## 4. 能力矩阵

| 能力 | NoName | SGS | 判断与影响 | 优先级 |
|---|---|---|---|---|
| 权威输入边界 | 部分具备 | 成熟 | NoName 的本地/联机事件与 UI 状态交织；SGS 统一 Command 与服务端校验应保留 | 保持 |
| 父子结算与挂起恢复 | 成熟 | 部分具备 | SGS Frame 已可挂起真实响应，但 continuation 只有两个固定名字，新增嵌套玩法仍需 Driver 分支 | P1 |
| 开放时机事件 | 成熟 | 部分具备 | SGS 有 TimingPoint 与 Trigger Dispatch，却没有默认真实 TriggerRule，尚未证明修改/取消链 | P1 |
| 触发顺序 | 成熟 | 部分具备 | 两者都有 priority；NoName 还处理 firstDo/lastDo、座次和同优先级选择，SGS 目前只按 priority/注册序 | P1 |
| 取消、替换、改写结果 | 成熟 | 缺失通用协议 | SGS Rule 可报错或执行 Effect，但没有统一的 event cancel/replace/result mutation 契约 | P1 |
| 统一响应选择 | 成熟 | 部分具备 | SGS 已统一 UI 与 ResponseRequest，但 Play/Response 是两个 Agent 方法，复杂按钮、多卡、多目标仍未形成完整选择协议 | 后续协议扩展 |
| 查询修正器 | 成熟 | 缺失 | SGS 距离、Slash 次数和目标判断目前在具体 Context/Rule 中完成，技能修正会逼出分支或重复查询 | P1 |
| 持续效果生命周期 | 部分具备 | 部分具备且边界更强 | SGS Timeline 更可审计，但当前自动过期只覆盖少量状态，尚未与通用触发/修正连接 | P1 |
| 内容作者入口 | 成熟 | 部分具备 | NoName 卡牌/技能对象集中声明 trigger/filter/content/mod/ai；SGS 每个复杂行为仍需要 C++ Rule 与手动注册 | P1 后续 |
| 确定性随机 | 缺失统一边界 | 成熟 | NoName 核心仍广泛直接使用 `Math.random/randomGet`；SGS RandomAudit 不应退让 | 保持 |
| 结构化回放审计 | 部分具备 | 成熟地基 | NoName 有 video/replay，但不等同于命令、随机、效果三日志一致性验证 | 保持 |
| 通用合法候选 | 成熟但与 UI 状态耦合 | 部分具备 | SGS Request.Options/ResponseCardIds 已由服务器提供合法候选，这是 P0 可直接复用的正确输入 | P0 |
| 通用选择算法 | 成熟 | 缺失 | SGS 没有 action/card/target 的共同打分与组合选择器 | P0 |
| 行动顺序与资源价值 | 成熟 | 缺失 | BasicAI 直接 Peach→Slash/酒→Pass，无法由内容定义 order/useful | P0 |
| 效果与目标价值 | 成熟 | 缺失 | SGS 没有统一 effect score、attitude、threat、双方技能修正或评分分解 | P0 |
| 卡牌/技能 AI 元数据 | 成熟 | 缺失 | 新卡必须修改 BasicAI 主分支，不符合开放扩展目标 | P0 |
| 身份不确定性 | 成熟启发式 | 缺失 | SGS AI 直接读取 Candidate.Identity，隐藏身份玩法会越过玩家可见信息 | P2 |
| AI 信息隔离 | 部分具备 | 缺失专用边界 | SGS 有 Public/Private Snapshot，但 BasicAI 绑定 GameContext，尚无 AI 专用只读视图 | P0 |
| AI 确定性与可解释性 | 部分具备 | 缺失 | SGS 可凭稳定候选顺序保持表面确定，但没有 ScoreBreakdown 或明确 tie-break 契约 | P0 |
| 多步搜索/模拟 | 缺失 | 缺失 | NoName 的核心仍以启发式贪心为主；SGS 当前无需把搜索当作追赶目标 | P3 |

## 5. 四条场景追踪

### 5.1 杀 → 闪

**NoName**

1. `sha` 内容创建目标的 `chooseToRespond({name: "shan"})`，并可调整 `shanRequired`：`card/standard.js:83-209`。
2. `chooseToRespond` 事件复用 filterCard、技能选择、AI 评分与确认/取消：`noname/library/element/content.js:4159-4342`。
3. 可响应技能只需声明 `enable: ["chooseToUse", "chooseToRespond"]` 与 viewAs/filter/ai，不需要为每个 UI 新建流程；例见 `card/standard.js:3377-3405`。

**SGS**

1. Slash Rule 校验次数、卡牌、目标和距离，写 SlashUsed，压入 typed Slash Frame：`Source/SGS/Server/Rules/Actions/BasicCards/SGSSlashRule.cpp:46-124`。
2. Rule 打开 `Slash.Dodge` 响应窗口；若无人可响应则直接命中：同文件 `:128-139`。
3. Dodge Command 由 Response Rule 再次验证窗口与卡牌，弃闪并完成当前 Frame：`Source/SGS/Server/Rules/Responses/BasicCards/SGSDodgeRules.cpp:39-82`。

**判断**：SGS 的权威与类型边界更强，真实纵切已经成立；NoName 在“响应技能、需要多张闪、改写响应条件”上的扩展成本明显更低。P1 应补通用时机与修正，不能为每种响应技能新增 Driver continuation。

### 5.2 濒死 → 桃/酒

**NoName**

1. `_save` 创建求救 Trigger，随后复用 `chooseToUse` 选择可救援牌或技能：`noname/library/element/content.js:9273-9340`。
2. 桃、酒及转换技能通过现有 filter/save tag/AI 元数据加入统一选择，不需要求桃事件知道每个具体技能。

**SGS**

1. Dying Frame 保存濒死者、响应顺序和下一响应者：`Source/SGS/Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h:8-23`。
2. 桃与本人酒分别使用 typed ResponseRule，验证当前 FrameState 后执行 Heal：`Source/SGS/Server/Rules/Responses/BasicCards/SGSDyingPeachRules.cpp:100-192`。
3. 成功则 CompleteCurrentFrame，失败继续下一响应者或淘汰：同文件 `:10-43,94-97`。

**判断**：SGS 已正确处理多响应者与服务端校验，但可接受救援方式仍由具体 Rule 注册和固定 continuation 组织。未来武将救援技能需要统一 Choice/Trigger 扩展点，而不是继续增长 Dying 专用分支。

### 5.3 技能/状态修改目标、距离或次数

**NoName**

1. 查询路径调用 `game.checkMod`，遍历相关技能并按 priority 应用修正：`noname/game/index.js:7967-8005`。
2. 标准技能已用 `maxHandcard/cardUsable/targetEnabled/globalFrom/globalTo` 等命名钩子修改通用规则：`character/standard.js:275,1328,1408,1524-1542`。

**SGS**

1. Slash Rule 直接判断 `HasStatus(SlashUsed)` 与 `GetDistance(...) > 1`：`Source/SGS/Server/Rules/Actions/BasicCards/SGSSlashRule.cpp:57-76`。
2. 酒杀增伤与 SlashUsed 通过 ActiveEffectTimeline 实现：同文件 `:104-113`。
3. Context 已支持坐骑带来的基础距离变化，但没有让技能组合修改任意规则查询的统一接口：`Source/SGS/Server/Engine/SGSGameContext.cpp:505-545`。

**判断**：SGS 的持续状态存储地基可复用，真正缺口是“规则查询值如何被多个来源稳定修正”。P1 应建立 typed Query/Modifier 管线，并要求最终合法性仍由服务器 Rule 判定。

### 5.4 隐藏身份目标选择

**NoName**

1. 行动会修改 `ai.shown` 并推断 `identity_mark`：`mode/identity.js:3055-3410`。
2. `rawAttitude` 把公开身份、暴露度、身份标记和场上局势合成为目标态度：`mode/identity.js:3671-4302`。
3. 通用 `effect` 再用 attitude 与 threaten 把对目标的规则效果换算为行动者收益：`noname/get/index.js:5447-5837`。

**SGS**

1. BasicAI 的 `GetTargetPriority` 和 `CanRescue` 直接读取自己与候选 Seat.Identity：`Source/SGS/Server/AI/SGSBasicAIAgent.cpp:12-45,75-145`。
2. UI Snapshot 会按 viewer、主公公开、死亡与终局规则隐藏身份，说明项目已有正确的展示可见性语义：`Source/SGS/Shared/UI/SGSTableViewTypes.h:183-304` 及相应 Driver snapshot 构建。

**判断**：当前 AI 确实拥有玩家不应拥有的信息。这是 0015 明示的最小身份 Demo 策略，不是漏洞修复范围；P0 必须先提供信息安全 WorldView，P2 再在公开事件之上建立身份信念。

## 6. 采纳与拒绝

### 6.1 采纳的设计思想

- **统一候选与选择语义**：规则层产生合法候选；真人 UI 与 AI 对同一候选模型作选择。
- **内容携带 AI 语义**：卡牌、技能和状态可以注册局部评价，不让中央 AI 认识每个名字。
- **共享评价词汇**：行动顺序、资源价值、直接效果、目标关系、威胁与风险分开计算，再汇总并保留明细。
- **查询时修正**：距离、次数、目标、成本、上限等规则事实通过统一 typed 管线组合修正。
- **开放时机与稳定排序**：TimingEvent 采用开放标识；优先级、座次/所有者和注册序形成完全确定的排序键。
- **公开行为驱动身份认知**：信念只消费公开事件，不读取权威隐藏身份。

### 6.2 明确拒绝移植

- 不引入 `_status` 等全局可变规则上下文。
- 不让 AI 通过操作 DOM/Slate 选择状态完成规则选择。
- 不让客户端运行可信规则或直接写规则状态。
- 不使用动态 JavaScript 对象、字符串 step 编译或运行时 `eval` 承载 SGS 规则。
- 不使用裸 `Math.random()` 参与业务决定；继续统一走 RandomAudit。
- 不复制 NoName 源码；只在 SGS 既有 C++/UE 架构中实现相同能力目标。

## 7. 后续路线图

### P0 — 通用 AI 评价框架（最高优先级）

目标：把 BasicAI 从“按 CardName/Identity 分支”改为“合法候选 + 信息安全视图 + 可组合评分”。

建议的最小公开契约：

- **只读 AI WorldView**：只包含该 Seat 可见的身份、体力、公开区、手牌数量、自己的私有手牌、当前阶段和公开历史；禁止暴露其他玩家隐藏身份与手牌实体。
- **ActionCandidate**：包装现有 PlayPhase Option 或 Response Option，保留 Command 所需稳定 ID、卡牌/技能开放标识和合法目标组合。
- **ScoreContext**：行动者、候选、WorldView、当前请求与稳定决策序号。
- **ScoreBreakdown**：按 Order、Resource、Effect、Attitude、Threat、Risk、RuleModifier 记录具名分量、总分和拒绝原因。
- **Evaluator Registry**：通用 evaluator 与卡牌/技能 evaluator 按 GameplayTag/FName 注册；新增卡牌只注册评价，不修改 BasicAI 主选择流程。

确定性规则：先比较总分，再比较显式优先级、CardId、目标 SeatIndex 和注册序；若需要随机化，只能通过带 Purpose 的 RandomAudit，并写入决策审计。

P0 不包含身份推理和搜索。未知身份先使用中性/公开阵营关系，避免继续窥视真实身份。

### P1 — 规则触发与修正能力

目标：让首个复杂被动技能能只通过 RuleRegistry/TimingEvent/Modifier 接入，不增加 GameDriver 专用分支。

- 扩展现有 EventPayload，使 TriggerRule 能返回 Continue、Cancel、Replace 或 MutateResult 等 typed 结果；每次改写写 Replay Event。
- 为 Trigger 排序明确 Priority、OwnerSeat、RegistrationOrder；需要 first/last 语义时使用开放 TimingStep 或显式阶段，不复制 NoName 特殊布尔字段。
- 建立 typed RuleQuery/Modifier 管线，首批覆盖 Distance、CardUsableCount、TargetLegality、MaxHandSize。
- ActiveEffectTimeline 作为临时/装备/技能修正来源之一；RuleRegistry 仍是规则执行入口，最终校验不能由 AI 或 UI 替代。
- 把 continuation 从 Driver 的名字分支逐步迁为可注册的 typed continuation handler，先用一个真实嵌套响应技能验证。

### P2 — 身份认知模型

目标：基于公开行动维护每个观察者自己的身份信念，而非保存全局唯一猜测。

- 输入仅限 Public Event/Replay Event、已公开身份和当前可见局面。
- 输出为对公开阵营标签的置信分布和派生 Attitude；真实身份只用于终局评测，不能参与在线决策。
- 首批规则覆盖攻击、救援、放弃救援与明确身份公开；复杂伪装和博弈推断后置。
- 用固定 Replay Event 序列验证同一观察者信念确定性、不同观察者信息隔离和终局准确率统计。

### P3 — 策略增强

仅当 P0 已覆盖基础牌、首批锦囊、装备和至少一个技能包后评估：

- 一至两步有限前瞻或候选后果模拟。
- 手牌保留、连锁伤害、响应资源与胜负临界状态的专用策略层。
- 性能预算、可取消异步任务和服务器帧时间约束。

NoName 核心 AI 本身主要是启发式贪心，因此“追赶 NoName”不要求立即建设 MCTS、行为树或机器学习系统。

## 8. 后续验收规格

### P0 必须验证

- AI WorldView 中看不到未公开身份与其他玩家手牌实体。
- 相同 WorldView、Request、Evaluator 集合和 RandomAudit 状态产生完全相同的 Command。
- 新增一张测试卡的评分消费者无需修改通用选择器或 BasicAI 主分支。
- 每个选择都能输出 ScoreBreakdown，解释胜出候选、拒绝候选和 tie-break。
- Slash、Peach、Analeptic、Dodge 与 Dying 响应继续只提交服务器合法 Command。

### P1 必须验证

- 多个同 TimingEvent 规则按稳定顺序执行，同优先级结果不依赖容器遍历顺序。
- Cancel/Replace/MutateResult 均写入 ReplayLog，非法修改返回结构化错误且不部分落地。
- Distance、CardUsableCount、TargetLegality、MaxHandSize 至少各有一个真实 modifier 消费者。
- Modifier 只影响服务器查询，客户端和 AI 不能声明最终合法性。
- 新嵌套响应玩法不增加新的 GameDriver continuation `if` 分支。

## 9. 完成核验

- [x] 核对 NoName 上游版本与本地快照版本。
- [x] 使用本地独立 graphify 图谱和精确源码位置验证 NoName 核心结构。
- [x] 使用 SGS 主图谱和源码调用点区分规则地基、真实消费者与预留接口。
- [x] 完成规则引擎、AI、四条场景、能力矩阵和 P0-P3 路线图。
- [x] 未修改生产代码、未运行 UE 构建/自动化、未修改两个 graphify 图谱。

## 10. 进度与决策记录

- 2026-07-20：完成 NoName 1.10.16/本地 1.10.11.3 核心规则与 AI 对照。确认 SGS 不应移植 NoName 动态运行时，但应优先吸收通用评价、规则修正和公开行为身份认知。
- 2026-07-20：评估完成并归档。下一实施优先级固定为 P0 通用 AI 评价框架，随后是 P1 规则触发与修正能力。
