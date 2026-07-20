# 0011-M5 — 服务器权威的摸牌、出牌与弃牌演出

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-07-20 |
| 最近更新 | 2026-07-20 |
| 关联需求 | `0000-RawRequirements.md` 中的第 32 条 |
| 父计划 | `0011-native-code-first-ui.md` |
| 关联代码 | `Source/SGS/Server/Effects/`、`Source/SGS/Server/UI/`、`Source/SGS/Shared/UI/`、`Source/SGS/Client/UI/Features/Table/` |

---

## 1. 目标

参考 NoName 的 `$draw`、`$gain2`、`$throw` 与 `useCard/respond` 表现流程，为 Native Slate 八人牌桌补齐服务器权威、全桌可见的摸牌、出牌/响应和弃牌转场。规则继续立即结算；演出只消费可复制的有序表现事件，不阻塞规则、AI 或输入。

## 2. 边界与约束

- ReplayLog 是表现事实来源；GameContext 不依赖 UI 类型，Slate 不反向读取服务器规则对象。
- 对手摸牌只公开数量、牌背和目标座位；本地 owner-only 快照用同序号覆盖牌面。公开使用、响应与弃牌对所有观看者显示牌面。
- 处理区回收和弃牌堆洗回牌堆保留为 `Cleanup` 事件，只推进游标而不重复播放。
- 动画只改变瞬时 Widget 的 RenderTransform、Opacity 和绘制；空闲时不保留 Active Timer。
- 复用现有 QSanguosha `card-back` 与卡面资源，不增加美术文件。声音、粒子、拖牌直接出牌和正式重连协议不在本里程碑。

## 3. 方案设计

- ReplayLog 牌移动载荷包含匹配纪元、连续牌移动序号、开放 `FName` 原因、牌 ID、起止牌区/座位与关联目标。标准 EffectStep 负责在权威移动成功后写入。
- GameDriver 的开局/回合/奖励摸牌、使用/响应、自动/死亡弃牌全部进入 EffectPipeline。表现状态变化通过委托通知 GameMode，GameMode 下一游戏 Tick 合并刷新 owner-only 与公共快照。
- 公共与私有快照携带最近 64 条牌移动。公共流做隐私裁剪，私有流只携带本地可见的同序号覆盖；组合时按序号替换、排序并去重。
- Table Store 保存纪元、已消费/已排队序号和最多 32 条待播提示；纪元切换、事件窗口缺口和队列溢出会清除旧演出并跳到最新权威状态。
- 布局提供摸牌堆、中央出牌区和弃牌堆矩形。全视口命中测试透明 Motion Layer 以单 Active Timer 播放所有瞬时牌，并绘制来源到目标的连线/高亮。

## 4. 工程质量门

- 表现事件必须来自真实规则路径，不允许 UI 猜测“手牌数变化即摸牌”。
- 公共快照中隐藏摸牌的 `VisibleCards` 必须为空，不能放入占位 CardId；私有信息只能覆盖对应观看者的事件。
- 牌移动序号独立连续，避免 ReplayLog 中其他事件造成伪缺口；Cleanup 也占序号并被客户端无动画确认。
- HUD 销毁、纪元变化、窗口缺口或队列快进必须注销计时器并移除 transient cards。

## 5. 任务拆解

- [x] 增加结构化 ReplayLog 牌移动事件与开放原因，并迁移权威牌移动入口。
- [x] 增加下一 Tick 合并快照刷新及公共/私有隐私投影。
- [x] 增加 Table Motion 队列、消费游标、缺口与溢出快进。
- [x] 增加三个牌区锚点、牌背/弃牌顶显示和单计时器全桌演出层。
- [x] 增加 M5 权威性、隐私、队列和布局自动化。
- [x] 完成 Development Editor 构建与 `SGS.Plan0011*` 自动化。
- [ ] PIE 手工验收 `1 真人 + 7 AI`；任何画面读取须另行获得用户确认。

## 6. 验收标准

- 开局发牌、本地/对手摸牌、杀/闪/桃/酒、自动弃牌与死亡清牌按服务器序号播放；本地开局座位优先，目标线与目标高亮在使用/响应停留期间可见。
- 对手摸牌公共事件不含牌 ID、名称、花色、点数；本地私有覆盖恢复本地牌面，公开牌移动对全桌可见。
- 重复/乱序快照不重播；纪元切换、64 条窗口缺口、32 条待播溢出和 HUD 销毁均无泄漏或残留动画。
- `1920×1080`、`1600×900`、`1280×720`、`960×540` 的牌区锚点不与座位、手牌或决策区冲突，空闲时没有 Motion Active Timer。
- Development Editor 构建、`SGS.Plan0011*` 自动化和 PIE 一局手工验收通过。

## 7. 进度与决策记录

- 2026-07-20：完成生产实现与自动化代码。采用独立牌移动连续序号，避免 ReplayLog 的 Step/Command 事件在客户端形成伪缺口；开局按相对座位距离成对重叠播放，使本地优先且总时长约 1.2 秒。
- 2026-07-20：首次 Development 构建在编译前被正在运行的 UE Editor Live Coding 拒绝。未擅自关闭 Editor，构建与自动化验证待 Editor 安全退出后执行。
- 2026-07-20：Editor 安全退出后，Development Editor 全量构建通过；`SGS.Plan0011M1.LocalUI.DecisionBridge`、`SGS.Plan0011M2.Table.StateStore` 与新增 `SGS.Plan0011M5.Motion.AuthorityPrivacy/Layout/Queue` 全部 Success。回归中同时修复 Nova 左右列座位按 196 高度比例放置、与当前 232 高度面板轻微相交的问题。当前只剩 PIE 主观演出验收。
