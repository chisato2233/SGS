# 0011 — Native Code-first UI 架构

| 字段 | 值 |
|---|---|
| 状态 | `Ready` |
| 创建日期 | 2026-06-19 |
| 最近更新 | 2026-07-12 |
| 关联需求 | `0000-RawRequirements.md` 中的第 5 条 |
| 关联代码 | `Source/SGS/Client/UI/`、`Source/SGS/Shared/UI/`、`Source/SGS/SGS.Build.cs` |
| 子计划 | `0011-M1-minimal-code-first-table-ui.md`、`0011-M2-react-inspired-ui-foundation-events.md`、`0011-M3-high-performance-hand-interaction.md`、`0011-M4-unified-response-ui.md` |

---

## 1. 目标

为 SGS 确定长期 UI 技术路线：采用 Unreal Native Code-first UI，不引入 WebView/React/Vue/Noesis/Gameface 作为主 UI，不自研完整浏览器级 UI runtime。

做完本 Plan 后，应形成一个可实施的 UI 架构边界：后续 UI 开发能以代码优先方式写出一致、现代、可维护的界面，同时不把项目拖入通用 UI 引擎开发。

## 2. 背景与约束

用户担心裸 Slate/UMG 会像古早前端一样僵硬、难看、迭代慢；也担心小型开源 Web UI 项目不够成熟，商业中间件又不适合当前个人学习项目。

因此本项目选择路线 B：

- 底层留在 Unreal 原生 UI 体系。
- UI 只显示状态、采集输入，不承载游戏规则。
- 不在编辑器 Designer 拖控件。
- 不自研 Gameface，不自研 HTML/CSS/JS runtime。
- 不把 UI 框架做成通用产品，只服务 SGS 的牌桌、手牌、玩家座位、菜单、调试面板等需求。

## 3. 方案设计

### 3.1 技术底座

- **Slate**：主要承载自定义控件、布局与绘制，使用 `S` 前缀组件。
- **UMG wrapper**：仅作为视口挂载、生命周期、与 UE 对象系统对接的薄适配层；如使用 `UWidget`/`UUserWidget`，必须代码拼装。
- **CommonUI**：作为可选项；只有当菜单栈、输入路由、手柄/键鼠提示等复杂度实际出现时再引入。

### 3.2 SGSUI 薄封装

SGSUI 不是通用 UI 引擎，只包含 SGS 需要的薄层：

- `Theme / Tokens`：颜色、字体、间距、圆角、阴影、动效时长。
- `Atomic Widgets`：按钮、标签、面板、图标按钮、计数器、提示条。
- `Game Widgets`：卡牌视图、手牌区、牌堆/弃牌堆、玩家座位、阶段指示、决策按钮组。
- `View Model`：从权威状态/复制状态生成 UI 快照，供 UI 读取。
- `Action Dispatch`：UI 发出动作意图，交给真人决策代理/RPC/逻辑入口处理。
- `Motion Presets`：淡入、滑入、缩放、卡牌移动等表现层动画预设。

### 3.3 现代 UI 体验的最低要求

- 每个界面必须使用统一 theme/token，不直接散落魔法颜色和魔法间距。
- 复杂界面拆成组件，不在一个 `NativeConstruct` 或 `Construct` 中堆完整页面。
- 状态变化通过 view model / attribute / explicit refresh 管理，不把业务逻辑写进控件。
- 动画只用于表现层，不影响结算时序。
- UI 代码不得反向依赖服务器权威逻辑内部实现。

### 3.4 备选方案与取舍

- **WebView + React/Vue**：现代前端体验强，但 Unreal 侧底座成熟度/性能/打包/输入焦点风险较高，且当前不是刚需。
- **NoesisGUI**：成熟度高，但引入 XAML 与商业许可，当前阶段先不采用。
- **RmlUi**：轻量且游戏友好，但仍偏 markup/CSS-like，不符合当前“纯代码优先”的取向。
- **自研 Gameface**：技术上诱人，但范围过大，会把项目从“做游戏 UI”拖成“做 UI 引擎”。

## 4. 任务拆解

### M1：最小代码优先牌桌 UI 纵切

详见 `0011-M1-minimal-code-first-table-ui.md`。M1 只做本地最小可操作牌桌 HUD，并依赖 `Archive/0005-basic-card-playable-rules.md` 的真实基础牌规则路径；静态假界面不能作为 M1 完成依据。

### M2：React-inspired UI Foundation 与表现事件框架

详见 `0011-M2-react-inspired-ui-foundation-events.md`。M2 建立与牌桌业务无关的 Slate 组件 Core，借鉴 React 的 props、callback、state ownership、context 与 composition；首要验收是拆分 M1 的 God Widget，并通过依赖护栏防止 Core 反向认识 Table / Game / Server。通用 State 与 typed signal 只按真实组件需求提炼，不引入 React runtime、虚拟 DOM、全局万能 Store 或浏览器式 Event Bus。

### M3：高性能手牌交互与弹性动画

详见 `0011-M3-high-performance-hand-interaction.md`。M3 在 Table Feature 内实现稳定 keyed 手牌组件树、本地展示顺序、Slate 原生拖拽和单 Active Timer 弹性动画；牌序只属于 LocalPlayer 表现状态，不进入服务器权威事实。Motion 暂不提升为通用 Foundation，待出现第二个真实动画消费者后再提炼。

### 后续 UI 任务池

- [ ] 在 UI 阶段开始时，按需为 `SGS.Build.cs` 追加 `UMG` / `Slate` / `SlateCore`。
- [ ] 建立 `Source/SGS/UI/` 的最小目录和命名规范。
- [ ] 定义第一版 SGS theme/token。
- [ ] 实现第一批 atomic widgets：按钮、面板、文本样式、图标按钮。
- [ ] 实现第一批 game widgets：卡牌视图、手牌区、玩家座位、阶段指示。
- [ ] 建立 UI view model 与 action dispatch 的最小桥接。
- [ ] 用一个可运行牌桌 HUD 验证布局、状态刷新、输入、动画与视觉一致性。
- [ ] 评估是否需要引入 CommonUI；若需要，单独记录原因和模块/插件变更。

## 5. 验收标准

- 至少有一个代码优先的牌桌 HUD 原型可运行。
- HUD 不含 Designer 拖控件资产。
- UI 通过 view model 读取状态，通过 action dispatch 发出操作意图。
- 牌桌 HUD 的颜色、字体、间距、按钮、卡牌样式来自统一 token/组件。
- UI 层不包含游戏规则结算逻辑。

## 6. 进度与决策记录

- 2026-06-19：用户确认选择路线 B。项目 UI 路线定为 Unreal Native Code-first + SGSUI 薄封装；不采用 WebView/React/Vue 作为主 UI，不自研完整 Gameface。
- 2026-06-28：拆出 `0011-M1-minimal-code-first-table-ui.md` 作为第一个 UI 实施子计划。M1 只验收真实规则状态驱动的最小可操作 HUD，依赖 Plan 0005，不接受静态假界面。
- 2026-07-11：M1 已按 NoName Nova 纠正摄像机、全视口背景、八人武将面板、本地 seat 0、压叠手牌底栏和控制区；离屏 1280x720 UI 截图与目标自动化通过。已导入武将图暂为表现占位，不进入规则事实源；M1 仍等待 PIE / Standalone 真实点击验收。
- 2026-07-11：新增 M2 详细执行计划。UI 长期路线进一步明确为“React 思想、Slate 实现”：Snapshot / Store / Selector / Props 单向下行，typed Action 经 adapter 上行，scoped UI Event Hub 只发布表现事实；以当前 M1 牌桌为真实迁移纵切，避免继续扩张单体 HUD Widget。
- 2026-07-11：按用户审查意见纠偏 M2。长期框架 Core 必须与牌桌 / 玩家 / 网络业务无关，组件职责与 Anti-God-Widget 成为第一优先级；Store、Dispatcher 与事件能力不再先行扩张，typed signal 仅服务真实横切 UI 通知。已有 Table Store 作为 feature 临时适配器保留，后续迁出 Foundation Core。
- 2026-07-11：M2.3-M2.6 生产代码完成：泛型 state/binding、typed signal/lifetime、每 LocalPlayer Context、Table Feature Controller、精准区域更新及真实 Toast/Focus 消费者已经接入。PlayerController 收缩为 Host/adapter，Core/Primitives 不认识 Table/Game/Server。Motion 等出现两个真实消费者后再提炼；CommonUI 与菜单栈必须按明确输入/导航需求另立计划。自动化与多 LocalPlayer 运行验收由独立测试模型继续。
- 2026-07-11：新增 M3 并开始实施真实手牌交互纵切。采用 Feature 内本地牌序、按 CardId 复用的稳定组件树、Slate 原生拖拽和单 Active Timer 弹簧动画；第一阶段保持“点击选择 + Confirm”，不扩张到拖牌出牌或通用 Motion 框架。
