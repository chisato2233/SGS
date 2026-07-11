# UI Rules

本文件规定 UI 技术路线和边界，不描述现有控件或界面结构。需要 UI 代码关系时使用 graphify。

---

## 技术路线

- UI 主路线为 Native Code-first UI。
- Slate 作为主要自定义控件底座。
- UMG 只作为视口、生命周期和对象系统适配层。
- CommonUI 按需评估，用于菜单栈、输入路由、手柄/键鼠提示等明确需求。

---

## 禁止事项

- 不在编辑器 Designer 中拖控件搭主 UI。
- 不以 WebView、React、Vue、Noesis、Gameface 作为主 UI 技术栈。
- 不自研浏览器级 UI runtime，除非新 Plan 明确推翻当前路线。
- UI 不承载规则结算。

---

## 边界

- UI 读取复制状态、规则事件和可显示 ViewModel。
- UI 采集玩家意图，并通过合法命令/RPC 通道提交。
- UI 不直接修改权威规则状态、索引事实源、效果管线或其他规则事实源。

---

## 分层与依赖方向

- `Core` 只提供业务无关的 context、state、signal 和 lifetime 能力；禁止 include `Features`、具体 Game/Server 类型或任何牌桌、卡牌、玩家类型。
- `Primitives` 只组合 Slate 与 Core 横切能力；不得读取业务 Store 或调用 RPC。
- `Features/<FeatureName>` 持有该功能的 props、组件、状态策略、Controller 和素材适配；Feature 可以依赖 Core / Primitives，反向依赖禁止。
- Host 负责 LocalPlayer/viewport 的 mount、unmount、复制状态 adapter 和外部 intent adapter；Host 不绘制 feature 细节。
- Feature Controller 是组件树的唯一业务协调入口；叶组件不得持有 PlayerController、DecisionAgent、GameDriver 或网络服务。

## 组件与 Props

- 复杂页面必须按 Container / Controller / Shell / Presentational Component 的变化原因拆分，不创建万能 Widget 基类。
- Presentational Component 的业务输入必须集中在紧凑、只读 props 中；禁止向叶组件传整张无关 Snapshot 或全局 Store。
- 父子交互优先使用强类型 callback；callback 参数表达 intent，不暴露 Host 或服务指针。
- Shell 只负责组合和布局区域，不负责规则判断、资源路径解析或外部提交。
- 同一 feature 的小型相关组件允许共置文件；一旦出现独立变化原因、依赖或复用入口，应拆为独立组件，禁止用文件数量替代职责判断。

## State 与更新

- 单组件短期状态留在最近 owner；兄弟组件共享状态提升到 Feature Controller；跨页面状态由 Host/Screen owner 管理。
- Core observable、selector、batch 和 equality 策略必须保持泛型，不理解 revision、viewer、prompt、card 或 seat。
- 业务合法性和归一化留在 Feature State；禁止把业务字段反向塞进 Core。
- 状态更新必须按稳定 slice 精准通知并只重建相关区域；禁止使用固定频率全树轮询作为常态刷新机制。
- 每个 LocalPlayer 必须拥有独立的 Feature Controller、UI Context 与 lifetime scope，禁止进程级 UI 单例保存私有状态。

## Typed Signal 与生命周期

- UI signal 只服务 Toast、Modal、焦点、动画/音效提示等横切表现通知；普通点击、状态同步和规则命令不得迁入 Event Bus。
- Signal 必须强类型、游戏线程发布，并通过 RAII subscription + lifetime scope 自动解绑；Widget 回调必须使用弱 owner。
- 禁止全局字符串/GameplayTag 事件注册表、事件 replay、网络复制、优先级工作流或返回值聚合。
- 未出现真实消费者前，不实现嵌套发布队列、事件风暴调度器或跨线程总线。

## 后续能力准入

- Motion 只作为表现预设加入，不影响规则时序；出现至少两个真实动画消费者后再提炼通用 API。
- CommonUI 仅在菜单栈、输入路由、手柄/键鼠提示形成明确需求时单独立项，不因已有 UI Core 自动引入。
- 菜单/Screen 栈属于后续 Host 层能力，不得塞入 Table Feature 或 typed signal。
