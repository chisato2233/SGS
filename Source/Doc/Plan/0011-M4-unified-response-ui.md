# 0011-M4 — 统一响应与技能询问 UI

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-12 |
| 最近更新 | 2026-07-12 |
| 关联需求 | `0000-RawRequirements.md` 中的第 26 条 |
| 父计划 | `0011-native-code-first-ui.md` |
| 关联代码 | `Source/SGS/Client/UI/Features/Table/`、`Source/SGS/Shared/Decisions/`、`Source/SGS/Shared/UI/` |

---

## 1. 目标

把当前简单 Confirm/Pass 条升级为统一决策面板，以同一套 Props 和 intent 支持出牌、响应指定卡牌、拒绝响应以及零卡牌技能询问。当前真实纵切覆盖杀→闪和濒死→桃；未来万箭/南蛮、无懈和响应技能只扩展服务器请求数据，不新增专用 Widget。

## 2. NoName 参考与约束

- NoName 将 `chooseToUse`、`chooseToRespond` 和技能入口统一为一个选择事件：Prompt、牌、目标、技能、Confirm/Cancel。
- `game.check()` 在每次选择后重新派生合法项和确认状态；技能通过临时选择模式复用同一事件，取消后恢复。
- SGS 保留该交互思想，但合法选项必须来自服务器权威请求；UI 不运行卡牌/技能规则。
- 继续使用 Native Slate、Controller→Props→Component 和 callback→Controller→Host 单向流，不创建全局事件总线。

## 3. 方案设计

- 响应请求补充上下文名与可用技能选项；技能选项可声明是否需要成本牌以及合法牌/目标集合。
- 私有快照携带纯展示 prompt、上下文座位和技能选项；Table State 保存当前卡牌、目标和技能模式，并负责归一化。
- 新决策面板由标题、主提示、上下文详情、技能按钮组、确认与不响应按钮组合；无请求时隐藏响应语义。
- Confirm 提交当前技能名、可选成本牌和目标；现有物理卡响应保持兼容。技能规则仍由服务端 Rule 校验，UI 选择不能直接改变权威状态。

## 4. 工程质量门

- 当前 Slash/Dodge 与 Dying/Peach 是真实消费者，不使用 fake prompt 验收。
- 请求替换、过期选择、技能消失、非法牌/目标和拒绝响应必须安全归一化或被 Agent 拒绝。
- WindowName、ContextName、SkillName 和 CardName 使用开放标识，不以封闭 enum 写死未来卡牌/技能。
- 本计划只建立响应交互与协议接缝；未实现的 AOE、无懈和武将技能结算不得标记为已完成玩法。

## 5. 任务拆解

- [x] 审查 NoName 的 choose/check/control/skill 交互模式。
- [x] 扩展权威响应请求、私有 ViewData 与本地选择状态。
- [x] 实现统一 Slate 决策面板及技能按钮模式。
- [x] 接入 Controller、HUD、Host RPC 与 LocalHumanDecisionAgent。
- [x] 完成 Development 构建、graphify 更新和运行验收交接。

## 6. 验收标准

- Slash.Dodge 显示来源、目标、所需【闪】，只能选择合法闪并确认或不响应。
- Dying.Peach 显示求桃上下文，只允许合法桃；请求结束后面板和选择状态同步清除。
- 结构化请求可显示任意 RequiredCardName/ContextName，因此万箭要求闪、南蛮要求杀和无懈询问无需新增 Widget。
- 技能选项能够切换选择模式、更新合法牌/目标并随 Confirm 上送 SkillName；无技能时不显示空技能条。
- Development Editor 编译通过；PIE 视觉读取仍须用户逐次确认。

## 7. 进度与决策记录

- 2026-07-12：完成 NoName 审查。保留其统一选择事件和动态技能/确认控制思想，不移植 DOM、全局状态或客户端规则判断。
- 2026-07-12：完成统一响应 UI 的生产实现。响应请求现可携带 ContextName 与技能选项；私有快照、Table State、Controller、HUD、RPC 和 LocalHumanDecisionAgent 已贯通；新增独立 DecisionPanel 组件并移除 Components 中的旧决策条实现。
- 2026-07-12：Development Editor 编译通过并更新主项目 graphify。当前真实玩法仍为杀→闪与濒死→桃；万箭/南蛮、无懈和具体武将响应技能需要后续服务端规则计划提供请求并校验结算，本计划等待 PIE 交互验收后收口。
