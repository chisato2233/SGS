# 0001 — 导入太阳神三国杀（QSanguosha）素材

| 字段 | 值 |
|---|---|
| 状态 | `In Progress` |
| 创建日期 | 2026-06-19 |
| 最近更新 | 2026-06-19 |
| 关联需求 | `0000-RawRequirements.md` 第 #3 条 |
| 关联代码 | `Content/ImportedAssets/QSanguosha/`（纯素材，无代码）|

---

## 1. 目标

把开源项目 **太阳神三国杀 / QSanguosha**（`Mogara/QSanguosha`）的素材（卡牌图、武将图、配音、音乐，以及 UI 素材）引入本项目，供后续做三国杀原型时复用。

验收：素材按原结构落到 `Content/ImportedAssets/QSanguosha/` 下，经 Git LFS 跟踪，附带署名文件 `ATTRIBUTION.md`。

## 2. 背景与约束

- **用途**：个人学习 / 非商用（用户明确）。
- **许可（硬约束）**：素材为 **CC BY-NC-ND 4.0**——
  - BY：必须署名（见 `ATTRIBUTION.md`）；
  - NC：仅非商用，**禁止商业化**；
  - ND：可原样（含格式转换）非商用再分发，不得公开分发改动后的衍生素材。
- QSanguosha 代码为 **GPLv3**：本次**只取素材、不取任何代码**（避开 `lua/`、`src/`），避免 GPL 传染本项目 C++。
- **残留风险**：部分立绘/配音原始来源可能为《三国杀 OL》（游卡）；若转商业化，须整目录移除并替换。
- 本仓库为 **public**，CC BY-NC-ND 允许非商用 + 署名的再分发，符合现状。

## 3. 方案设计

- 落地路径：`Content/ImportedAssets/QSanguosha/{audio,image}/`，**保持 QSanguosha 原始子目录结构**（便于对照其文档/wiki，降低分类出错）。
- 走 **Git LFS**：在 `.gitattributes` 增补 `*.png/.jpg/.jpeg/.bmp/.gif/.tga/.psd/.ogg/.wav/.mp3` 的 LFS 规则。
- 署名：`Content/ImportedAssets/QSanguosha/ATTRIBUTION.md` 记录来源仓库、取用提交、许可与使用约束。
- **UE `.uasset` 导入**不在本计划内：当前环境无 UE 编辑器，只放原始 `png/ogg` 源文件；正式 import 成 `.uasset` 待后续在编辑器中完成（或写 import commandlet 自动化），另立计划。

## 4. 任务拆解

- [x] 确认素材许可（CC BY-NC-ND 4.0）与用途（非商用）
- [x] `.gitattributes` 增补图片/音频 LFS 规则
- [x] 拷贝 `audio/`、`image/` 到 `Content/ImportedAssets/QSanguosha/`
- [x] 编写 `ATTRIBUTION.md`
- [x] 记录原始需求（`0000-RawRequirements.md` #3）并建本计划
- [ ] 提交（LFS）并推送、建 PR
- [ ] （后续，另立计划）在 UE 编辑器导入为 `.uasset` 并整理引用

## 5. 验收标准

- `Content/ImportedAssets/QSanguosha/` 下含 `audio`（≈404 文件）、`image`（≈1704 文件）与 `ATTRIBUTION.md`。
- `git lfs ls-files` 能列出这些 png/ogg，确认走 LFS 而非直接入库。
- 仓库可正常 clone，素材完整。

## 6. 进度与决策记录

- 2026-06-19：QSanguosha 取用提交 `85baa748`（2022-02-25）。确认双协议：代码 GPLv3 / 素材 CC BY-NC-ND 4.0，本次只取素材。
- 2026-06-19：用户确认非商用、全量导入（含 UI 素材）。
- 2026-06-19：素材按原结构落入 `Content/ImportedAssets/QSanguosha/`，新增图片/音频 LFS 规则与 `ATTRIBUTION.md`。
- 2026-06-19：决定 UE `.uasset` 正式导入另立计划（当前环境无编辑器）。
