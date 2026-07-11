# 0000 — 原始需求档案

> 本文件按**时间顺序**记录用户键入的原始需求（尽量保留原话）。
> 规则：**只追加，不修改历史条目**。最新的放在最上面。
> 每条需求若衍生出具体计划，在「关联计划」里注明对应的 `NNNN-*.md`。

---

## #23 — 2026-07-11 — 完成 M2.3 至 M2.6，Token 集中于生产代码

**原话：**
> 继续按照这上面的计划，完成M2.3 - M2.6，依然是不要把token花费在测试或者测试代码开发上

**要点拆解：**
- 完成通用 State/Binding、typed signal/lifetime、Host/Feature 边界和第二真实消费者。
- 不新增测试、不扩写测试场景、不运行自动化；仅允许为生产接口迁移做最小现有测试编译适配。
- Token 优先投入生产框架代码、真实 Table 接入和规则固化。

**关联计划：** `0011-M2-react-inspired-ui-foundation-events.md`。

---

## #22 — 2026-07-11 — 将 M2 重心纠偏为业务无关组件框架与 Anti-God-Widget

**原话：**
> 审查一下这一份计划，，我更想构建的是与业务无关的参考React的UI代码框架。。。。。并且最大的目的是拆分Widget并避免后续出现类似Widget这种指责单一的东西，像一个资深工程师一样审视这份框架计划，保留或增加价值最高的地方，删掉不必要的部分

**上下文：**
- 原 M2 把 LocalPlayer Store、Selector、Action Dispatcher 与完整 Event Hub 放在组件拆分之前，容易先建设业务基础设施，迟迟不解决单体 Widget 膨胀。
- 已实现的 `FSGSLocalUIStateStore` 直接依赖 Table Snapshot，具有真实价值，但不满足业务无关 Core 的定义。
- 用户明确将最大目标调整为组件职责拆分、组合复用与防止新的 God Widget。

**要点拆解：**
- 通用 Core 不依赖 Table、PlayerController、GameDriver、Server 或卡牌 / 玩家类型。
- React 思想映射为 props、callback、state ownership、context、composition 与生命周期安全订阅，不复制虚拟 DOM。
- 将组件契约和牌桌 God Widget 拆分提前，Store / Dispatcher / Event Hub 只按真实消费者逐步提炼。
- UI 事件框架缩减为 typed signal + subscription handle + lifetime scope；普通父子交互继续使用 callback。
- 现有 `FSGSLocalUIStateStore` 保留为 Table feature 临时适配器，后续迁出 Foundation Core。

**关联计划：** `0011-M2-react-inspired-ui-foundation-events.md`。

---

## #21 — 2026-07-11 — 制定 React-inspired UI Foundation 与 UI 事件框架计划

**原话：**
> 我需要你制定一个详细的执行计划以达成我们的目标，为了省token不要写代码，目标是借鉴React的模块化，单向数据流和组件组合思想，实现一套健壮的UI开发框架，同时也顺便构建一套复用性高的专属于UI的事件框架，让我们开始吧

**上下文：**
- 当前 M1 已建立真实牌桌 Snapshot、布局、Theme 与决策路径，但牌桌根 Widget 同时承担刷新、绘制、资源、选择和动作提交，继续增加装备、技能与交互会加速膨胀。
- 项目仍坚持 Unreal Native Code-first UI，不采用 React / WebView 作为运行时；本需求只借鉴现代前端的架构思想。
- 用户明确要求本阶段只制定详细执行计划，不编写代码，并继续遵守 graphify 优先与截图逐次确认的 Token 节省规则。

**要点拆解：**
- 建立 Snapshot / Local State / Props / Action / Event 的明确语义与单向数据流。
- 建立 local-player UI store、selector、typed action dispatcher、scope/lifecycle 和可诊断的 UI Event Hub。
- 以现有牌桌真实路径渐进拆分 Container、Presentational Component、Primitives 与 Host Adapter。
- UI Event 只服务表现通知，不复制 CommandRouter、ReplayLog、规则 TimingEvent 或 RPC。
- 用自动化覆盖 revision、scope 隔离、订阅生命周期、重入、事件风暴、过期动作和真实基础牌纵切。

**关联计划：** `0011-M2-react-inspired-ui-foundation-events.md`。

---

## #20 — 2026-07-11 — 修正牌桌摄像机并按 NoName 重建原始牌桌布局

**原话：**
> 目前是这个情况。。。。。1. 摄像头错位了，修一下摄像头，2. 武将牌的显示和玩家武将的显示，以及玩家的手牌区没有很好的格式化，我们着重开始搭建最原始的牌桌的正确布局，可以参考NoName代码的布局！
>
> NoName代码在QSgsRef/NoName，记得用graphify省token

**上下文：**
- 当前 0011-M1 已有八人 Nova 坐标与本地决策桥，但 PIE 截图中牌桌只覆盖左上区域，世界网格大面积露出。
- 玩家区仍是纯文本黑框，手牌使用横向文本按钮，没有形成武将面板与牌面底栏。
- 用户明确授权参考 `QSgsRef/NoName`，并要求优先使用它自己的 graphify 图谱缩小读取范围。

**要点拆解：**
- 修正牌桌 Pawn 的摄像机位置和朝向，使正交相机居中俯视世界桌面。
- 背景成为全视口独立底层；布局使用本帧真实 Slate geometry，兼容高 DPI 与紧凑窗口。
- 按 NoName Nova 语义拆分完整 seat 0、其他玩家武将面板、手牌底栏和独立控制条。
- 使用已导入素材形成第一版武将图与基础牌面展示；在武将规则模型建立前，武将图仅为表现占位，不进入规则事实源。
- 增加摄像机与 640×360 至 1920×1080 的八人布局自动化，并保留 PIE 点击验收。

**关联计划：** `0011-M1-minimal-code-first-table-ui.md`。

---

## #19 — 2026-07-04 — 收束 Plan0014：Rule 骨架阶段到此为止

**原话：**
> 先将Rule骨架留在这里，我们还没有开始真正的游戏逻辑的开发，比如距离计算这些东西。。。修改Plan0014

**上下文：**
- Plan 0014-M2/M3/M4/M5 已完成 typed Rule、ResolutionStack、Trigger dispatch 和目录组织地基。
- 父计划仍把 Modifier / ViewAs / 距离或目标修正写作后续未完成项，容易造成文档偏移。
- 用户明确要求 Rule 骨架先停在这里；距离计算等真实游戏逻辑后续另起计划。

**要点拆解：**
- 将 `0014-rule-layer-long-term-refactor.md` 收束为“Rule 层骨架已完成”的归档计划。
- Modifier、ViewAs、距离/目标修正、具体技能、更多卡牌逻辑不再作为 0014 未完成项。
- 本次只做文档边界调整，不修改运行时代码。

**关联计划：** `Archive/0014-rule-layer-long-term-refactor.md`。

---

## #18 — 2026-07-04 — 执行 0014-M5：TriggerRule Dispatch 地基

**原话：**
> PLEASE IMPLEMENT THIS PLAN:
> # Plan 0014-M5：TriggerRule Dispatch 地基

**上下文：**
- Plan 0014-M2/M3/M4 已完成 typed Rule、可恢复 ResolutionStack、TimingEvent 接缝和 Rule 目录整理。
- M3 留下的 `PublishTimingEvent` 仍是空实现，`TriggerRule` 尚未通过 `RuleRegistry` 被索引和同步调度。
- 用户要求本阶段只完成 Trigger dispatch，不引入 Modifier / ViewAs，也不新增正式技能规则。

**要点拆解：**
- 将 `TimingEvent` 映射为 `RuleKind=Trigger` 的 typed `FSGSRuleInvocation`。
- `RuleRegistry` 新增 Trigger 专用 `DispatchAll`：候选按 descriptor 查询、priority/order 排序并同步执行全部可处理规则。
- `USGSGameDriver::PublishTimingEvent` 接通 Registry dispatch；无默认 Trigger 时保持当前基础牌行为不变。
- 新增最小自动化覆盖 Trigger 排序执行、无候选 no-op、payload mismatch 诊断。

**关联计划：** `0014-M5-trigger-rule-dispatch.md`。

---

## #17 — 2026-07-03 — 执行 0014-M4：Rule 目录与 Payload 组织重构

**原话：**
> PLEASE IMPLEMENT THIS PLAN:
> # Plan 0014-M4：Rule 目录与 Payload 组织重构

**上下文：**
- Plan 0014-M2/M3 已完成 typed Rule、ResolutionStack、resume 和 TimingEvent 地基。
- 当前 Rule 文件仍主要平铺在 `Source/SGS/Server/Rules/`，`SGSBasicCardRules.*` 与 `SGSRulePayloads.h` 继续增长会影响后续新增卡牌 / 技能的落点。
- 用户明确要求本阶段不新增玩法 Rule，先优化结构。

**要点拆解：**
- 按 `RuleKind + 内容包` 组织目录：Core、Payloads、Resolution、Actions/BasicCards、Responses/BasicCards、BasicCards。
- 拆分 payload headers，保留 `SGSRulePayloads.h` 作为 umbrella include。
- 将 Slash / DyingPeach frame state 从通用 `ResolutionStack` 移到对应 Rule 模块。
- 基础牌通过内容包注册入口统一注册，默认规则注册不直接 include 每个具体 Rule。

**关联计划：** `0014-M4-rule-module-layout.md`。

---

## #16 — 2026-07-03 — 执行 0014-M3：ResolutionStack Resume 与受控 TimingEvent 地基

**原话：**
> PLEASE IMPLEMENT THIS PLAN:
> # 0014-M3 执行计划：ResolutionStack Resume + 受控 TimingEvent 地基

**上下文：**
- Plan 0014-M2 已移除 legacy `FSGSRuleRequest` 路径，并把基础牌 pending 状态迁入 `ResolutionStack`。
- 当前 `ResolutionStack` 仍偏向响应窗口容器，基础牌完成时会重置整栈，无法正确表达子结算完成后恢复父结算。
- 用户明确要求测试相关修改和运行放到最后统一处理，前期优先运行时代码与文档状态更新。

**要点拆解：**
- 将 `ResolutionStack` 升级为可恢复结算栈：子 frame 完成后 pop 并按 continuation 恢复 parent。
- 用通用 Runtime 接口替代基础牌专用的 processing card / dying responder API。
- 迁移 Slash / Dodge / Peach / DyingPeach 到 LIFO resume 语义，支持嵌套濒死的长期方向。
- 建立最小受控 TimingEvent / TriggerRule 接缝，但不完整实现 Trigger / Modifier / ViewAs。

**关联计划：** `0014-M3-resolution-stack-resume-timing-event.md`。

---

## #15 — 2026-07-03 — 创建 0014 第二阶段执行计划，要求完全替代旧兼容层

**原话：**
> 开始构建第二阶段的开发计划，请注意，现在还在早期开发阶段，不要为了历史老旧版本做一些额外的降低代码优雅和质量的兼容层，完全的替代

**上下文：**
- Plan 0014 第一阶段已实现 typed `RuleInvocation` 与 Indexed RuleRegistry 地基。
- 第一阶段为了保持基础牌回归，临时保留了 legacy `FSGSRuleRequest` 作为 BasicCardRules 兼容输入。
- 用户明确要求第二阶段不要继续背负早期过渡兼容层，应直接替换旧结构。

**要点拆解：**
- 创建 0014 的 M2 子计划，范围聚焦 typed Rule、ResolutionStack、legacy request 删除。
- `FSGSRuleRequest`、`BuildRuleRequest`、`SGSLegacyRuleRequests` 不再作为运行时代码路径存在。
- `USGSGameDriver` 中基础牌 pending 状态迁移到显式 `ResolutionStack / ResolutionFrame`。
- Trigger / Modifier / ViewAs 暂不并入 M2，避免第二阶段过大。

**关联计划：** `0014-M2-typed-rules-resolution-stack.md`。

---

## #14 — 2026-07-03 — 创建 Rule 层长期重构计划

**原话：**
> 很好，按照目前的讨论，创建一份详细的开发计划用于重构Rule层，

**上下文：**
- Plan 0013 已建立基础 `Command -> Rule -> EffectPipeline` 管线，但 `FSGSRuleRequest` 有继续膨胀为万能参数包的风险。
- 用户希望结合长期卡牌 / 技能开发需求，确定更适合海量 Rule、技能触发、modifier 和视为技的结构。
- 讨论中参考了 QSanguosha 的技能架构：C++ 稳定接口、TriggerSkill / ViewAsSkill / DistanceSkill / TargetModSkill 分类、事件表和优先级触发；但 SGS 不应照搬 Lua / QVariant / 直接 Room mutation 风格。
- 用户询问 `RuleRegistry` 是否能复用 `TSGSIndexedStore`，结论是适合作为 Registry 多索引底座。

**要点拆解：**
- 新建计划，重构 Rule 层为 typed `RuleInvocation`，避免继续给单一 `RuleRequest` 增加大量字段。
- 将 Rule 分类为 Action / Trigger / Modifier / ViewAs 等长期扩展点。
- 使用 `TSGSIndexedStore` 管理 `RuleEntry`，通过 `RuleDescriptor` 和索引查询候选 Rule。
- 将 Driver 中的 pending 规则状态下沉到显式 `ResolutionStack / ResolutionFrame`。
- 保持 CommandRouter、EffectPipeline、ReplayLog、GameContext 等服务器权威边界不变。

**关联计划：** `0014-rule-layer-long-term-refactor.md`。

---

## #13 — 2026-07-01 — 构建 Command 到 Rule 到 Effect 的规则管线

**原话：**
> 下一步？构建一下从Command到Rule到Effect的管线规则，先新建一个计划

**上下文：**
- Command 已完成 typed payload 与 CommandType 收口。
- UI 同步边界已拆到 GameState / PlayerController，下一步回到服务器权威规则层。
- 当前基础牌规则仍有一部分 Slash / Peach / Dodge 分发逻辑直接写在 `USGSGameDriver` 中。

**要点拆解：**
- 新建计划，设计并落地从 Command 校验、Rule 分发、EffectPipeline 状态变更的统一管线。
- Rule 层应承接卡牌 / 响应 / 后续技能规则，不让 Driver 继续膨胀为规则分支中心。
- 保持服务器权威、CommandRouter、EffectPipeline、ReplayLog 等现有基础工具为默认入口。

**关联计划：** `0013-command-rule-effect-pipeline.md`。

---

## #12 — 2026-06-28 — 创建 Plan0005 与 Plan0011-M1，强调长期项目质量门

**原话：**
> 就按你说的做先写Plan0005和Plan0011-M1，然后记得这个：使用Plan都是长期开发项目，不要写假代码不要写壳，不要糊弄，适当的考虑未来项目的长期稳定性和健壮性

**上下文：**
- 用户希望快速推进到带 UI 的最小可玩版，但接受先拆成基础规则纵切与最小 UI 纵切。
- Plan 文档必须服务长期开发，不接受假代码、空壳、只为演示而绕过真实架构的实现。

**要点拆解：**
- 创建 `0005`：杀 / 闪 / 桃、濒死 / 求桃、最小牌库与自动化验收，先让规则层产生真实可玩闭环。
- 创建 `0011-M1`：在 Plan 0011 的 Native Code-first UI 路线下做最小牌桌 HUD 与本地真人决策桥。
- 两个计划都必须写明真实使用入口、既有架构接入、失败路径、长期扩展边界和不可接受的假实现。

**关联计划：** `0005-basic-card-playable-rules.md`、`0011-M1-minimal-code-first-table-ui.md`。

---

## #11 — 2026-06-24 — 纠偏 Plan 0012：所有 P0/P1 里程碑都按一步到位验收

**原话：**
> 前面的Core Utility和Command + RandomAudit是不是也要按照最新的标准改一下？包括后面的M4，M5

**上下文：**
- 用户已明确 Plan 0012 的目标不是“低侵入第一版”，而是本周期基础工具库核心形态一步到位。
- 该标准不应只约束 M3；M1、M2、M4、M5 也必须按同一验收口径重构。

**要点拆解：**
- Core Utility、Command、RandomAudit、Timing / ActiveEffect、EffectPipeline、ReplayLog 都必须写明核心完成形态。
- 已落地的基础原语若只是过渡接入，不能标记为对应里程碑完成。
- 后续实现应减少未来大重构，而不是把关键抽象留给应用侧补救。

**关联计划：** `0012-sgs-foundation-toolkit.md`。

---

## #10 — 2026-06-24 — 纠偏 Plan 0012：基础工具库必须一步到位

**原话：**
> 注意我的Plan0012的目标就是一步到位而不是所谓可侵入的第一版，强调这个事实然后重构Plan0012，然后更新状态

**上下文：**
- 在讨论 `TSGSIndexedStore` 与 Boost.MultiIndex 的关系时，用户强调 SGS 卡牌世界天然存在多维查询需求：拥有者、Zone、牌堆、点数、花色、牌名、牌类以及组合条件。
- 当前 `TSGSIndexedStore` 的轻量外置索引与 `GameContext` 扫描式 TargetQuery 不应被视为 M3 完成形态。

**要点拆解：**
- Plan 0012 的目标是本周期基础工具库核心形态一步到位，而不是先做可侵入/低侵入第一版后续再大改。
- M3 应明确要求 SGS 版内建多索引 Store，而不是零散 `TMap/TArray` 或外围查询薄封装。
- 当前 M3 状态需要回退为未完成 / 需重构，并在计划中写明完成标准。

**关联计划：** `0012-sgs-foundation-toolkit.md`。

---

## #9 — 2026-06-23 — 基于开放规则概念重构项目

**原话：**
> 那么基于此重构目前的项目吧

**上下文：**
- 用户要求重新评估枚举使用条件，强调卡牌游戏中的阶段、回合、座次、花色、状态、目标等规则概念都可能被技能或模式改写。
- 项目已决定引入 GAS / GameplayTags。

**要点拆解：**
- 将现有规则/内容概念从封闭 `enum class` / `UENUM` 迁移到开放式标识。
- 标准三国杀阶段、花色、装备槽、牌区、事件和动作只作为默认标签基线，不代表全集。
- 保留服务器权威规则管线，不把规则核心外包给蓝图 Ability。

**关联计划：** `0012-sgs-foundation-toolkit.md`。

---

## #8 — 2026-06-23 — 改定规则：引入 GAS

**原话：**
> 现在就修改规则吧，要引入GAS

**要点拆解：**
- 用户明确推翻此前“不用 GAS”的硬约束。
- 项目应启用 `GameplayAbilities` / `GameplayTasks` / `GameplayTags`。
- GAS 作为属性、标签、GameplayEffect、GameplayCue / 表现桥接与可复用效果载体底座。
- SGS 的动作命令、随机审计、回放日志、牌区移动、响应窗口和三国杀式结算顺序仍由自研服务器权威规则管线控制。

**关联计划：** `0012-sgs-foundation-toolkit.md`。

---

## #7 — 2026-06-23 — SGS 基础工具库开发计划

**原话：**
> 基于目前的讨论结果，生成一个SGS工具库开发详细计划，目标是本周期工具库开发一步到位，足够健壮，足够有用，避免不断专门创建计划迭代工具库，未来应用方面只做针对需求的对工具库的小迭代和必要时的专门迭代，你的任务是生成一个计划，放在工程中合适的位置，不是执行代码编写
>
> PLEASE IMPLEMENT THIS PLAN:
> # 0012 — SGS 基础工具库开发计划
> （后续附完整计划草案：范围锁定 P0+P1，包含 GAS 评估门、结果与错误体系、动作命令系统、随机审计系统、实体与查询工具、时序与持续效果、效果管线与回放地基。）

**要点拆解：**
- 新建 `0012-sgs-foundation-toolkit.md`，作为本周期 SGS 规则层基础工具库的总计划。
- 目标是本周期尽量一次性规划并实现足够通用、健壮、有用的工具库，避免后续为每个基础工具反复开独立 Plan。
- 范围锁定 P0+P1：错误/结果、Command、RandomAudit、Handle/IndexedStore、TargetQuery、Timing/Duration/ActiveEffect、EffectPipeline、ReplayLog 地基。
- P2 工具（Bimap、BoundedHistory、完整 RangeMap 等）只作为后续按需小迭代。
- 计划创建阶段只写文档，不开始实现工具代码。

**关联计划：** `0012-sgs-foundation-toolkit.md`。

---

## #6 — 2026-06-19 — 将 AGENTS 融入当前文档系统并提交

**原话：**
> 将AGENTS融入当前的文档系统，你可能需要阅读ProjectBrief，该改动的改动，然后提交git

**要点拆解：**
- `AGENTS.md` 应成为 Codex/Agent 自动发现适配层，而不是新的项目事实源。
- `AGENTS.md` 需要指回 `ProjectBrief.md`、`Rulers.md`、`Source/Doc/Plan/` 启动协议。
- graphify 的 Codex 接入、图谱产物和 Git 跟踪/忽略规则需要与现有文档系统一致。
- 完成后提交 git。

**关联计划：** 文档系统维护，无独立 Plan。

---

## #5 — 2026-06-19 — UI 技术路线选择：路线 B（Unreal Native Code-first）

**原话：**
> 路线B吧

**上下文：**
- 在讨论 WebView/React/Vue、RmlUi、NoesisGUI、Slate/UMG 与「自研 Gameface」风险后，用户选择路线 B。
- 路线 B 含义：继续留在 Unreal 原生 UI 体系内，但不裸写混乱 UI；基于 Slate/UMG/CommonUI 按需组合，做 SGS 专用的轻量代码优先 UI 层。

**要点拆解：**
- 不把 React/Vue/WebView/CEF 作为主 UI 技术栈。
- 不自研完整 Gameface/浏览器级 UI runtime。
- 目标是：纯代码、现代、美观、组件化、状态驱动，但控制框架复杂度。
- SGSUI 只做薄封装：主题 token、通用组件、游戏组件、动画预设、状态/动作桥接。

**关联计划：** `0011-native-code-first-ui.md`。

---

## #4 — 2026-06-19 — 启动核心逻辑长计划（多人 + AI）

**原话：**
> 不，现在做下一步，我可能要开启一个长计划，就是开始实现游戏的核心逻辑，也就是开始系统性的写代码了，这需要分成多个plan逐步实现，需要没有文档偏移，合适的代码分层结构/文件夹，以及高质量的，富有健壮性的代码风格，请注意我需要制作多人游戏与ai并存的风格，ai逻辑参考qsanguosha

**要点拆解：**
- 开启"长计划"，系统性实现三国杀**核心逻辑**，分成多个 Plan 逐步推进。
- 要求：**无文档偏移**、合理的**代码分层/文件夹结构**、**健壮高质量**的代码风格。
- 架构要求：**多人游戏与 AI 并存**；AI 逻辑**参考 QSanguosha**。

**影响：** 推翻初版「当前阶段单机、不写复制代码」约束，改定为**服务器权威 + 决策代理（真人/AI 并存）+ 异步非阻塞**。已同步 `Rulers.md` 硬约束 #3 与 `ProjectBrief.md`（第 1/2/3/5 节）。

**关联计划：** `0002-core-logic-architecture-roadmap.md`（总纲 + 路线图），后续 0003…逐项实现。

---

## #3 — 2026-06-19 — 导入太阳神三国杀素材

**原话：**
> 下一步，额你知道github项目的太阳神三国杀吗？你能将他clone到工作台中，然后其中的卡牌，配音和音乐倒入我们的项目
>
> （追问确认）是的是个人学习使用，ui素材暂时先导入吧，接受

**要点拆解：**
- 把开源项目「太阳神三国杀 / QSanguosha」（`Mogara/QSanguosha`）的素材导入本项目。
- 范围：卡牌图、武将图、配音、音乐，**外加 UI 素材**（用户追加确认全量）。
- 用途：**个人学习 / 非商用**（用户明确）。

**关键约束：** 素材许可为 **CC BY-NC-ND 4.0**（署名 / 非商用 / 禁演绎传播）；代码为 GPLv3，本次**不导入任何代码**。需附署名文件，且**禁止商业化**。

**关联计划：** `0001-import-qsanguosha-assets.md`。

---

## #2 — 2026-06-19 — 建立文档系统

**原话：**
> 第一步，先建立一个文档系统，用于跨Agent持久记录进度和状态，我会使用graphify用于记录我的代码结构，我需要一个再根目录的ProjectBrief，记录技术栈，文档阅读指南和当前项目状态，是所有Agent的起点，我需要一个Source/Doc/Plan文件夹，里面放上后续每一个开发计划的计划文档，以及用户键入的原始需求，现在你的目标就是构建这样一个系统

**要点拆解：**
- 建立跨 Agent 持久记录进度/状态的文档系统。
- 代码结构由用户用 **graphify** 维护（文档不重复记录代码结构）。
- 根目录 `ProjectBrief.md`：记录技术栈、文档阅读指南、当前项目状态，作为所有 Agent 的起点。
- `Source/Doc/Plan/` 文件夹：存放后续每个开发计划的计划文档，以及用户键入的原始需求。

**关联计划：** 本条即文档系统本身（阶段 0），无独立 Plan 文档。

---

## #1 — 2026-06-19 — 项目立项与引擎选择

**原话：**
> 目前这是一个空的unreal工程，我想要实现一个简单的三国杀卡牌桌面游戏，求问，有没有推荐的unreal c++卡牌游戏模板，或者你更加建议我使用unity？

**要点拆解：**
- 空白 Unreal 工程，想做一个简单的三国杀卡牌桌面游戏。
- 咨询是否有 Unreal C++ 卡牌模板，或是否改用 Unity。

**结论：** 留在 Unreal + C++；强调逻辑层与表现层解耦（三国杀难点在卡牌结算逻辑，与引擎无关）。

**关联计划：** 立项决策，无独立 Plan 文档。
