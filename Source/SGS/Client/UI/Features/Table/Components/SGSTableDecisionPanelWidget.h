#pragma once

#include "Client/UI/Features/Table/Components/SGSTableComponents.h"

// NoName 风格的统一选择/响应面板。它只消费已派生 Props，并通过显式
// callback 上报技能、确认和拒绝；牌与目标合法性仍由 Table State 提供。
class SGS_API SSGSTableDecisionPanelWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableDecisionPanelWidget) {}
		SLATE_ARGUMENT(FSGSTableDecisionBarProps, Props)
		SLATE_EVENT(FSGSOnTableSkillClicked, OnSkillClicked)
		SLATE_EVENT(FOnClicked, OnConfirmClicked)
		SLATE_EVENT(FOnClicked, OnPassClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
