# AGENTS.md — Codex 入口适配层

> 本文件只服务 Codex / Agent 自动发现规则。它不是项目事实源，不替代
> `ProjectBrief.md`、`Source/Doc/Rulers.md`、`Source/Doc/Rules/*` 或 `Source/Doc/Plan/*`。

## SGS 启动协议

每次新会话或切换任务时，先按项目文档系统获取上下文：

1. 读 `ProjectBrief.md`（项目全貌、技术栈、当前状态）。
2. 读 `Source/Doc/Rulers.md`（项目宪法，包含架构、编码、工作流底线）。
3. 读 `Source/Doc/Rules/README.md`，并只按当前任务读取相关领域细则。
4. 读 `Source/Doc/Plan/README.md`，再按路径发现当前工作面：`Source/Doc/Plan/*.md`（排除 README、模板、`0000-RawRequirements.md`）是活跃 / Ready Plan；`Source/Doc/Plan/Archive/` 是历史，默认不读，只在追溯决策时按需读取具体文件。`0000-RawRequirements.md` 是可能很长的原始需求历史，默认不全文读取。
5. 需要理解代码结构、依赖关系、影响面时，优先使用 graphify 图谱。

执行规则：

- `ProjectBrief.md` 是长期稳定入口；`AGENTS.md` 只把 Codex 引回这个入口。
- `Rulers.md` 是全局长期宪法；架构、编码、构建、提交、LFS、graphify 更新等高频规则都在这里。
- 任务领域细则按 `Source/Doc/Rules/README.md` 路由读取，不默认全文加载整个规则目录。
- 代码结构和依赖关系由 graphify 维护，不在文档里手写类图/依赖图，也不要用规则文件替代 graphify 描述代码功能。
- 规则层、UI、GAS、建模等任务按 `Source/Doc/Rules/README.md` 读取对应细则。

## 本地参考仓库

- `QSgsRef/` 是本机参考代码区，当前用于存放太阳神三国杀 / QSanguosha 仓库；它不是 SGS 项目源码、不是项目事实源，也不进入 Git 跟踪。
- 日常开发默认不要读取 `QSgsRef/`，也不要把它并入主项目 `graphify-out/`。只有用户明确要求参考“太阳神三国杀 / QSanguosha”，或任务确实需要对照其规则、UI、资源组织、历史实现时，才按需读取。
- 参考仓库路径：`QSgsRef/QSanguosha/`。它的独立 graphify 索引放在 `QSgsRef/QSanguosha/graphify-out/`。
- 查询参考仓库时，优先使用它自己的图谱，例如在 `QSgsRef/QSanguosha/` 下运行 `graphify query "<问题>"`，或在项目根目录使用 `graphify query "<问题>" --graph .\QSgsRef\QSanguosha\graphify-out\graph.json`。不要因此改写主项目图谱。
- 如果参考仓库图谱缺失或明显过期，在 `QSgsRef/QSanguosha/` 目录内运行 `graphify update .` 维护它；修改 SGS 项目源码后仍只按常规在项目根目录运行 `graphify update .`。

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, invoke the `skill` tool with `skill: "graphify"` before doing anything else. If the current Codex surface does not expose a skill tool, use the local `graphify` CLI instead.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- For human-facing graph HTML, run `powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\GraphifyVisualFilter.ps1` after `graphify export html` to collapse common UE/C++ primitive/container type nodes into `UE/Core Types`. Use `-Mode Remove` when a fully filtered view is preferred. Keep raw `graphify-out/graph.json` as the authoritative graph data.
