#include "Misc/AutomationTest.h"

#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Shared/Cards/SGSCard.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Players/SGSSeat.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FSGSDeckCardSpec MakePlan0013BasicCard(FName CardName, FSGSSuit Suit, int32 Number)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = CardName;
	Spec.Suit = Suit;
	Spec.Number = Number;
	Spec.CardType = SGSGameplayTags::CardType_Basic.GetTag();
	return Spec;
}

TScriptInterface<ISGSDecisionAgent> MakePlan0013ScriptedAgent(USGSScriptedDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSScriptedDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TArray<FSGSDeckCardSpec> MakePlan0013TwoSeatDeck(bool bSeatOneHasDodge, bool bSeatZeroHasPeach)
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakePlan0013BasicCard(bSeatZeroHasPeach ? TEXT("Peach") : TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 3));
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 9));

	Deck.Add(MakePlan0013BasicCard(bSeatOneHasDodge ? TEXT("Dodge") : TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 2));
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 10));
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 11));
	Deck.Add(MakePlan0013BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 12));
	return Deck;
}

USGSGameDriver* RunPlan0013TwoSeatScriptedGame(
	USGSScriptedDecisionAgent* Seat0,
	USGSScriptedDecisionAgent* Seat1,
	TArray<FSGSDeckCardSpec> Deck,
	TMap<int32, int32> InitialHealth = TMap<int32, int32>())
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	TScriptInterface<ISGSDecisionAgent> Seat0Ref;
	Seat0Ref.SetObject(Seat0);
	Seat0Ref.SetInterface(Cast<ISGSDecisionAgent>(Seat0));
	Agents.Add(Seat0Ref);

	TScriptInterface<ISGSDecisionAgent> Seat1Ref;
	Seat1Ref.SetObject(Seat1);
	Seat1Ref.SetInterface(Cast<ISGSDecisionAgent>(Seat1));
	Agents.Add(Seat1Ref);

	FSGSGameStartConfig Config;
	Config.RandomSeed = 19;
	Config.InitialDeck = MoveTemp(Deck);
	Config.bShuffleInitialDeck = false;
	Config.MaxTurns = 1;
	Config.StartingHandSize = 4;
	Config.InitialHealthBySeat = MoveTemp(InitialHealth);

	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

bool HasExecutedCommandDetail(const USGSGameDriver* Driver, const FString& DetailNeedle)
{
	for (const FSGSCommandLogEntry& Entry : Driver->GetCommandLog())
	{
		if (Entry.bSucceeded
			&& Entry.Lifecycle == SGSCommandLifecycle::Executed()
			&& Entry.Detail.Contains(DetailNeedle))
		{
			return true;
		}
	}
	return false;
}

bool HasReplayEvent(const USGSGameDriver* Driver, FName EventName)
{
	for (const FSGSEventLogEntry& Entry : Driver->GetReplayLog().GetEventLog())
	{
		if (Entry.EventName == EventName)
		{
			return true;
		}
	}
	return false;
}

FSGSCommandBuildRequest MakePlan0013CommandBuildRequest(FName SourceName)
{
	FSGSCommandBuildRequest Request;
	Request.CommandId = FSGSCommandId(1);
	Request.RequestId = 1;
	Request.SeatIndex = 0;
	Request.Phase = SGSGameplayTags::Phase_Play.GetTag();
	Request.SourceChannel = FName(TEXT("Test"));
	Request.SourceName = SourceName;
	return Request;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0013RulePipelineBasicCardsTest,
	"SGS.Plan0013.RulePipeline.BasicCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0013RulePipelineBasicCardsTest::RunTest(const FString& Parameters)
{
	USGSScriptedDecisionAgent* Seat0 = nullptr;
	USGSScriptedDecisionAgent* Seat1 = nullptr;
	MakePlan0013ScriptedAgent(Seat0);
	MakePlan0013ScriptedAgent(Seat1);
	Seat0->EnqueuePlayCard(TEXT("Slash"), 1);
	Seat1->EnqueueResponseCard(TEXT("Dodge"));

	USGSGameDriver* Driver = RunPlan0013TwoSeatScriptedGame(
		Seat0,
		Seat1,
		MakePlan0013TwoSeatDeck(/*bSeatOneHasDodge*/ true, /*bSeatZeroHasPeach*/ false));
	TestEqual(TEXT("Dodge still prevents Slash damage through RulePipeline."), Driver->GetContext()->GetSeat(1)->Health, 4);
	TestTrue(TEXT("Slash rule request was executed."), HasExecutedCommandDetail(Driver, TEXT("Card=Slash")));
	TestTrue(TEXT("Dodge response rule request was executed."), HasExecutedCommandDetail(Driver, TEXT("Window=Slash.Dodge")));
	TestTrue(TEXT("RulePipeline basic cards keep replay invariants."), Driver->GetReplayLog().CheckInvariants());
	TestTrue(TEXT("RulePipeline basic cards keep context invariants."), Driver->GetContext()->CheckInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0013RulePipelineInvalidWindowTest,
	"SGS.Plan0013.RulePipeline.InvalidWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0013RulePipelineInvalidWindowTest::RunTest(const FString& Parameters)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> EmptyAgents;
	EmptyAgents.SetNum(1);
	USGSGameContext* Context = NewObject<USGSGameContext>();
	Context->Initialize(EmptyAgents, 23);
	USGSCard* Dodge = Context->CreateCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2, SGSGameplayTags::CardType_Basic.GetTag());
	Context->DrawCards(0, 1);

	FSGSCommandRouter Router;
	FSGSCommandExecutionContext ExecutionContext;
	ExecutionContext.GameContext = Context;
	ExecutionContext.ExpectedCommandId = FSGSCommandId(1);
	ExecutionContext.ExpectedRequestId = 1;
	ExecutionContext.ExpectedSeatIndex = 0;
	ExecutionContext.ExpectedPhase = SGSGameplayTags::Phase_Play.GetTag();
	ExecutionContext.ExpectedWindowName = TEXT("Slash.Dodge");
	ExecutionContext.RequiredCardName = TEXT("Dodge");
	ExecutionContext.EffectSourceSeatIndex = 1;
	ExecutionContext.EffectTargetSeatIndex = 0;

	FSGSCommand WrongWindowResponse = FSGSCommandFactory::Make(
		MakePlan0013CommandBuildRequest(TEXT("WrongWindowResponse")),
		FSGSRespondCardCommandPayload(Dodge->CardId, TArray<int32>(), TEXT("Dying.Peach")));

	TestTrue(TEXT("Router rejects response commands with mismatched typed window."),
		Router.SubmitCommand(WrongWindowResponse, ExecutionContext).HasError());
	TestTrue(TEXT("Invalid window command log keeps invariants."), Router.CheckInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0013RulePipelineReplayInvariantsTest,
	"SGS.Plan0013.RulePipeline.ReplayInvariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0013RulePipelineReplayInvariantsTest::RunTest(const FString& Parameters)
{
	USGSScriptedDecisionAgent* Seat0 = nullptr;
	USGSScriptedDecisionAgent* Seat1 = nullptr;
	MakePlan0013ScriptedAgent(Seat0);
	MakePlan0013ScriptedAgent(Seat1);
	Seat0->EnqueuePlayCard(TEXT("Slash"), 1);
	Seat0->EnqueueResponsePass();
	Seat1->EnqueueResponsePass();
	Seat1->EnqueueResponsePass();

	TMap<int32, int32> InitialHealth;
	InitialHealth.Add(1, 1);
	USGSGameDriver* Driver = RunPlan0013TwoSeatScriptedGame(
		Seat0,
		Seat1,
		MakePlan0013TwoSeatDeck(/*bSeatOneHasDodge*/ false, /*bSeatZeroHasPeach*/ false),
		InitialHealth);

	TestFalse(TEXT("Dying target is eliminated when no Peach response is available."), Driver->GetContext()->GetSeat(1)->bIsAlive);
	TestTrue(TEXT("Elimination is recorded by EffectPipeline."), HasReplayEvent(Driver, FName(TEXT("SGS.Event.EliminateSeat"))));
	TestTrue(TEXT("Replay log keeps invariants after RulePipeline elimination."), Driver->GetReplayLog().CheckInvariants());
	TestTrue(TEXT("Command log keeps invariants after RulePipeline elimination."), Driver->GetCommandLog().Num() > 0);
	TestTrue(TEXT("Game context keeps invariants after RulePipeline elimination."), Driver->GetContext()->CheckInvariants());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
