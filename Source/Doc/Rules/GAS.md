# GAS Rules

本文件规定 GAS 使用边界，不描述当前 GAS 适配代码结构。需要查现有适配关系时使用 graphify。

---

## 定位

GAS 是 SGS 的底座之一，不是 SGS 规则核心本身。

优先用于：

- Attribute。
- GameplayEffect / Modifier。
- GameplayTag。
- GameplayCue / 表现桥接。
- 可复用效果载体。

---

## 不外包给 GAS 的部分

- Command 和服务器权威校验。
- 随机审计。
- 回放日志。
- 牌区移动。
- 响应窗口。
- 三国杀式结算顺序。
- EffectPipeline 的规则语义。

---

## 蓝图边界

- 不用蓝图 Ability 承载核心规则。
- 蓝图可服务表现、调试、资产绑定，但不得成为规则事实源。
- SGS ActiveEffect、Command、ReplayLog 与 GAS Effect 的所有权关系必须由 C++ 规则层明确控制。
