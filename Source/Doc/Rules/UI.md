# UI Rules

本文件规定 UI 技术路线和边界，不描述现有控件或界面结构。需要 UI 代码关系时使用 graphify。

---

## 技术路线

- UI 主路线为 Native Code-first UI。
- Slate 作为主要自定义控件底座。
- UMG 只作为视口、生命周期和对象系统适配层。
- CommonUI 按需评估，用于菜单栈、输入路由、手柄/键鼠提示等明确需求。

---

## 禁止事项

- 不在编辑器 Designer 中拖控件搭主 UI。
- 不以 WebView、React、Vue、Noesis、Gameface 作为主 UI 技术栈。
- 不自研浏览器级 UI runtime，除非新 Plan 明确推翻当前路线。
- UI 不承载规则结算。

---

## 边界

- UI 读取复制状态、规则事件和可显示 ViewModel。
- UI 采集玩家意图，并通过合法命令/RPC 通道提交。
- UI 不直接修改权威规则状态、索引事实源、效果管线或其他规则事实源。
