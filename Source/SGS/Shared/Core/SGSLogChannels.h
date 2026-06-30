#pragma once

#include "CoreMinimal.h"

// 集中声明的日志分类。新增分类必须同时在此声明、在 SGSLogChannels.cpp 定义，
// 并登记到 Rulers.md 第 4 节。
DECLARE_LOG_CATEGORY_EXTERN(LogSGS, Log, All);       // 通用
DECLARE_LOG_CATEGORY_EXTERN(LogSGSCard, Log, All);   // 牌堆 / 手牌 / 用牌
DECLARE_LOG_CATEGORY_EXTERN(LogSGSSkill, Log, All);  // 技能 / 触发 / 效果
DECLARE_LOG_CATEGORY_EXTERN(LogSGSTurn, Log, All);   // 回合 / 阶段 / 结算流程
DECLARE_LOG_CATEGORY_EXTERN(LogSGSCombat, Log, All); // 伤害 / 回复 / 濒死
DECLARE_LOG_CATEGORY_EXTERN(LogSGSNet, Log, All);    // 复制 / RPC / 决策通道
DECLARE_LOG_CATEGORY_EXTERN(LogSGSAI, Log, All);     // AI 决策代理
DECLARE_LOG_CATEGORY_EXTERN(LogSGSUI, Log, All);     // 表现层
