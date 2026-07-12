# 0011-M3 — 高性能手牌交互与弹性动画

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-11 |
| 最近更新 | 2026-07-12 |
| 关联需求 | `0000-RawRequirements.md` 中的第 25 条 |
| 父计划 | `0011-native-code-first-ui.md` |
| 关联代码 | `Source/SGS/Client/UI/Features/Table/`、`Source/SGS/Client/UI/Widgets/` |

---

## 1. 目标

在现有 Native Slate、单向数据流和 Table Feature 边界内，为真实玩家手牌实现低延迟拖拽、本地重排和自然弹性动画，同时保留点击选牌与 Confirm 出牌路径。

完成后必须达到：手牌组件树按 `CardId` 稳定复用；拖拽期间不逐帧重建 Widget 或写 Store；一组手牌只由一个按需 Active Timer 驱动；本地牌序能在权威快照增删牌时正确调和。

## 2. 背景与约束

- 当前牌桌已经完成 M2 的 Controller / Shell / Presentational Component 拆分，但手牌更新仍可能替换整棵子树，无法承载持续拖拽动画。
- 牌序是当前 LocalPlayer 的表现偏好，不是服务器权威规则事实，不发送 RPC，也不改写权威快照。
- 复用 Unreal 原生 `FReply::DetectDrag`、`FDragDropOperation`、`FMath::SpringDamperSmoothing`、`FCurveSequence` 和 Active Timer；不引入 UMG 拖拽层、CommonUI、第三方动效框架或逐卡 Tick。
- `104×132` 卡牌尺寸是本计划开始时的既有基线，不在本计划重复调整。
- 第一阶段只做鼠标手牌重排；拖牌出牌/选目标、触摸、手柄、声音和粒子明确不在范围内。

## 3. 方案设计

### 3.1 本地展示状态与单向数据流

- Table Feature State 新增 `FSGSTableHandPresentationState`，只保存 `OrderedCardIds`；Controller 暴露只读 observable，并提供经过校验的 `ReorderHand` intent。
- 新快照到达时依次保留仍存在的本地顺序、删除失去的牌、按快照顺序追加新牌；重复、缺失或不属于当前手牌的 ID 不得写入状态。
- Shell props 按展示顺序生成。Hand 通过显式强类型 callback 将最终顺序提交给 Controller；普通父子交互不进入 typed signal。
- 为展示牌序设置独立更新 slice，只刷新 Hand，不触发无关座位、中央区或控制区重建。

### 3.2 稳定组件树与原生拖拽

- Shell 长期持有同一个 Hand Widget，并通过 `SetProps` 更新；Hand 以 `CardId` 保存稳定 Card Widget、Canvas Slot 与运动状态，只在卡牌集合真实增删时改变子节点。
- 卡牌组件自身区分点击和拖拽：左键按下请求 DetectDrag；未越过阈值时松开走原有点击选择；越过阈值后创建仅承载 CardId 和弱 owner 的 drag operation；抓取偏移、当前/预览顺序由 Hand model 持有。
- 原槽位卡牌在拖拽期间保留为透明占位；可见牌由 windowless drag decorator 在当前 Slate 窗口最终绘制层渲染，因此不受 Hand/ScrollBox 裁剪。`OnDragOver` 只更新预览顺序；手牌区内 Drop 才提交一次 Store，区外松开或取消只回位。
- 所有手牌都可参与展示重排；`bSelectable` 只限制点击选牌，不能通过禁用整个 Widget 阻断拖拽。

### 3.3 动画与性能模型

- 每张牌维护当前位置/目标位置、速度、缩放、旋转、槽位与 Hover/Selected/Dragging/Settling 标志；逐帧只更新 Render Transform，ZOrder 只在拖拽开始/结束、选择或最终提交时改变。
- 被拖牌采用约 `0.04s` 临界阻尼跟随鼠标；相邻牌采用约 `0.10s`、阻尼比约 `0.9` 的弹簧让位；插入索引按槽位中心计算，并使用约 `0.2 × CardStride` 的迟滞避免边界抖动。
- Hover 约放大至 `1.04` 并轻微上浮；取消/区外松开时位置弹簧回归，缩放与旋转以约 `0.18s` CubicOut 曲线复位。
- Hand 最多持有一个 Active Timer，只在拖拽或尚未稳定时运行，全部误差/速度低于阈值后立即停止；Idle 不保留 Tick、Timer 或 volatile 状态。
- 手牌溢出时保留水平 ScrollBox；拖到左右边缘时由同一 Active Timer 按边缘穿透深度自动滚动。

### 3.4 失败路径与快照竞争

- Esc、区外松开、鼠标捕获丢失和窗口失焦通过 Slate drag operation 的 cancel/drop 生命周期归一为未提交结束；operation 使用弱 owner，Hand 销毁后不会回调悬空 Widget。
- 被拖牌在快照中消失时立即取消；新增牌在保留现有本地顺序后追加，不打断仍合法的拖拽。
- 重排 intent 必须校验完整集合；非法顺序保持上一状态，并通过现有项目诊断方式暴露，不部分应用。
- 布局/视口重建导致 Hand 销毁时，drag operation 只持有弱引用，不能回调悬空 Widget。

## 4. 工程质量门

- 真实入口是本地玩家牌桌手牌，不使用 fake hand 或平行演示 Widget 验收。
- UI 只保存表现顺序和运动状态；规则合法性、卡牌所有权、出牌结算仍由既有权威路径负责。
- 拖拽每帧不得写 observable/Store、创建 Card Widget、重排子节点或触发全 Shell 重建。
- 动效保持在 Table Feature 内；出现至少第二个真实消费者后，才评估提炼业务无关 Motion API。
- 不新增大规模测试框架或测试代码；生产实现优先，运行回归由独立测试模型承接。

## 5. 任务拆解

- [x] 固化 M3 范围、状态边界、拖拽语义和性能不变量。
- [x] 增加本地手牌展示状态、快照调和、重排 intent 与精准更新 slice。
- [x] 将 Hand 改为按 CardId 复用的稳定组件树，并保持点击选择兼容。
- [x] 接入原生 drag operation、预览插入、取消/Drop 语义与边缘自动滚动。
- [x] 接入单 Active Timer、弹簧位移和曲线缩放/旋转动画。
- [x] 完成 Development Editor 构建、graphify 更新及独立运行验收交接。

## 6. 验收标准

- 单击仍可选择/取消并经 Confirm 执行动作；不可选择牌仍可拖动排序。
- 快速跨越多张牌时被拖牌跟手，相邻牌连续让位，不闪跳、不重复提交。
- 区内 Drop 后顺序稳定并跨后续增删牌快照保留；区外 Drop、Esc、失焦与被移除牌均安全回位/取消。
- 20 张以上手牌、滚动状态及不同 16:9 分辨率下插入位置与边缘自动滚动正确。
- Idle 时 Hand 不运行 Tick/Active Timer；拖拽期间最多一个动画驱动，且不逐帧产生 ChildOrder 重建。
- Development Editor 编译通过；PIE 与 Slate Insights 验收交给独立测试模型。若需要读取 UE 画面，必须为每次读取先取得用户明确确认。

## 7. 进度与决策记录

- 2026-07-11：创建 M3 并进入实施。确定使用 Slate 原生拖拽、Feature 内本地展示牌序、稳定 keyed 组件树、单 Active Timer 与弹簧/曲线组合；第一阶段不做拖牌出牌，也不提前提炼通用 Motion Foundation。
- 2026-07-11：完成生产实现。本地展示顺序及严格校验、快照调和、稳定 Hand/Card 节点、显式 reorder callback、Slate drag/drop、边缘滚动、Hover/选择动效、弹簧换位和 CubicOut 取消回弹已经接入；CurveSequence 关闭自带 timer，由唯一 Hand Active Timer 采样。Development Editor 构建通过，Plan 保持 In Progress，等待独立模型执行 PIE 与 Slate Insights 运行验收。
- 2026-07-12：完成拒绝回滚、同步回调重入防护、重复刷新 props early-out 与快照 CardId 不变量诊断；再次通过 Development Editor 构建并完成 `graphify update .`。生产任务全部交接，Plan 继续保持 In Progress，直到独立运行验收完成。
- 2026-07-12：根据实际 PIE 反馈修复拖拽牌被 Hand/ScrollBox 祖先裁剪的问题。拖拽视觉提升为 windowless Slate decorator，并继续由 Hand 的唯一 Active Timer 驱动位置、缩放和旋转；原 keyed Card 节点透明保留占位。Development Editor 构建通过。
