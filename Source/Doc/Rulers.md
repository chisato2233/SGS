# Rulers.md — SGS 项目宪法

> 本文件是所有 Agent 必读的长期全局约束。它包含每个任务都必须知道的架构、编码与工作流底线。
> 任务领域细则位于 `Source/Doc/Rules/`，按需读取；具体计划位于 `Source/Doc/Plan/`。
> 代码结构、类关系、依赖关系和影响面由 graphify 维护，不在文档里手写替代。

---

## 1. 启动协议

每次新会话或切换任务时，按顺序读取：

1. `ProjectBrief.md`：项目目标、技术栈、当前状态。
2. `Source/Doc/Rulers.md`：本文件，项目宪法。
3. `Source/Doc/Rules/README.md`：按任务选择需要读取的领域规则细则。
4. `Source/Doc/Plan/README.md`：了解计划系统与路径发现规则；只按需读取相关 Plan。
5. 需要理解代码结构、依赖关系、调用路径、影响面时，优先使用 graphify query/path/explain。

不默认全文读取 `Archive/`、`0000-RawRequirements.md`、`GRAPH_REPORT.md` 或所有规则文件。`Archive/` 是历史计划，默认不读；`0000-RawRequirements.md` 是可能非常冗长的原始需求历史，只在追溯用户原话时检索 / 局部阅读。

---

## 2. 文档分层

- `ProjectBrief.md`：长期稳定项目地图，只写项目是什么、当前阶段、技术栈和导航。
- `Rulers.md`：全局不可违反的宪法，包含架构、编码、工作流和安全底线。
- `Source/Doc/Rules/*`：按需领域细则，说明约束和边界，不描述代码结构。
- `Source/Doc/Plan/*.md`：具体开发计划、验收和进度；排除 README、模板、`0000-RawRequirements.md` 后，主目录计划即当前工作面。
- `Source/Doc/Plan/Archive/*`：历史计划档案，默认不读，只在追溯决策时读取具体文件。
- `Source/Doc/Plan/0000-RawRequirements.md`：用户原始需求档案，只追加，不整理成常驻上下文，默认不全文读取。
- `graphify-out/*`：代码事实图谱，负责结构、关系、跨文件影响面。

历史不另建大量 decision/ADR 文件；需要追溯时使用 git、归档 Plan、RawRequirements 和 graphify。

---

## 3. 架构宪法

- **服务器权威**：规则结算只在服务器权威逻辑层执行，服务器状态是唯一真相源。客户端只显示状态和采集输入，不持有可信规则状态。
- **多人与 AI 同源**：真人、AI、RPC、回放都通过同一决策抽象进入规则层；规则层不关心决策来源。
- **异步非阻塞**：等待玩家决策时逻辑层挂起，不阻塞游戏线程；应答、超时、默认选择或托管恢复结算。
- **表现解耦**：UI、动画、音效、摄像机、输入提示不得承载规则结算。规则层可以产出事件、状态和意图，但不反向依赖具体 UI 实现。
- **网络边界清晰**：网络只传递可信服务器状态和玩家指令，不允许客户端直接修改规则事实源。
- **数据与规则分离**：静态内容走 DataTable、DataAsset、GameplayTag 等开放数据入口；规则执行走 SGS C++ 管线。
- **GAS 是底座，不是规则核心**：GAS 用作 Attribute、GameplayEffect、GameplayTag、GameplayCue / 表现桥接和可复用效果载体；Command、随机审计、回放、牌区移动、响应窗口和三国杀式结算顺序仍由 SGS C++ 规则管线控制。

---

## 4. 编码宪法

- **UE 命名**：类前缀遵守 UE 约定：`U` / `A` / `F` / `I` / `S` / `E`。SGS 业务类使用 `SGS` 或清晰领域前缀，避免 `UCard`、`UEffect` 这类泛名。
- **命名风格**：函数、变量、常量使用 PascalCase；bool 使用 `b` 前缀。
- **枚举克制**：技术层内部、可证明封闭稳定、确实需要编译期分支时，才使用 enum。规则/内容概念默认不用封闭 enum，详见 `GameplayModeling.md`。
- **头文件**：使用 `#pragma once`；一个头文件只放一个主类型，小型辅助 struct 可同文件；优先前向声明；公共 API 使用 `SGS_API`；`.generated.h` 必须是最后一个 include。
- **UE 反射**：核心逻辑默认 C++，不要为了“未来可能蓝图扩展”提前加 `BlueprintCallable` / `BlueprintNativeEvent`。对象指针使用 `TObjectPtr<>`。`BlueprintReadOnly` 可用于调试可视化，不代表允许蓝图承载规则逻辑。
- **注释**：非平凡主类 / 主工具头文件开头写 3-8 行中文用途注释：职责、典型入口、关键不变量。不写流水账注释、changelog、作者、日期；行内注释只解释 why、不变量、陷阱或 TODO。
- **错误处理**：不使用 C++ 异常。不可恢复的不变量用 `check()`；可继续但不应发生的情况用 `ensure()`；业务日志使用集中声明的 SGS 日志分类；规则层失败路径优先使用项目结果/错误类型，而不是裸 `bool` 丢失原因。

---

## 5. 工作流宪法

- **状态先行**：新会话、提交前、切换任务前运行 `git status --short --branch`。
- **计划先行**：有规模的代码或文档工作应对应活跃 Plan；小型维护可直接完成，但不得绕过已有 Plan 的约束。
- **真实工程切片**：Plan 和实现必须面向真实游戏路径，考虑失败路径、关键不变量、扩展点和后续接入；不得用空实现、假数据或只创建类的脚手架冒充完成。
- **状态回写**：完成实质工作后更新相关 Plan 与 `ProjectBrief.md` 当前状态；完成并验证的 Plan 移入 `Source/Doc/Plan/Archive/`，主目录只保留当前工作面。
- **原始需求**：`0000-RawRequirements.md` 只追加，不修改历史条目。
- **构建脚本**：统一使用项目脚本：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Unreal.ps1 -Action Build -Configuration Development
```

若 UBT/UBA 卡住，仍使用同一脚本加 `-NoUBA` 重试。不要直接手写裸 `Build.bat`，除非任务是在排查该脚本本身。

- **graphify**：需要代码结构、依赖关系、调用路径、影响面时，优先使用 `graphify query/path/explain`。修改代码后运行 `graphify update .`；只改说明性文档时按需要更新。面向人的 HTML 图可用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\GraphifyVisualFilter.ps1
```

该脚本只影响可视化视图，不替代原始 `graph.json`。

- **Git 与 LFS**：Git LFS 只用于单文件大小 >= 50 MiB 的大文件，不按扩展名一概托管。提交前运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\CheckLargeFiles.ps1 -Scope Staged
```

- **提交安全**：只 stage 本次任务相关文件；不使用 `git reset --hard`、`git checkout -- <path>`、批量删除或改写历史，除非用户明确要求。

---

## 6. 规则建模硬约束

- **数据驱动与开放标识优先**：规则/内容概念默认使用 GameplayTag、FName、DataTable RowName、数据资产、注册表、策略对象或规则类。
- **不要封闭可扩展概念**：阶段、回合、座次、花色、卡牌名、技能名、效果类型、状态类型、目标类型等会被卡牌/技能扩展的概念，不得轻易用 enum 表达。
- **结算不依赖 wallclock**：规则时序按回合、阶段、出牌次序和离散 TimingPoint 推进；`Tick`、`DeltaTime`、`FTimerHandle` 只能服务表现层动画。
- **Native Code-first UI**：UI 主路线为 Slate/UMG/CommonUI 按需组合的原生代码优先体系；不使用 Designer 拖控件，不以 WebView/React/Vue/Noesis/Gameface 作为主 UI。
- **Plan 0012 工具库是规则层默认入口**：新规则代码优先使用 CommandRouter、RandomAudit、IndexedStore/TargetQuery、ActiveEffectTimeline、EffectPipeline、ReplayLog。不得重新发明平行命令、随机、索引、目标筛选、持续效果、效果执行或回放体系。
- **graphify 正交原则**：文档只写意图、约束、边界、为什么；代码功能、文件关系、类图、调用路径、影响面交给 graphify。

---

## 7. 领域细则路由

架构、编码、工作流规则已经并入本文件，不再作为按需细则维护。

从 `Source/Doc/Rules/README.md` 选择任务相关领域文件：

- 规则建模、数据驱动、开放标识：`GameplayModeling.md`
- UI：`UI.md`
- GAS：`GAS.md`
- Plan 0012 工具库入口：`RuleToolkit.md`

只读取与当前任务有关的细则，不把整个 `Rules/` 目录塞进上下文。

---

## 8. 变更协议

- 新项目级规则先写入相关 Plan 的进度/决策记录；确认具有长期全局价值后，才提升到 `Rulers.md` 或 `Rules/*`。
- `Rulers.md` 新增条目必须全局、稳定、值得每个 Agent 默认读取；详细理由留在 Plan、git 历史或 graphify 可查材料中。
- `Rules/*` 只承载领域细则，不沉淀架构、编码、工作流这类高频通用规则。
- 不修改 `0000-RawRequirements.md` 的历史条目，只能在顶部追加。
- 完成并验证的 Plan 移入 `Source/Doc/Plan/Archive/`，主目录只保留当前工作面。

Last reviewed: 2026-06-27
