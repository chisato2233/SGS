# 0004 — 对局数据模型：卡牌、牌区、玩家状态与基础操作

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-06-19 |
| 最近更新 | 2026-06-19 |
| 关联需求 | `0000-RawRequirements.md` 第 #4 条 |
| 关联代码 | `Source/SGS/Logic/{Cards,Players,Engine}/`（详见 graphify）|

---

## 1. 目标

提前建好后续所有玩法都依赖的**数据底座与操作原语**（杀/闪/桃、锦囊、装备、判定、武将技、AI 全建在其上）：

- 卡牌模型（静态定义 + 运行态实例）。
- 牌区（牌堆/弃牌堆/手牌/装备区/判定区）。
- 玩家运行态（体力/势力/手牌/装备/判定区）。
- 基础操作原语：通用移牌、摸牌（含洗牌回收）、弃牌、伤害、回复、距离计算，并广播事件。

这些是"网络依赖低、可人工审查"的纯服务器侧逻辑，适合在无编译环境时提前建设。

## 2. 背景与约束

- 因无 UE 编译环境（见 Plan 0003），优先建设可人工审查的逻辑层；复制/RPC 仍待 0003N。
- Rulers：数据驱动（牌库走 DataTable，代码不硬编码全套）、纯 C++、结算不依赖 wallclock、注释只写 why。
- 分层：新增 `Logic/Cards/`；模型与"改模型的最小操作"集中到 `USGSGameContext`，与流程控制 `USGSGameDriver`、决策 `ISGSDecisionAgent` 三者分离。

## 3. 方案设计

### 3.1 卡牌
- `FSGSCardDef`（DataTable 行）：牌名、显示名、大类、装备栏、武器攻击范围、坐骑距离修正。完整标准牌库由 DataTable 资产维护。
- `USGSCard`（运行态）：唯一 `CardId` + 牌名 + 花色 + 点数；颜色由花色推导。
- `USGSCardPile`：有序牌区（索引 0 为顶），含增删/取顶/移除/洗牌（Fisher–Yates + 可复现随机流）。

### 3.2 玩家状态
- 扩展 `USGSSeat`：体力/上限、势力、手牌区、判定区、装备区（按栏 `TMap`）。

### 3.3 对局模型 + 原语（`USGSGameContext`）
- 持有牌堆/弃牌堆/座位/全部牌 + 可复现随机流。
- 原语：`MoveCards`（通用换区）、`DrawCards`（空时洗回弃牌堆）、`DiscardFromHand`、`ApplyDamage`、`Heal`、`GetDistance`（存活环形最短 + 坐骑修正，最小 1）。
- 事件：`OnCardsMoved` / `OnDamage` / `OnHealthChanged` / `OnSeatDying`（C++ 多播，供触发系统订阅）。

### 3.4 与驱动器集成
- `USGSGameDriver` 改为持有 `USGSGameContext`（座位归 Context）。
- 摸牌阶段调用 `DrawCards(2)`；开局发 4 张起手（牌堆为空时不发，待牌库数据接入自然生效）。
- 决策/濒死/弃牌限制等仍走后续 Plan。

### 3.5 有意延后（避免在无编译环境下写半成品）
- 装备的安装/卸下与装备区移动 → Plan 0008。
- 濒死/求桃/真正死亡判定 → Plan 0005（本期 `ApplyDamage` 仅广播 `OnSeatDying`）。
- 弃牌阶段手牌上限的"选哪些弃"（需决策）→ 后续。
- 标准牌库内容（108+ 张）走 DataTable 资产 → 内容步骤（需编辑器）。

## 4. 任务拆解

- [x] 新增 `LogSGSCombat` 日志分类（同步 Rulers）
- [x] 卡牌模型：`SGSCardTypes` / `SGSCardDef` / `SGSCard` / `SGSCardPile`
- [x] 扩展 `USGSSeat` 玩家运行态
- [x] `USGSGameContext`：模型 + 原语 + 事件
- [x] 重构 `USGSGameDriver` 用 Context；摸牌阶段/起手接入
- [ ] （待编译环境）编辑器编译 + 用占位牌库 PIE 验证原语
- [ ] （后续）标准牌库 DataTable 内容

## 5. 验收标准

- 代码符合 Rulers 规范；分层依赖单向（`Engine → Cards/Players`，无反向）。
- 人工走查：移牌/摸牌/洗回/伤害/回复/距离逻辑自洽；事件在对应原语处广播。
- 距离：相邻为 1；+1/-1 马按栏修正；最小 1。
- 待编译环境补：编译通过 + 用占位牌库跑通摸牌/弃牌/伤害链。

## 6. 进度与决策记录

- 2026-06-19：引入 `USGSGameContext`（数据 + 原语）与 `USGSGameDriver`（流程）分离；座位归 Context。
- 2026-06-19：牌区索引 0 定义为「顶」；洗牌用可复现 `FRandomStream`（StartGame 传 seed）。
- 2026-06-19：距离用「存活座位环形最短 + 坐骑栏修正」；坐骑判定直接看装备栏 Key，无需查 def。
- 2026-06-19：装备移动/濒死/弃牌选择/牌库内容按 3.5 有意延后。
- 2026-06-19：⚠️ 全部未编译，人工审查为准。
