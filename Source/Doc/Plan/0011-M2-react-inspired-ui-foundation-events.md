# 0011-M2 — React-inspired、业务无关的 UI 组件框架

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-11 |
| 最近更新 | 2026-07-11 |
| 关联需求 | `0000-RawRequirements.md` 中的第 21、22、23 条 |
| 父计划 | `0011-native-code-first-ui.md` |
| 首个迁移对象 | `0011-M1-minimal-code-first-table-ui.md` 的牌桌 HUD |
| 关联代码 | `Source/SGS/Client/UI/`、`Source/SGS/Shared/UI/`、`Source/SGS/Client/Game/`、`Source/SGS/Tests/` |

---

## 1. 目标

建立一套在 SGS 项目内**与牌桌、卡牌、玩家、网络和规则业务无关**的 UI 代码框架。框架借鉴 React 最有价值的工程思想，但继续使用 Slate 原生组件，不引入 React runtime、WebView、虚拟 DOM、JSX 或浏览器式渲染系统。

本计划的第一目标不是 Store 或 Event Bus，而是从结构上阻止新的 God Widget：

- 页面由小组件组合而成；每个组件只有一个主要变化原因。
- 数据通过不可变 props 向下传递；交互通过显式 callback / intent 向上传递。
- 状态由最近的共同 owner 持有，不让叶组件主动抓取全局状态。
- Theme、Assets、Lifecycle 等横切能力通过显式 Context / Scope 注入。
- 跨树 UI 通知使用生命周期安全的 typed signal；父子通信不绕进 Event Bus。
- 业务适配器留在 `Features/*` 或 `Host/*`，通用 Core 不认识 SGS 对局类型。

完成后，牌桌根 Widget 应退化为组件装配壳；新增装备区、技能区、弹窗、菜单或调试面板时，开发者可以复用统一的组件契约、状态绑定、生命周期和 UI 事件能力，而不把绘制、素材、布局、状态、输入和服务调用重新塞进一个类。

## 2. 资深工程审查结论

### 2.1 保留的高价值方向

- Slate Native Code-first，UMG 只做必要的 UObject / viewport 适配。
- 组件组合优于继承，不创建万能 Widget 基类。
- Props 向下、Callback / Intent 向上的单向数据流。
- 业务状态、局部 UI 状态和瞬时事件必须分离。
- Feature-first 目录，通用能力与牌桌实现分开。
- 生命周期安全的订阅句柄与 scope teardown。
- 先迁移真实 M1 牌桌，再用第二个真实 UI feature 验证复用。
- 构建与自动化优先；截图仍需用户逐次确认。

### 2.2 原计划中需要降级或删除的部分

- **删除“LocalPlayer Store 是 Foundation 核心”的假设**：LocalPlayer、viewer seat、Snapshot revision 都属于 Host / Table feature 策略，不属于通用组件框架。
- **删除“所有交互都进入 Action Dispatcher”的假设**：普通父子交互优先使用 callback；只有跨多层且确实需要统一结果语义的 feature intent 才使用 controller / dispatcher。
- **删除第一阶段的 GameplayTag Event Registry**：通用 UI 事件先以 C++ typed signal/channel 表达；只有出现动态路由需求时再评估 tag registry。
- **删除先做完整 Store / Event Hub、最后才拆 Widget 的顺序**：组件边界与依赖规则必须先落地，基础设施随真实拆分需求生长。
- **删除全套性能计数器的前置要求**：第一阶段只保留能证明“没有全树重复重建”和“没有订阅泄漏”的最小指标。
- **删除浏览器框架式 reconciliation 企图**：Slate 已经负责 Widget Tree、Invalidation 和绘制；本项目不重复实现虚拟 DOM。

### 2.3 已有 Table UI State Store 的定位修正

已经实现的 State Store 解决了真实牌桌的 revision guard、viewer 隔离和选择归一化；它是**Table feature 的业务状态适配器**，不是业务无关 UI Core。现已直接迁移并改名为 `FSGSTableUIStateStore`，未保留旧路径兼容层。

后续迁移要求：

- 通用 Core 不得 include `SGSTableViewTypes.h`。
- `FSGSTableUIStateStore` 固定归属 `Features/Table/State/`。
- 若它需要订阅、batch 或 selector，优先组合 Core 提供的通用状态原语，不把 Table 字段反向塞进 Core。
- 不允许在 `Foundation` 下恢复 Table 专用 Store 或转发兼容头。

## 3. React 思想在 Slate 中的正确映射

| React 思想 | SGS Slate 映射 | 不做什么 |
|---|---|---|
| Component | 小型 `SWidget` / `SCompoundWidget` 与组合函数 | 不创建 DOM 节点模型 |
| Props | 显式 props struct、Slate args、只读数据与 callback | 不让叶组件主动查全局对象 |
| Local State | 最近 owner 持有的轻量状态 | 不把所有状态塞进全局 Store |
| Lift State Up | Feature Controller / Container 持有共享状态 | 不靠广播同步兄弟组件 |
| Composition | Slots、child content、组合小组件 | 不靠多层继承复用 |
| Context | 显式传入的 Theme / Assets / UI Services context | 不使用无边界全局 singleton |
| Reducer / Intent | 复杂 feature 可选的 typed intent handler | 不强制每个按钮走 Dispatcher |
| Subscription | RAII handle + scope-owned typed signal | 不保存悬空 Widget callback |
| Render Update | props 变化后更新必要组件并使用 Slate invalidation | 不实现虚拟 DOM diff |

核心判断：如果一个机制只是为了“看起来像 React”，但没有减少依赖、变化原因或测试成本，则不进入框架。

## 4. 分层与依赖方向

```text
Client/UI/
  Core/
    Context/        # 通用 UI context、子 scope、依赖注入
    State/          # 可选的通用 observable state / selector / batch
    Events/         # typed signal、subscription handle、scope teardown
    Lifecycle/      # 与业务无关的 mount/unmount、owner lifetime
  Primitives/       # Button、Panel、Label、Badge、ImageFrame 等纯表现原语
  Theme/            # token、style、字体、间距、状态色
  Assets/           # 语义资源请求、Brush 生命周期与 fallback
  Features/
    Table/
      Components/   # Seat、Card、Hand、DecisionBar、CenterInfo
      State/        # Table snapshot、selection、selector
      Controller/   # Table intent 与组件编排
      Adapters/     # PlayerController / DecisionAgent / RPC 接缝
  Host/             # viewport、LocalPlayer、UObject/Slate 生命周期适配
```

硬依赖规则：

- `Core/` 不得依赖 `Features/`、`Client/Game/`、`Server/`、`SGSTableViewTypes` 或任何卡牌 / 玩家业务类型。
- `Primitives/` 只能依赖 Core、Theme、Assets 和 Slate 基础类型；不得访问 Store、PlayerController 或对局对象。
- `Features/Table/` 可以依赖 Core / Primitives / Theme / Assets 与 Table view data，但不得把 Table 类型暴露回 Core。
- `Host/` 负责把 UE 生命周期、LocalPlayer 和网络数据适配给 Feature；Feature component 不直接依赖 Host。
- 依赖只能向内：Host → Feature → Primitives/Core；禁止 Core 反向 include Feature。

通用框架的“业务无关”定义是：可以用整数、字符串或测试 fake state 验证全部 Core 能力，不需要启动世界、不需要 SGS 对局，也不需要构造 PlayerController。

## 5. 组件责任模型

### 5.1 四种角色

- **Primitive**：颜色、文字、边框、点击等纯表现能力；无业务语义。
- **Presentational Component**：接收 feature props，绘制一个可理解的 UI 单元，并通过 callback 上报交互。
- **Feature Controller / Container**：拥有共享状态、派生 props、处理 intent、组合组件。
- **Host Adapter**：负责 viewport、UObject、LocalPlayer、网络 / 规则边界和 feature mount/unmount。

一个类型只能有一个主要角色。角色组合必须有明确理由，并在该类型开始膨胀前拆分。

### 5.2 Anti-God-Widget 质量门

以下任一情况出现时，必须触发拆分审查，而不是继续向 Widget 添加 helper：

- 同时负责组件装配、素材加载、状态归一化、布局算法或外部服务调用中的两项以上。
- 同时持有业务状态源和直接服务器 / PlayerController 调用。
- 拥有多个互不相关的 `Build*` 区域或为多个视觉单元维护独立状态。
- `Tick` / ActiveTimer 同时承担数据同步和页面绘制重建。
- 修改一个叶视觉样式必须重新理解整个页面输入与服务器提交逻辑。
- 单元测试必须构造 World、PlayerController 或完整对局才能验证一个按钮 / 卡片的显示行为。

以下是审查信号，不是机械行数禁令：

- Widget 实现超过约 300 行。
- 超过 5 个私有 `Build*` / `Get*Brush` / 输入处理族函数。
- include 跨越 Core、Feature、Host、Server 中三个以上层级。
- 一个类有三个以上独立变化原因。

### 5.3 Props 与更新契约

- Presentational component 的全部业务输入集中在紧凑 props 中，props 默认只读。
- props 只包含绘制所需的最小 slice，不把整张 Snapshot 传给所有叶组件。
- callback 使用语义明确的参数，不把 PlayerController 或 Store 指针传进叶组件。
- props 更新只 invalidates 必要 Widget；稳定 props 不触发全页面重建。
- 列表项必须使用稳定业务 key，由 feature 生成；Core 不理解 key 的业务含义。
- Component 构造与 props 更新必须能在无 World 环境下测试。

### 5.4 Context 与依赖注入

Core Context 只允许包含通用横切能力：

- Theme / style access。
- Asset resolver。
- UI lifetime scope。
- Typed UI signal service。
- 可选的调试 / invalidation instrumentation。

对局快照、PlayerController、DecisionAgent、GameDriver 和网络接口不得进入 Core Context。Feature 可在自己的 Controller / Adapter 中组合业务服务。

## 6. 状态与单向数据流

框架支持状态能力，但不要求所有页面使用全局 Store：

```text
Host / Feature Adapter
        ↓ input state
Feature Controller
        ↓ minimal props
Component Tree
        ↑ callbacks / intents
Feature Controller
        ↑ external result
Host / Feature Adapter
```

状态放置规则：

- 单组件使用的短期状态留在组件或最近 owner。
- 多个兄弟组件共享的状态提升到最近 Feature Controller。
- 跨页面持久状态由 Host / Screen owner 管理。
- 通用 Core 只提供 observable value、derived selector、batch 和 subscription 等机制，不定义 Snapshot、revision 或 selection 业务策略。
- Table revision guard、viewer 隔离和合法选择归一化继续属于 Table State。

Feature intent 的处理优先级：

1. 父子之间使用显式 callback。
2. 多层组件共享行为由 Feature Controller 暴露 typed intent handler。
3. 只有需要统一排队、结果或跨 adapter 时才引入 feature dispatcher。
4. Event channel 永远不用于寻找 intent handler。

## 7. 业务无关的 UI 事件框架

事件框架保留，但缩减为解决 UI 组件树横切通信的最小能力。

### 7.1 第一版只提供

- `TypedSignal<Payload>` 或等价强类型 channel。
- RAII subscription handle。
- 通用 lifetime scope：scope 销毁时自动解绑全部 subscription。
- 弱 owner / 安全 callback，不调用已销毁 Widget。
- 游戏线程发布语义。
- 开发模式下的最小诊断：channel 类型、scope、subscriber count、被丢弃原因。

### 7.2 第一版不提供

- 全局字符串 / GameplayTag 事件注册表。
- Event replay、持久化、网络复制或规则日志。
- 优先级工作流、返回值聚合或“第一个 handler 胜出”。
- 为每个点击创建 Event。
- 浏览器式全局 Event Bus。
- 未出现真实消费者前的复杂嵌套队列、事件风暴调度器或跨线程总线。

合法用途：Toast、Modal 请求、焦点、动画 / 音效提示、跨树 UI 生命周期通知。父子交互、状态同步和规则命令不是 Event。

若真实接入证明需要嵌套发布保护或动态 tag 路由，再通过后续小里程碑加入；不能在没有消费者前预建。

## 8. 分阶段执行计划

### M2.0 — 基线与方向纠偏

- [x] 用 M1 自动化冻结八人 HUD、私有手牌与 Slash / Dodge / Peach / Pass 路径。
- [x] 确认当前 God Widget 的变化原因：装配、状态、素材、布局、输入和外部提交。
- [x] 明确 Props、Callback/Intent、State、Context、Signal 的边界。
- [x] 将计划从“业务 Store / Event Hub 优先”纠偏为“组件与依赖边界优先”。
- [x] 将原 `FSGSLocalUIStateStore` 定位为 Table feature 适配器。

退出条件：通用 Core 与 Table 业务的依赖方向无歧义；后续实现不再向旧 Widget 添加新的业务区域。

### M2.1 — Core 契约与架构护栏

- [x] 建立业务无关的 Context、lifetime scope 与 subscription handle 最小契约。
- [x] 为 props / component / controller / host 制定命名和 include 规则，不创建万能基类。
- [x] 固化轻量依赖护栏并审计当前源码：Core / Primitives 不得 include Table、Game、Server 类型。
- [ ] 由独立测试模型用 fake payload / fake state 验证 Core；本轮按用户指示不开发测试代码。
- [x] 将 Store 改名为 `FSGSTableUIStateStore` 并迁移到 Table feature 边界；删除旧实现和旧 include 路径。

退出条件：Core 可以脱离牌桌和 World 编译测试；依赖违规能在评审或自动化中被发现。

### M2.2 — 优先拆分现有 God Widget

- [x] 定义并实现 Card、Seat、Hand、DecisionBar、CenterInfo 的紧凑 props 与 callbacks。
- [x] 将每个叶组件迁移为无 PlayerController、无 Store、无素材加载策略的 presentational component。
- [x] 将 `SSGSTableHudWidget` 收缩为 Table Container，负责 Snapshot slice、选择状态与 intent 处理。
- [x] 建立 `SSGSTableShellWidget`，只组合背景、布局区域和子组件。
- [x] 将素材解析移入 `FSGSTableAssetCatalog`，将纯坐标算法留在 Layout 层。
- [ ] 由独立测试模型运行 M1 自动化与交互回归；本轮按用户指示只执行 Development 编译校验。

退出条件：旧 `SSGSTableHudWidget` 不再绘制 Seat / Card / Hand / Controls 细节，不再直接加载它们的素材，也不再直接处理它们的局部选择规则。

### M2.3 — 按真实需求提炼通用 State / Binding

- [x] 从 Table Controller 的实际状态切片提炼 observable value、selector、batch 和 equality 策略。
- [x] Core state 原语只处理泛型值与通知，不理解 revision、viewer、prompt、card 或 seat。
- [x] Table State 组合通用原语，继续负责 revision guard 与选择合法性。
- [x] 用 public/private/interaction 精准通知替换固定频率全树重建；只有布局变化重排整张牌桌。
- [ ] 由独立测试模型覆盖 batch、selector equality、unsubscribe 和 teardown；本轮不开发或运行测试。

退出条件：状态原语至少被两个 Table 子区域真实使用；没有为未来假设添加未使用 API。

### M2.4 — Typed Signal 与 UI Lifetime

- [x] 实现 typed signal、RAII subscription 和 scope teardown。
- [x] 接入两个真实横切用途：LocalPlayer Toast 与 Table Confirm 焦点请求。
- [x] 普通 Card / Seat / Confirm / Pass 交互继续使用显式 callback，不迁入 Event Bus。
- [x] 生产实现包含 weak owner、scope teardown、零订阅者安全发布和错误线程拒绝语义；测试验证交由独立模型。
- [x] 当前无嵌套发布队列的真实证据，因此明确不实现。

退出条件：事件框架与 Table 业务类型无关；移除所有 signal subscriber 不改变 feature state 和规则结果。

### M2.5 — Host / Feature 边界收口

- [x] PlayerController / viewport 只负责 mount、unmount 与外部状态 / intent adapter。
- [x] `FSGSTableFeatureController` 成为组件树唯一业务协调入口。
- [x] 叶组件和 Primitives 不直接调用 PlayerController、DecisionAgent、GameDriver 或 RPC。
- [x] 消除旧 HUD 的 fixed polling、SnapshotProvider、PlayerController 和双重状态字段。
- [x] 每个 PlayerController 独立创建 Feature Controller、UI Context、Toast 与 lifetime；多 LocalPlayer 运行验证交由独立测试模型。

退出条件：Host、Feature、Core 的依赖方向符合第 4 节；牌桌真实操作链保持通过。

### M2.6 — 第二消费者与规则固化

- [x] 使用真实游戏内 Toast 作为第二 feature，并与 Table 共享每 LocalPlayer typed signal context。
- [x] 审查通用 API，仅保留 observable/selector/batch、lifetime、typed signal、context 与真实 Focus primitive。
- [x] 将稳定的 Anti-God-Widget、Props、Context、State 与 Signal 规则提升到 `Rules/UI.md`。
- [x] 更新父计划、ProjectBrief 与 graphify，记录 Motion / CommonUI / 菜单栈等后续边界。

退出条件：Core / Primitives 至少服务两个真实 feature；新增 feature 不需要修改 Core 才能组合页面。

## 9. 验收标准

必须全部满足：

- Development Editor 构建通过，M1 与 M2 自动化通过，graphify 更新成功。
- `Core/` 与 `Primitives/` 不依赖 Table、PlayerController、GameDriver、Server 或任何具体卡牌 / 玩家类型。
- Core 测试只使用 fake props、fake state 和 fake event payload，无需 World。
- Table 页面由 Shell、Controller 和独立叶组件组合；根 Widget 不再同时承担装配、绘制、素材、状态和服务提交。
- Seat、Card、Hand、DecisionBar、CenterInfo 可以独立构造和验证 props / callback。
- 业务数据通过 Feature Controller 生成最小 props；叶组件不主动读取全局 Store。
- 普通父子交互使用 callback；Event framework 只服务横切 UI 通知。
- typed signal 在 owner / scope 销毁后不会调用悬空回调。
- 稳定 props 不触发无关组件重建；不再依赖固定频率全树刷新。
- 现有真实 Slash / Dodge / Peach / Pass、私有手牌与八人布局行为保持不变。
- 至少两个真实 feature 复用 Core / Primitives / Signal 中的一部分。
- UI 不包含 Designer 主界面、虚拟 DOM、浏览器 runtime、全局万能 Store 或第二套规则事件系统。
- 若需要读取 UE 画面，必须先取得用户对每次读取的明确确认；截图不是架构完成的唯一证据。

## 10. 风险与控制

| 风险 | 控制 |
|---|---|
| 把 React 名词机械移植到 Slate | 只保留能减少依赖和变化原因的思想 |
| 框架 Core 被牌桌需求污染 | 编译 / include 护栏，Table 类型只存在于 Feature |
| Event Bus 成为隐藏依赖 | callback 优先；Signal 只允许横切通知 |
| Store 成为所有状态的默认答案 | 状态放在最近 owner；Core state 原语可选 |
| 万能基类重新制造耦合 | 组合、props、context 和 helper 优先，不要求统一继承 |
| 先造框架、迟迟不拆 Widget | M2.2 提前为主里程碑，后续原语从真实拆分提炼 |
| 大爆炸迁移破坏 M1 | 一次迁移一个叶组件，每步运行既有自动化 |
| 只按行数拆文件、职责仍耦合 | 以变化原因和依赖方向验收，不以文件数量验收 |

## 11. 进度与决策记录

- 2026-07-11：创建 M2，原方向为 Snapshot / Store / Selector / Action / Event 后再拆牌桌组件。
- 2026-07-11：完成第一段实现。当时新增 `FSGSLocalUIStateStore`，接入 revision guard、viewer 隔离和选择归一化；Development 构建、StateStore 与 M1 DecisionBridge 通过（测试现已随迁移改名为 `SGS.Plan0011M2.Table.StateStore`）。
- 2026-07-11：根据用户审查意见重构计划。最终目标改为“业务无关的组件框架 + 防止 God Widget”；组件契约和牌桌拆分前置，完整 Store / Dispatcher / GameplayTag Event Hub 降级。当时决定将旧 Store 迁出 `Foundation`，该迁移已在下一阶段完成。
- 2026-07-11：完成 M2.2 首轮结构重构。Store 改名为 `FSGSTableUIStateStore` 并迁入 Table feature，旧 Foundation 文件直接删除；新增紧凑展示 props、Card / Seat / Hand / DecisionBar / CenterInfo 叶组件、纯组合 `SSGSTableShellWidget` 与 `FSGSTableAssetCatalog`。`SSGSTableHudWidget` 仅保留快照协调、状态/intent 和 props 组装；Development Editor 编译通过，自动化回归留给独立测试模型。
- 2026-07-11：完成 M2.3-M2.6 生产实现。Core 新增泛型 observable/selector/batch、RAII subscription/lifetime、typed signal 与每 LocalPlayer Context；Table Store 组合这些原语，HUD 删除 0.1 秒全树轮询并按 public/private/interaction slice 精准更新。新增 `FSGSTableFeatureController` 收口业务协调，PlayerController 仅保留挂载与外部 adapter；真实 Toast feature 和 Focus primitive 验证第二消费者与两类横切 signal。Development Editor 编译通过；按用户要求未运行自动化，剩余测试验收交由独立测试模型，因此 Plan 暂维持 In Progress。
