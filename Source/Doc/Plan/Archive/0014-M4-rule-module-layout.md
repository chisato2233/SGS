# 0014-M4 — Rule 目录与 Payload 组织重构

| 字段 | 值 |
|---|---|
| 状态 | `Archived` |
| 创建日期 | 2026-07-03 |
| 最近更新 | 2026-07-03 |
| 关联需求 | `0000-RawRequirements.md` 中的第 17 条 |
| 父计划 | `0014-rule-layer-long-term-refactor.md` |
| 前置计划 | `0014-M3-resolution-stack-resume-timing-event.md` |
| 关联代码 | `Source/SGS/Server/Rules/`、`Source/SGS/Server/Commands/`、`Source/SGS/Server/Engine/`、`Source/SGS/Tests/` |

---

## 1. 目标

M4 不新增玩法 Rule，只整理 Rule 层工程结构，让后续新增卡牌、响应、触发、修正和视为规则时有固定落点：

1. Rule 核心框架、payload、resolution stack 和具体基础牌规则分离。
2. 基础牌按 `RuleKind + 内容包` 两层目录组织：主动规则进入 `Actions/BasicCards`，响应规则进入 `Responses/BasicCards`。
3. `SGSResolutionStack` 只保存通用 frame 与 stack 语义，不再 hard-code Slash / DyingPeach frame state。
4. `SGSRulePayloads.h` 退化为 umbrella include，具体 payload 分散到专属 payload 头。
5. `SGSRules::RegisterDefaultRules` 只调用基础牌内容包注册入口，不直接 include 每个具体 Rule。

本阶段只调整模块边界和 include 路径，不改变 Command payload、RuleInvocation 语义、Registry 匹配规则或基础牌行为。

## 2. 目录布局

```text
Source/SGS/Server/Rules/
  Core/
  Payloads/
  Resolution/
  BasicCards/
  Actions/BasicCards/
  Responses/BasicCards/
```

后续新增内容默认按同一策略扩展：

- 新主动牌 / 主动技能：`Actions/<ContentPackage>/`
- 新响应规则：`Responses/<ContentPackage>/`
- TriggerRule：`Triggers/<ContentPackage>/`
- ModifierRule：`Modifiers/<ContentPackage>/`
- ViewAsRule：`ViewAs/<ContentPackage>/`
- 内容包共享 helper / 注册入口：`<ContentPackage>/`

## 3. 任务拆解

- [x] 建立 `Rules/Core`、`Rules/Payloads`、`Rules/Resolution`、`Rules/Actions/BasicCards`、`Rules/Responses/BasicCards`、`Rules/BasicCards` 目录。
- [x] 移动 core framework 文件：RuleTypes、TypedRule、Descriptor、Invocation、Registry。
- [x] 拆分 payload：Pass / UseCard / RespondCard / RuleEvent 各自进入 focused header，`SGSRulePayloads.h` 只做 umbrella include。
- [x] 移动 `SGSResolutionStack` 到 `Rules/Resolution`，删除对 Slash / DyingPeach state 的直接依赖。
- [x] 将 `FSGSSlashResolutionState` 移入 `SGSSlashRule.h`。
- [x] 将 `FSGSDyingPeachResolutionState` 移入 `SGSDyingPeachRules.h`。
- [x] 将 `SGSBasicCardRules.*` 拆为内容包注册入口、共享 helper 和具体 Rule 文件。
- [x] 更新 Driver、CommandType、自动化测试 include 路径。
- [x] 跑静态结构检查、Development build、Plan0005 / Plan0013 / Plan0014 自动化与 `graphify update .`。

## 4. 验收标准

- Development build 通过。
- 自动化通过：`SGS.Plan0005.BasicCards`、`SGS.Plan0013.RulePipeline`、`SGS.Plan0014`。
- 静态结构检查通过：
  - `.generated.h` 后无 include。
  - `SGSResolutionStack.*` 不引用 `FSGSSlashResolutionState` / `FSGSDyingPeachResolutionState`。
  - `SGSRulePayloads.h` 不直接定义 payload `USTRUCT`，只聚合 include。
  - 根目录旧 `SGSBasicCardRules.*` 删除；新 `BasicCards/SGSBasicCardRules.cpp` 只负责注册。
- `graphify update .` 已运行。

## 5. 进度记录

- 2026-07-03：创建 M4 计划。决策：本阶段不新增玩法能力，只做 Rule 目录与 payload 组织重构；具体 Rule 按 `RuleKind + 内容包` 两层目录增长。
- 2026-07-03：完成 M4 代码与验证。Rule 核心、Payload、ResolutionStack、基础牌主动 / 响应规则已拆入目标目录；`SGSResolutionStack` 不再引用 Slash / DyingPeach state；`SGSRulePayloads.h` 已成为 umbrella include；Development build、Plan0005、Plan0013、Plan0014 和静态结构检查均通过，`graphify update .` 已运行。
