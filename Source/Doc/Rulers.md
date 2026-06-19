# AGENTS.md — 项目级编码约束

> **本文档为长期、稳定的编码约束，所有 Agent 必读。**
> 临时计划见 `Source/Docs/Plan/<current-plan>/plan.md`。
> 玩法设计见 `Content/Docs/Design/v0.2-设计大纲.md`。

---

## 1. 工作流程

### 1.1 启动协议

每个新会话按**严格顺序**：

1. 读 `Source/ProjectBrief.md`
2. 读 `Source/AGENTS.md`（本文件）
3. 按 ProjectBrief "当前 Plan" 指向，读对应 `Source/Docs/Plan/<plan>/plan.md`
4. **按本期任务涉及的模块，读对应 `Source/Docs/Modules/<模块>.md`**（活文档，描述当前接口面与不变量）
5. **仅在需要追溯设计意图时**，读 `Content/Docs/Design/v0.2-设计大纲.md`

> **不必读全库**。这是文档体系存在的全部理由。
> `plan.md` = 本期为什么这么做；`Modules/<X>.md` = 该模块现在怎么用。两者职责互补。

### 1.2 提交协议

每完成一个有意义的步骤：

- 勾选 / 更新 `plan.md` 的 TODO
- 必要时把决策写入 `plan.md` 的"已做决策"节
- **不要**直接改 `ProjectBrief.md`（除非阶段切换 / Plan 切换）
- **不要**直接改 `Content/Docs/Design/v0.2-设计大纲.md`
- **不要**改 `Content/Docs/OrginalRequire/`（用户原始资料）

### 1.3 引入新约束的协议

发现需沉淀的新项目级约束（例如"所有 UMG 必须重写 NativePreConstruct"）：

1. 先在当前 `plan.md` 的"已做决策"中暂存
2. 该 Plan 完成、归档时，提升为本文件的正式条目
3. 重大约束变更需征得用户同意

### 1.4 模块文档同步（硬约束）

修改 `Source/Stuff/<X>/` 下任一模块时，下列变更**必须**在同一 Plan 内同步对应 `Source/Docs/Modules/<X>.md`：

- 新增 / 删除 / 重命名导出类（`UCLASS` / `USTRUCT` / 普通 class）
- 改变 `UPROPERTY` / `UFUNCTION` / 委托的签名或语义
- 改变模块的关键不变量（生命周期、调用顺序、并发约束）
- 改变跨模块依赖（新增 / 移除一个 `#include` 链路对外）

**判定**：如果你刚改的内容会让"读旧版 Modules/X.md 的人写出错代码"，就必须改文档。

新模块判定为"复杂模块"时（>500 行 / ≥3 跨模块依赖 / 非 trivial 状态机）必须新建 `Modules/<X>.md`，详见 `Source/Docs/Modules/README.md`。

归档 Plan 时检查清单：
- [ ] 涉及的所有模块文档已更新
- [ ] 新模块的 `Modules/<X>.md` 已建并登记到 `Modules/README.md` 清单
- [ ] 对应文档底部 `Last Updated` 已刷新
- [ ] `Modules/<X>.md` 的"沉淀历史"追加了本 Plan 一行

### 1.5 不要做的事

- ❌ 不要主动修改 `Content/Docs/OrginalRequire/`
- ❌ 不要主动修改 `Content/Docs/Design/v0.2-设计大纲.md`
- ❌ 不要为了"看起来工作多了"加冗余测试 / 注释 / 日志
- ❌ 不要绕过 plan.md 偷偷做计划外变更
- ❌ 不要在没有 plan.md 的情况下开始一个 > 1 小时的任务

---

## 2. UE C++ 编码规范

### 2.1 命名

- **类前缀**：`U`（UObject）/ `A`（AActor）/ `F`（POD/Struct）/ `I`（接口）/ `S`（Slate）/ `E`（Enum）
- **业务前缀**：项目类一律加业务前缀避免和引擎/插件冲突：
  - ✅ `UCardSkill`, `UCardSkillEffect`, `UCombatAttributeComponent`, `UMatchSubsystem`
  - ❌ `USkill`（易冲突）, `UEffect`（太泛）
- **文件名 = 类名（不含前缀）**：`UCardSkill` → `CardSkill.h` / `CardSkill.cpp`
- **函数**：PascalCase（`ApplyDamage`, `GetCurrentCharge`）
- **变量**：PascalCase（`CurrentCharge`），bool 加 `b` 前缀（`bIsAlive`）
- **常量**：PascalCase（`DefaultActionPoint`），枚举值 `EFoo::Bar` 形式

### 2.2 头文件

- 总是用 `#pragma once`，不用 include guard
- 单一职责：一个 .h 一个主类（小辅助 struct/enum 可同文件）
- 用 `class FFoo;` 前向声明，避免引入大头
- 公共 API 加 `STUFF_API` 宏（项目模块名为 Stuff）

### 2.3 反射与对象

- 公开属性默认 `UPROPERTY(EditAnywhere, BlueprintReadOnly)`，BlueprintReadOnly 是为了未来 debug 工具，不代表会在蓝图里写逻辑
- 实例化子对象：`UPROPERTY(Instanced, EditAnywhere) TArray<TObjectPtr<UCardSkillEffect>> Effects;`
- 容器 / 对象指针使用 `TObjectPtr<>` 而非裸 `*`（UE 5+ 标准）
- ❌ 不用 `BlueprintCallable` / `BlueprintNativeEvent` 除非确实需要蓝图调用（本项目纯 C++）
- 不要为了"提供蓝图扩展点"提前加 BlueprintNative—— YAGNI

### 2.4 模块依赖

`Source/Stuff/Stuff.Build.cs` 当前已含：
`Core, CoreUObject, Engine, InputCore, EnhancedInput`

按需追加：
- `UMG`, `Slate`, `SlateCore` —— 启用 UMG 纯 C++ 时
- `GameplayTags` —— 派系 / 印记标签
- `DeveloperSettings` —— 项目级配置

❌ **不添加 `GameplayAbilities` / `GameplayTasks` / `GameplayAbilitiesEditor`**（不上 GAS）

### 2.5 注释规则

- **不写"narrate the code"型注释**（"// 增加计数器"、"// 设置变量"）
- 只写 **why** / 不变量 / 陷阱 / TODO
- 不在文件头加 changelog / author / date（用 git）
- 不在代码里写中文长解释（用 commit message 或 plan.md）

### 2.6 错误处理

- `check()` —— 不可恢复的不变量违反（断言）
- `ensure()` —— "不该发生但能继续" 的诊断
- `UE_LOG(LogCategory, Verbosity, TEXT("..."))` —— 业务日志
- 日志分类：`LogStuff`（通用）/ `LogStuffCombat` / `LogStuffSkill` / `LogStuffMatch` / `LogStuffUI`
- **不用 C++ 异常**（UE 默认禁）

### 2.7 头文件组织顺序（规范化模板）

```cpp
// .h
#pragma once

#include "CoreMinimal.h"          // 必须第一个
#include "<父类头文件>.h"
// 其他 #include
// 前向声明
#include "CardSkill.generated.h"  // 必须最后一个

UCLASS()
class STUFF_API UCardSkill : public UObject
{
    GENERATED_BODY()

public:
    // 构造、析构
    UCardSkill();

    // 公开接口
    void Activate(...);

    // 公开属性
    UPROPERTY(EditAnywhere)
    int32 ChargeMax;

protected:
    // 子类可见

private:
    // 内部实现
};
```

---

## 3. 项目硬约束（来自 v0.2 设计大纲）

以下约束**全局生效，不可违反**：

1. **不用 GAS**：技能 / 属性 / 印记一律自研。不引入 `GameplayAbilities` 模块。
2. **UMG 纯 C++**：`UWidget` 子类在 `NativeConstruct` 中用 `WidgetTree->ConstructWidget` 拼装。**不在编辑器 Designer 中拖控件**。少量必须在编辑器建的 Widget 资产（用于绑定动画）必须有充分理由并记录在对应 Plan 中。
3. **不写多人复制代码**：所有类不要加 `Replicated` / RPC 标记 / `bReplicates`。
4. **数据驱动优先**：新技能 = 一个 DataAsset 实例 + 复用现成 Effect 类。除非现有 Effect 类不够用，否则**不**为单张技能写新 C++ 类。
5. **战斗结算不依赖 wallclock**：所有冷却 / 持续 / 计时一律按"回合数"或"出手数"。`FTimerHandle`、`Tick` 中的 `DeltaTime` 累加都不可用于游戏逻辑。

---

## 4. 项目术语表（Glossary）

供命名 / 注释 / 文档统一使用：

| 中文           | 英文              | 说明                                    |
| -------------- | ----------------- | --------------------------------------- |
| 派系           | Faction           | 五行 + 阴阳，共 7 个 GameplayTag        |
| 技能           | Skill             | 不用 Card / Spell / Ability，统一 Skill |
| 印记           | Mark              | buff / debuff 统一称印记                |
| 充能           | Charge            | 整数累加，触达阈值释放                  |
| 主动技能       | Active Skill      | 玩家预约的崩铁式                        |
| 被动技能       | Passive Skill     | 触发条件型                              |
| 充能技能       | Charge Skill      | 自动累计型（默认）                      |
| 嘲讽值         | Threat            | AI 索敌权重                             |
| 行动值         | ActionPoint (AP)  | 标准 10000，按速度消耗                  |
| 速度           | Speed             | 行动值消耗速率，标准 100                |
| 角色（玩家方） | Hero              | 玩家可控角色                            |
| 角色（敌方）   | Enemy             | 敌人                                    |
| 单场战斗       | Combat            | 一波战斗（避开 Battle 歧义）            |
| 一局游戏       | Run               | Roguelite 单次游戏过程                  |
| 商店           | Shop              | 准备阶段刷新技能的地方                  |
| 准备阶段       | Preparation Phase | 战斗之间的购买阶段                      |
| 战斗阶段       | Combat Phase      | 自动战斗进行中                          |
| 章节           | Chapter           | 6 波为一章                              |
| 波次           | Wave              | 单场战斗                                |
| 派系亲和       | Faction Affinity  | 角色对某派系技能的加成                  |

---

## 5. 命名空间与日志分类

C++ 业务命名空间（可选用）：`Stuff::Combat::`, `Stuff::Card::` 等。MVP 阶段不强制。

日志分类**必须**在 `Source/Stuff/Core/StuffLogChannels.h` 集中声明：

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogStuff, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStuffCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStuffSkill, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStuffMatch, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStuffUI, Log, All);
```

---

## 6. 测试策略

- **MVP 阶段不强制单元测试**
- 关键不变量（伤害公式、行动值排序、嘲讽算法）可在需要时写 `Source/StuffTests/`（独立模块）
- 不要主动给 utility 函数补测试，除非用户明确要求

---

## 7. Git 工作流

- 仓库根目录为项目根（`Stuff.uproject` 所在目录）。新会话开始、提交前、切换任务前都先看 `git status --short --branch`。
- 默认跟踪：`Source/`、`Config/`、`Content/Docs/`、项目级 `.vscode/` 任务脚本、`.uproject`、`.editorconfig`、`.vsconfig`、`.gitignore`、`.gitattributes`。
- 默认忽略：`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`、`.sln` / `.vcxproj*` 等 UE/IDE 生成物。
- UE 二进制资产（`.uasset` / `.umap` / 贴图 / 音频 / DCC 源文件等）走 Git LFS（规则见根目录 `.gitattributes`）。
- 提交前只 stage 本次任务相关文件；不要把用户未要求的本地改动顺手塞进提交。
- 任何 `git reset`、`git checkout -- <path>`、批量删除、改写历史操作都必须先确认意图；不要为了"干净"丢弃未知改动。
- 初始化仓库后若本机没有 `user.name` / `user.email`，先让用户设置身份，再创建正式提交。

---

## 8. 提交粒度

- 一个 Plan 通常对应 1-3 个 git commit
- 单 commit **净增 ≤ 500 行**（除非纯样板/生成代码）
- commit message 包含 plan ID：
  - `[plan-001] add UCardSkill base class`
  - `[plan-002] implement turn order via action points`
- 中文 / 英文均可

---

## 9. 文件 / 目录创建准则

- 不主动创建 README 或文档文件，除非用户要求或本 AGENTS.md 明确规定
- 不创建 `.gitkeep` / 子目录 `.gitignore`（根目录 `.gitignore` 由 Git 基础设施维护）
- 不创建 example / sample 文件
- 不创建未来用得上的"占位"文件

---

## Last Updated

2026-06-14 — 初始化 Git 工作流：根目录 .gitignore/.gitattributes、LFS 规则、提交前状态检查与用户改动保护约束。
2026-04-25 — 初版。同步 v0.2 设计大纲与开发文档体系。
