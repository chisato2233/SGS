# 0011-M1 — 最小代码优先牌桌 UI 纵切

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-06-28 |
| 最近更新 | 2026-06-28 |
| 关联需求 | `0000-RawRequirements.md` 中的第 12 条 |
| 父计划 | `0011-native-code-first-ui.md` |
| 前置计划 | `Archive/0005-basic-card-playable-rules.md` 的真实规则路径 |
| 关联代码 | `Source/SGS/UI/`（新增）、`Source/SGS/Game/`、`Source/SGS/Logic/Players/`、`Source/SGS/Logic/Commands/` |

---

## 1. 目标

在 Plan 0011 的 Native Code-first UI 路线下，实现第一个**本地最小可操作牌桌 HUD**。

做完后，开发者在 PIE / Standalone 中能看到真实对局状态，并用鼠标完成最小基础牌操作：

1. 查看座位、当前阶段、当前行动者、体力、手牌数、牌堆 / 弃牌堆数量。
2. 查看本地玩家自己的手牌。
3. 在出牌阶段选择 `Slash`、选择合法目标、提交命令。
4. 在响应窗口中选择 `Dodge` / `Peach` 或 Pass。
5. UI 行为通过真实 `ISGSDecisionAgent` / `FSGSCommand` / `FSGSCommandRouter` 进入服务器权威规则层。

本计划不是完整 UI 系统，不追求美术完成度；但它必须建立可继续扩展的 UI 地基，不允许使用假数据、假按钮或绕过规则层的演示逻辑。

---

## 2. 背景与约束

父计划 `0011-native-code-first-ui.md` 已确定长期 UI 路线：

- Slate / UMG 原生代码优先。
- UMG 只作为视口挂载、生命周期和对象系统适配层。
- UI 只显示状态、采集输入，不承载规则结算。
- 不使用 Designer 拖控件，不使用 WebView / React / Vue / Noesis / Gameface 作为主 UI。

本 M1 的前置条件是 `0005` 至少完成基础牌真实规则路径。若 `0005` 尚未完成，M1 可以先搭建 tokens、ViewModel 和静态布局，但不能标记 Done；最终验收必须接入 `0005` 的真实对局状态和命令路径。

非目标：

- 不做完整大厅、菜单、联机房间、账号、匹配。
- 不做完整动画系统、皮肤系统、响应式布局全集。
- 不做正式网络 RPC；本地真人决策桥必须能被后续 RPC 代理替换。
- 不把 UI 写成通用 UI 引擎或可复用产品。

---

## 3. 方案设计

### 3.1 模块与挂载方式

按需为 `SGS.Build.cs` 增加 UI 依赖：

- `Slate`
- `SlateCore`
- `UMG`

建立 `Source/SGS/UI/`，但只放 SGS 需要的最小层：

- `Theme`：颜色、字体、间距、圆角、状态色 token。
- `ViewModel`：从权威对局状态生成可显示快照。
- `Widgets`：牌桌、座位、手牌、阶段、决策按钮。
- `Bridge`：本地真人决策代理与 UI action dispatch。

HUD 挂载可以采用代码创建的 UMG wrapper 或 PlayerController / HUD 中直接创建 Slate。无论采用哪种方式，都不能依赖 Designer 资产作为主 UI。

### 3.2 ViewModel

新增最小快照结构，例如：

- `FSGSTableViewSnapshot`
- `FSGSSeatViewData`
- `FSGSCardViewData`
- `FSGSDecisionPromptViewData`

ViewModel 只做展示转换：

- 公共状态：座位、体力、存活 / 濒死 / 死亡状态、阶段、当前行动者、牌堆 / 弃牌堆数量。
- 私有状态：本地玩家手牌，只通过拥有者视角读取。
- 决策提示：当前可操作牌、候选目标、是否允许 Pass、当前响应窗口名称。

ViewModel 不执行规则校验的最终裁决。它可以提供 UI 提示，但服务器侧 `CommandRouter` 必须重新校验。

### 3.3 本地真人决策桥

新增本地 UI 决策代理，作为 `ISGSDecisionAgent` 的真人实现原型：

- 服务器逻辑请求决策时，代理保存 pending request，并广播给 UI。
- UI 选择牌 / 目标 / Pass 后，代理组装 `FSGSCommand` 并调用原回调。
- 代理必须处理过期 request、重复提交、取消、非法空选择、UI 未挂载等失败路径。
- `SourceChannel` 使用明确值，例如 `LocalUI`，便于日志、回放和未来 RPC 替换。

这不是网络层；未来网络 Plan 可以把同样的 pending request 映射到 PlayerController / RPC，而不改变规则层。

### 3.4 Widget 组成

最小牌桌 HUD 包含：

- 四个座位区：名称 / 座位号、体力、手牌数、状态标记。
- 中央信息区：当前阶段、当前行动者、牌堆 / 弃牌堆数量、最近事件摘要。
- 手牌区：本地玩家手牌卡片按钮，显示牌名、花色、点数。
- 决策区：确认、Pass、取消选择；响应窗口显示需要 `Dodge` / `Peach` 的提示。
- 目标选择：可选目标高亮，不可选目标禁用或无交互。

控件必须使用统一 token，不散落魔法颜色 / 魔法间距。卡牌和按钮可以简洁，但必须稳定、可读、可扩展。

### 3.5 与 0005 的接入

M1 的 UI 不维护独立 demo state。所有显示来自真实对局：

- 牌区与手牌来自 `USGSGameContext` / ViewModel 快照。
- 当前阶段与事件来自 `USGSGameDriver` / 事件总线。
- 可操作动作来自 pending decision request 与 TargetQuery / Command hint。
- 提交动作进入 `FSGSCommandRouter`，执行结果通过事件 / 快照刷新回 UI。

若为了早期开发需要静态布局预览，只能放在隔离的 developer preview 路径，不能作为验收依据，也不能进入正式 GameMode 默认路径。

---

## 4. 工程质量门

- **真实使用入口**：本地玩家在 PIE 中通过 HUD 完成 Plan 0005 的基础牌操作。
- **既有架构接入**：UI -> LocalHumanDecisionAgent -> `FSGSCommand` -> `FSGSCommandRouter` -> `FSGSEffectPipeline` -> `USGSGameContext`。UI 不直接改 Context。
- **关键不变量**：
  - UI 只能看到本地玩家有权限看到的手牌。
  - 没有 pending decision 时，确认 / Pass 不得提交 Command。
  - pending request 结束后，迟到点击必须被忽略并清空选择。
  - UI 目标高亮只是提示，不是服务器合法性的来源。
  - UI 刷新不能改变规则时序，动画不得驱动结算。
- **失败路径**：
  - UI 未挂载：本地真人代理应 fallback 或报告可诊断错误，不能让游戏线程卡死。
  - 非法选择：UI 给出不可提交状态；若仍提交，服务器拒绝且 UI 清晰回到可恢复状态。
  - 对局结束 / 角色死亡：清空 pending prompt，禁用无效操作。
- **未来扩展**：
  - 后续网络 RPC 只替换决策桥的传输层，不推翻 ViewModel / Widget / Command 边界。
  - 后续装备、锦囊、武将技在 UI 层表现为更多 prompt / action hint，而不是在控件里写规则分支。
- **不可接受的实现**：
  - UI 按钮直接调用 `ApplyDamage` / `Heal` / `MoveCards`。
  - 另造一套 UI-only 对局模型或假手牌。
  - 只做静态界面截图就标记完成。
  - 使用 Designer 拖控件搭主 HUD。

---

## 5. 任务拆解

- [x] 按需增加 `Slate` / `SlateCore` / `UMG` 模块依赖，并保持构建脚本通过。
- [x] 建立 `Source/SGS/UI/` 最小目录和命名规范。
- [x] 定义第一版 SGS UI tokens：颜色、字体、间距、状态色、卡牌尺寸。
- [x] 定义 `FSGSTableViewSnapshot` 等最小 ViewModel，覆盖座位、阶段、牌堆、手牌、决策提示。
- [x] 实现本地真人决策代理，将 `ISGSDecisionAgent` pending request 暴露给 UI，并接收 UI action dispatch。
- [x] 实现最小 Slate widgets：牌桌根、座位视图、手牌卡片、阶段信息、决策按钮、事件摘要。
- [x] 将 HUD 挂到本地 PlayerController / viewport，并让默认开发路径可选择 1 真人 + AI 座位。
- [x] 接入 Plan 0005 的 `Slash` / `Dodge` / `Peach` 决策提示与 Command 提交。
- [x] 增加调试日志或轻量自动化，确认 UI 提交的 Command 进入 `CommandRouter`，且 SourceChannel 为 `LocalUI`。
- [ ] 在 PIE / Standalone 完成手工验收记录，并更新父 Plan 0011 与 ProjectBrief 状态。

---

## 6. 验收标准

必须全部满足：

- Development Editor 编译通过。
- PIE 或 Standalone 启动后，显示最小牌桌 HUD：四个座位、体力、阶段、当前行动者、牌堆 / 弃牌堆、本地手牌。
- 本地玩家在出牌阶段可以选择一张真实手牌 `Slash`，选择合法目标并确认；日志显示 `LocalUI` Command 进入 `FSGSCommandRouter`，状态通过规则层改变。
- 本地玩家在被 `Slash` 指定时，若手牌有 `Dodge`，可以点击响应；若不响应，真实体力减少。
- 本地玩家或 AI 进入濒死 / 求桃窗口时，`Peach` 可通过 UI 或 AI 决策桥救回；UI 状态随真实结果刷新。
- UI 不含 Designer 主控件资产，不含独立 fake game state，不直接修改 `USGSGameContext`。
- 控件在常见桌面窗口大小下不重叠、不遮挡关键操作；文字可读，按钮状态清晰。

---

## 7. 进度与决策记录

- 2026-06-28：创建 M1 子计划。定位为 Plan 0011 的第一个可操作 UI 纵切，依赖 Plan 0005 的真实基础牌规则，不接受静态假界面作为 Done。
- 2026-06-28：完成代码实现与自动化桥接验证。新增 `USGSLocalHumanDecisionAgent`、`FSGSTableViewModel`、`SSGSTableHudWidget`、`ASGSPlayerController` 和 `FSGSUITheme`；本地开发路径 seat 0 使用 LocalHuman，命令行 / 无人值守路径仍使用 AI。`Development` 构建通过；`SGS.Plan0011M1.LocalUI.DecisionBridge` 自动化测试 `Success`，日志确认 `LocalUI` 提交 UseCard / Pass / RespondCard。
- 2026-06-28：补强 HUD 质量门。座位、手牌、操作按钮改为 theme token 驱动的稳定尺寸；自动化测试实际构造 `SSGSTableHudWidget` 并验证非零期望尺寸；非 `Unattended` 的 `-game -NullRHI` smoke 确认默认本地入口走 `LocalHuman=true`。剩余验收项是 PIE / Standalone 下的人工视觉与点击操作确认。
- 2026-07-04：参考 `QSgsRef/NoName` 的 `nova` 八人布局，将 HUD 从纵向调试列表改为 `ol_bg` 背景上的 Slate 原生牌桌布局。新增布局计算层，seat 0 固定主视角底栏，其他玩家按 Nova 边缘公式排布；`SGS.Plan0011M1.LocalUI.DecisionBridge` 自动化补充 8 人 1920x1080 / 1280x720 不重叠断言并通过。PIE 视觉与点击验收仍待人工确认。
