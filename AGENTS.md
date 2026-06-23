# AGENTS.md — Codex 入口适配层

> 本文件只服务 Codex / Agent 自动发现规则。它不是项目事实源，不替代
> `ProjectBrief.md`、`Source/Doc/Rulers.md` 或 `Source/Doc/Plan/*`。

## SGS 启动协议

每次新会话或切换任务时，先按项目文档系统获取上下文：

1. 读 `ProjectBrief.md`（项目全貌、技术栈、当前状态）。
2. 读 `Source/Doc/Rulers.md`（项目级编码约束）。
3. 读 `Source/Doc/Plan/README.md`，再按 `ProjectBrief.md` 第 5 节指向读取相关 Plan。
4. 需要理解代码结构、依赖关系、影响面时，优先使用 graphify 图谱。

执行规则：

- `ProjectBrief.md` 是长期稳定入口；`AGENTS.md` 只把 Codex 引回这个入口。
- 代码结构和依赖关系由 graphify 维护，不在文档里手写类图/依赖图。
- 修改代码后运行 `graphify update .` 更新图谱；只改说明性文档时按需要更新。
- 提交前遵守 `Source/Doc/Rulers.md` 的 Git 工作流。
- Agent 创建 git commit 前必须运行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\CheckLargeFiles.ps1 -Scope Staged`。本地 `.githooks/pre-commit` 会执行同一检查；若发现单文件 >= 50 MiB 且未进 LFS，先按脚本提示处理再提交。

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, invoke the `skill` tool with `skill: "graphify"` before doing anything else. If the current Codex surface does not expose a skill tool, use the local `graphify` CLI instead.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
