# 0003 — 权威对局骨架（回合阶段机闭环）

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-06-19 |
| 最近更新 | 2026-06-19 |
| 关联需求 | `0000-RawRequirements.md` 第 #4 条 |
| 关联代码 | `Source/SGS/{Logic,AI,Game}/`（详见 graphify）|

---

## 1. 目标

落地"长计划"的第一块可运行骨架：**一个空对局能从开始跑到结束**，全程服务器权威、经决策代理驱动出牌阶段、按回合/阶段推进。

验收（见第 5 节）：纯 AI 座位下，对局依次广播 `GameStarted → (每回合) TurnBegan → 各阶段 PhaseBegan/PhaseEnded → TurnEnded → … → GameEnded`，出牌阶段经 `ISGSDecisionAgent` 异步取得「Pass」后继续，无阻塞、无递归爆栈。

## 2. 背景与约束

- 依 Plan 0002 架构：**服务器权威 + 决策代理 + 异步非阻塞**。
- Rulers 硬约束：结算不依赖 wallclock（不用 Timer/DeltaTime 推进，用蹦床循环 + 回调）；纯 C++（事件用 C++ 多播委托，不暴露蓝图）。
- **编译环境限制（重要）**：当前 Cloud 环境无 UE 引擎，无法编译/运行。带 UE 复制宏、RPC、`GENERATED_BODY` 的网络层代码**最易在不编译时留坑**。因此本期**只交付可严格人工审查的服务器侧逻辑闭环**，把**复制（GameState/PlayerState 属性）、PlayerController、RPC 决策通道**留到具备编译环境的网络 Plan。这是有意的范围收缩，已更新 Plan 0002 路线图。

## 3. 方案设计

分层落点（单向依赖 `Game → Logic`，`AI → Logic`）：

- `Logic/Players/SGSDecisionTypes.h`：出牌阶段请求/应答数据 + 异步回调委托（纯 C++）。
- `Logic/Players/SGSDecisionAgent.h`：`ISGSDecisionAgent` 接口——真人/AI 统一入口；约定**异步应答**。
- `Logic/Players/SGSSeat.h`：座位模型（持有决策代理，不感知人/AI）。
- `Logic/Engine/SGSGameEvents.h`：事件总线最小形态（开放 `FSGSGameEvent` 标签 + 上下文 + C++ 多播委托）。
- `Logic/Engine/SGSGameDriver.{h,cpp}`：对局驱动器——回合/阶段推进 + 出牌阶段异步决策。
- `AI/SGSAutoPassAgent.{h,cpp}`：骨架期占位 AI（一律 Pass，同步应答）。
- `Game/SGSGameMode.{h,cpp}`：服务器权威入口；`BeginPlay` 建纯 AI 座位并启动一局。

关键机制：

- **蹦床推进（trampoline）**：`Pump()` 在一个循环内连续推进所有「同步完成」的阶段；遇到出牌阶段挂起（等待决策）或对局结束才停。避免深递归与定时器。
- **异步决策 + 重入保护**：出牌阶段置 `bWaitingForDecision` 并向代理请求；代理可同步（AI）或跨帧（真人）回调。`bPumping` 守卫防止同步回调重入 `Pump`。
- **请求配对**：`RequestId` 单调递增，丢弃过期/重复应答（为真人超时托管预留）。
- **健壮性**：空座位保护、缺失代理按 Pass、推进时跳过阵亡座位、无存活座位即结束。

## 4. 任务拆解

- [x] 决策类型与代理接口（`SGSDecisionTypes` / `SGSDecisionAgent`）
- [x] 座位模型（`SGSSeat`）
- [x] 事件总线最小形态（`SGSGameEvents`）
- [x] 对局驱动器：异步回合阶段机（`SGSGameDriver`）
- [x] 占位 AI 代理（`SGSAutoPassAgent`）
- [x] 服务器入口（`SGSGameMode`）
- [ ] （后续网络 Plan）复制状态 + PlayerController + RPC 决策通道
- [ ] （待编译环境）在 UE 编辑器编译并以 PIE 验证日志序列

## 5. 验收标准

- 代码符合 Rulers 规范（命名、`#pragma once`、`SGS_API`、`generated.h` 顺序、注释规则）。
- 逻辑自洽（人工走查）：空对局产生预期事件序列，出牌阶段经代理 Pass 后推进，跑满 `MaxTurnsForSkeleton` 个回合后 `GameEnded`。
- 无阻塞、无递归爆栈（蹦床 + 重入守卫）。
- 待编译环境补充：编辑器编译通过 + PIE 日志验证。

## 6. 进度与决策记录

- 2026-06-19：实现服务器侧逻辑闭环（驱动器/事件/座位/代理/AI/GameMode）。
- 2026-06-19：**范围收缩**——复制/RPC/PlayerController 因无编译环境留待网络 Plan；蹦床循环 + `bPumping` 重入守卫替代阻塞式等待。
- 2026-06-19：终止条件用 `MaxTurnsForSkeleton` 占位，胜负条件实现后替换。
- 2026-06-19：⚠️ 风险：所有 UE C++ 未经编译，以人工审查为准；需尽快接入可编译的 UE5.7 环境补验证。
