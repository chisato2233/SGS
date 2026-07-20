#include "Server/Effects/SGSStandardEffectSteps.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Engine/SGSGameContext.h"

namespace
{
void AppendStandardEvent(FSGSEffectContext& Context, FName EventName, FString Payload)
{
	if (Context.ReplayLog != nullptr)
	{
		Context.ReplayLog->AppendEvent(
			EventName,
			Context.CommandId,
			Context.CurrentStepId,
			Context.TimingPoint,
			MoveTemp(Payload));
	}
}

void AppendCardMoveEvent(
	FSGSEffectContext& Context,
	FName Reason,
	const TArray<USGSCard*>& Cards,
	FSGSCardZone FromZone,
	int32 FromSeat,
	FSGSCardZone ToZone,
	int32 ToSeat,
	TArray<int32> RelatedTargetSeatIndices)
{
	if (Context.ReplayLog == nullptr || Cards.IsEmpty())
	{
		return;
	}

	FSGSCardMoveEventPayload Payload;
	Payload.Reason = Reason;
	Payload.FromZone = FromZone;
	Payload.FromSeat = FromSeat;
	Payload.ToZone = ToZone;
	Payload.ToSeat = ToSeat;
	Payload.RelatedTargetSeatIndices = MoveTemp(RelatedTargetSeatIndices);
	Payload.CardIds.Reserve(Cards.Num());
	for (const USGSCard* Card : Cards)
	{
		if (Card != nullptr)
		{
			Payload.CardIds.Add(Card->CardId);
		}
	}
	if (!Payload.CardIds.IsEmpty())
	{
		Context.ReplayLog->AppendCardMoveEvent(
			Context.CommandId,
			Context.CurrentStepId,
			Context.TimingPoint,
			MoveTemp(Payload));
	}
}
}

FName SGSCardMoveReasons::InitialDeal() { return FName(TEXT("SGS.CardMove.InitialDeal")); }
FName SGSCardMoveReasons::Draw() { return FName(TEXT("SGS.CardMove.Draw")); }
FName SGSCardMoveReasons::RewardDraw() { return FName(TEXT("SGS.CardMove.RewardDraw")); }
FName SGSCardMoveReasons::Use() { return FName(TEXT("SGS.CardMove.Use")); }
FName SGSCardMoveReasons::Respond() { return FName(TEXT("SGS.CardMove.Respond")); }
FName SGSCardMoveReasons::Discard() { return FName(TEXT("SGS.CardMove.Discard")); }
FName SGSCardMoveReasons::Cleanup() { return FName(TEXT("SGS.CardMove.Cleanup")); }

FSGSEffectStep SGSStandardEffectSteps::MakeMoveCardsStep(
	TArray<USGSCard*> Cards,
	FSGSCardZone FromZone,
	int32 FromSeat,
	FSGSCardZone ToZone,
	int32 ToSeat,
	FSGSCardMoveEventMetadata Metadata)
{
	if (Metadata.Reason.IsNone())
	{
		Metadata.Reason = SGSCardMoveReasons::Discard();
	}
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.MoveCards"));
	Step.SourceName = FName(TEXT("StandardEffect.MoveCards"));
	Step.Execute = [Cards = MoveTemp(Cards), FromZone, FromSeat, ToZone, ToSeat, Metadata = MoveTemp(Metadata)](FSGSEffectContext& Context)
	{
		if (Context.GameContext == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingContext")),
				TEXT("MoveCards step has no GameContext.")));
		}

		Context.GameContext->MoveCards(Cards, FromZone, FromSeat, ToZone, ToSeat);
		AppendCardMoveEvent(
			Context,
			Metadata.Reason,
			Cards,
			FromZone,
			FromSeat,
			ToZone,
			ToSeat,
			Metadata.RelatedTargetSeatIndices);
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeDrawCardsStep(
	int32 SeatIndex,
	int32 Count,
	FSGSCardMoveEventMetadata Metadata)
{
	if (Metadata.Reason.IsNone())
	{
		Metadata.Reason = SGSCardMoveReasons::Draw();
	}
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.DrawCards"));
	Step.SourceName = FName(TEXT("StandardEffect.DrawCards"));
	Step.Execute = [SeatIndex, Count, Metadata = MoveTemp(Metadata)](FSGSEffectContext& Context)
	{
		if (Context.GameContext == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingContext")),
				TEXT("DrawCards step has no GameContext.")));
		}

		const int32 DrawPileCountBefore = Context.GameContext->GetCardsInZone(
			SGSGameplayTags::CardZone_DrawPile.GetTag()).Num();
		TArray<USGSCard*> RecycledCards;
		if (DrawPileCountBefore < Count)
		{
			RecycledCards = Context.GameContext->GetCardsInZone(
				SGSGameplayTags::CardZone_DiscardPile.GetTag());
		}
		const TArray<USGSCard*> DrawnCards = Context.GameContext->DrawCards(SeatIndex, Count);
		if (!RecycledCards.IsEmpty())
		{
			AppendCardMoveEvent(
				Context,
				SGSCardMoveReasons::Cleanup(),
				RecycledCards,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_DrawPile.GetTag(),
				INDEX_NONE,
				{});
		}
		AppendCardMoveEvent(
			Context,
			Metadata.Reason,
			DrawnCards,
			SGSGameplayTags::CardZone_DrawPile.GetTag(),
			INDEX_NONE,
			SGSGameplayTags::CardZone_Hand.GetTag(),
			SeatIndex,
			Metadata.RelatedTargetSeatIndices);
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeDamageStep(int32 SourceSeat, int32 TargetSeat, int32 Amount)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.Damage"));
	Step.SourceName = FName(TEXT("StandardEffect.Damage"));
	Step.Execute = [SourceSeat, TargetSeat, Amount](FSGSEffectContext& Context)
	{
		if (Context.GameContext == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingContext")),
				TEXT("Damage step has no GameContext.")));
		}

		Context.GameContext->ApplyDamage(SourceSeat, TargetSeat, Amount);
		AppendStandardEvent(Context, FName(TEXT("SGS.Event.Damage")), FString::Printf(
			TEXT("Source=%d Target=%d Amount=%d"),
			SourceSeat,
			TargetSeat,
			Amount));
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeHealStep(int32 SeatIndex, int32 Amount)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.Heal"));
	Step.SourceName = FName(TEXT("StandardEffect.Heal"));
	Step.Execute = [SeatIndex, Amount](FSGSEffectContext& Context)
	{
		if (Context.GameContext == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingContext")),
				TEXT("Heal step has no GameContext.")));
		}

		Context.GameContext->Heal(SeatIndex, Amount);
		AppendStandardEvent(Context, FName(TEXT("SGS.Event.Heal")), FString::Printf(
			TEXT("Seat=%d Amount=%d"),
			SeatIndex,
			Amount));
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeEliminateSeatStep(int32 SeatIndex, int32 SourceSeat, FName Reason)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.EliminateSeat"));
	Step.SourceName = FName(TEXT("StandardEffect.EliminateSeat"));
	Step.Execute = [SeatIndex, SourceSeat, Reason](FSGSEffectContext& Context)
	{
		if (Context.GameContext == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingContext")),
				TEXT("EliminateSeat step has no GameContext.")));
		}

		Context.GameContext->EliminateSeat(SeatIndex, SourceSeat, Reason);
		AppendStandardEvent(Context, FName(TEXT("SGS.Event.EliminateSeat")), FString::Printf(
			TEXT("Seat=%d Reason=%s"),
			SeatIndex,
			*Reason.ToString()));
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeJudgementPlaceholderStep(int32 SeatIndex, FName Reason)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.Judgement"));
	Step.SourceName = FName(TEXT("StandardEffect.Judgement"));
	Step.Execute = [SeatIndex, Reason](FSGSEffectContext& Context)
	{
		AppendStandardEvent(Context, FName(TEXT("SGS.Event.JudgementRequested")), FString::Printf(
			TEXT("Seat=%d Reason=%s"),
			SeatIndex,
			*Reason.ToString()));
		return FSGSEffectResult::Success();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeReactionWindowStep(int32 SeatIndex, FName WindowName)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.ReactionWindow"));
	Step.SourceName = FName(TEXT("StandardEffect.ReactionWindow"));
	Step.bCanSuspend = true;
	Step.Execute = [SeatIndex, WindowName](FSGSEffectContext& Context)
	{
		AppendStandardEvent(Context, FName(TEXT("SGS.Event.ReactionWindowOpened")), FString::Printf(
			TEXT("Seat=%d Window=%s"),
			SeatIndex,
			*WindowName.ToString()));
		return FSGSEffectResult::Suspend();
	};
	return Step;
}

FSGSEffectStep SGSStandardEffectSteps::MakeExpireActiveEffectStep(FSGSStableHandle EffectHandle, FName Reason)
{
	FSGSEffectStep Step;
	Step.StepName = FName(TEXT("SGS.Effect.ExpireActiveEffect"));
	Step.SourceName = FName(TEXT("StandardEffect.ActiveEffect"));
	Step.Execute = [EffectHandle, Reason](FSGSEffectContext& Context)
	{
		if (Context.ActiveEffects == nullptr)
		{
			return FSGSEffectResult::Failure(FSGSError::Make(
				FName(TEXT("SGS.Effect.MissingActiveEffectTimeline")),
				TEXT("ExpireActiveEffect step has no ActiveEffectTimeline.")));
		}

		if (FSGSStatus Status = Context.ActiveEffects->Expire(EffectHandle, Context.TimingPoint, Reason); Status.HasError())
		{
			return FSGSEffectResult::Failure(Status.GetError());
		}

		AppendStandardEvent(Context, FName(TEXT("SGS.Event.ActiveEffectExpired")), FString::Printf(
			TEXT("Effect=%s Reason=%s"),
			*EffectHandle.ToLogString(),
			*Reason.ToString()));
		return FSGSEffectResult::Success();
	};
	return Step;
}
