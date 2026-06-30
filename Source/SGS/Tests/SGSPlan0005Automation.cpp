#include "Misc/AutomationTest.h"

#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Cards/SGSCard.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Players/SGSSeat.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FSGSDeckCardSpec MakePlan0005BasicCard(FName CardName, FSGSSuit Suit, int32 Number)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = CardName;
	Spec.Suit = Suit;
	Spec.Number = Number;
	Spec.CardType = SGSGameplayTags::CardType_Basic.GetTag();
	return Spec;
}

TScriptInterface<ISGSDecisionAgent> MakeScriptedAgent(USGSScriptedDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSScriptedDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TArray<FSGSDeckCardSpec> MakeTwoSeatDeck(bool bSeatOneHasDodge, bool bSeatZeroHasPeach)
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakePlan0005BasicCard(bSeatZeroHasPeach ? TEXT("Peach") : TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 3));
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 9));

	Deck.Add(MakePlan0005BasicCard(bSeatOneHasDodge ? TEXT("Dodge") : TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 2));
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 10));
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 11));
	Deck.Add(MakePlan0005BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 12));
	return Deck;
}

USGSGameDriver* RunTwoSeatScriptedGame(
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
	Config.RandomSeed = 7;
	Config.InitialDeck = MoveTemp(Deck);
	Config.bShuffleInitialDeck = false;
	Config.MaxTurns = 1;
	Config.StartingHandSize = 4;
	Config.InitialHealthBySeat = MoveTemp(InitialHealth);

	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

bool HasRejectedCommand(const USGSGameDriver* Driver)
{
	for (const FSGSCommandLogEntry& Entry : Driver->GetCommandLog())
	{
		if (!Entry.bSucceeded && Entry.Lifecycle == SGSCommandLifecycle::Rejected())
		{
			return true;
		}
	}
	return false;
}

FSGSCommandBuildRequest MakeTestCommandBuildRequest(FName SourceName)
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
	FSGSPlan0005SlashDodgeDamageTest,
	"SGS.Plan0005.BasicCards.SlashDodgeDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0005SlashDodgeDamageTest::RunTest(const FString& Parameters)
{
	USGSScriptedDecisionAgent* DodgeSeat0 = nullptr;
	USGSScriptedDecisionAgent* DodgeSeat1 = nullptr;
	MakeScriptedAgent(DodgeSeat0);
	MakeScriptedAgent(DodgeSeat1);
	DodgeSeat0->EnqueuePlayCard(TEXT("Slash"), 1);
	DodgeSeat1->EnqueueResponseCard(TEXT("Dodge"));

	USGSGameDriver* DodgeDriver = RunTwoSeatScriptedGame(
		DodgeSeat0,
		DodgeSeat1,
		MakeTwoSeatDeck(/*bSeatOneHasDodge*/ true, /*bSeatZeroHasPeach*/ false));
	TestNotNull(TEXT("Dodge driver created."), DodgeDriver);
	TestEqual(TEXT("Dodge prevents Slash damage."), DodgeDriver->GetContext()->GetSeat(1)->Health, 4);
	TestTrue(TEXT("Dodge scenario keeps invariants."), DodgeDriver->GetContext()->CheckInvariants());
	TestTrue(TEXT("Dodge scenario replay keeps invariants."), DodgeDriver->GetReplayLog().CheckInvariants());

	USGSScriptedDecisionAgent* HitSeat0 = nullptr;
	USGSScriptedDecisionAgent* HitSeat1 = nullptr;
	MakeScriptedAgent(HitSeat0);
	MakeScriptedAgent(HitSeat1);
	HitSeat0->EnqueuePlayCard(TEXT("Slash"), 1);
	HitSeat1->EnqueueResponsePass();

	USGSGameDriver* HitDriver = RunTwoSeatScriptedGame(
		HitSeat0,
		HitSeat1,
		MakeTwoSeatDeck(/*bSeatOneHasDodge*/ false, /*bSeatZeroHasPeach*/ false));
	TestEqual(TEXT("Pass on Dodge window takes one Slash damage."), HitDriver->GetContext()->GetSeat(1)->Health, 3);
	TestTrue(TEXT("Hit scenario keeps invariants."), HitDriver->GetContext()->CheckInvariants());
	TestTrue(TEXT("Hit scenario replay keeps invariants."), HitDriver->GetReplayLog().CheckInvariants());
	TestTrue(TEXT("Hit scenario records replay events."), HitDriver->GetReplayLog().GetEventLog().Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0005PeachAndDyingTest,
	"SGS.Plan0005.BasicCards.PeachAndDying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0005PeachAndDyingTest::RunTest(const FString& Parameters)
{
	USGSScriptedDecisionAgent* RescueSeat0 = nullptr;
	USGSScriptedDecisionAgent* RescueSeat1 = nullptr;
	MakeScriptedAgent(RescueSeat0);
	MakeScriptedAgent(RescueSeat1);
	RescueSeat0->EnqueuePlayCard(TEXT("Slash"), 1);
	RescueSeat0->EnqueueResponseCard(TEXT("Peach"), 1);
	RescueSeat1->EnqueueResponsePass(); // Dodge
	RescueSeat1->EnqueueResponsePass(); // Self Peach

	TMap<int32, int32> DyingHealth;
	DyingHealth.Add(1, 1);
	USGSGameDriver* RescueDriver = RunTwoSeatScriptedGame(
		RescueSeat0,
		RescueSeat1,
		MakeTwoSeatDeck(/*bSeatOneHasDodge*/ false, /*bSeatZeroHasPeach*/ true),
		DyingHealth);
	TestTrue(TEXT("Peach rescue keeps target alive."), RescueDriver->GetContext()->GetSeat(1)->bIsAlive);
	TestEqual(TEXT("Peach rescue restores target to one HP."), RescueDriver->GetContext()->GetSeat(1)->Health, 1);
	TestTrue(TEXT("Rescue scenario keeps invariants."), RescueDriver->GetContext()->CheckInvariants());

	USGSScriptedDecisionAgent* DeathSeat0 = nullptr;
	USGSScriptedDecisionAgent* DeathSeat1 = nullptr;
	MakeScriptedAgent(DeathSeat0);
	MakeScriptedAgent(DeathSeat1);
	DeathSeat0->EnqueuePlayCard(TEXT("Slash"), 1);
	DeathSeat0->EnqueueResponsePass(); // Other Peach
	DeathSeat1->EnqueueResponsePass(); // Dodge
	DeathSeat1->EnqueueResponsePass(); // Self Peach

	TMap<int32, int32> DeathHealth;
	DeathHealth.Add(1, 1);
	USGSGameDriver* DeathDriver = RunTwoSeatScriptedGame(
		DeathSeat0,
		DeathSeat1,
		MakeTwoSeatDeck(/*bSeatOneHasDodge*/ false, /*bSeatZeroHasPeach*/ false),
		DeathHealth);
	TestFalse(TEXT("No Peach eliminates dying target."), DeathDriver->GetContext()->GetSeat(1)->bIsAlive);
	TestEqual(TEXT("Eliminated target remains at zero HP."), DeathDriver->GetContext()->GetSeat(1)->Health, 0);
	TestTrue(TEXT("Death scenario keeps invariants."), DeathDriver->GetContext()->CheckInvariants());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0005IllegalCommandTest,
	"SGS.Plan0005.BasicCards.IllegalCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0005IllegalCommandTest::RunTest(const FString& Parameters)
{
	USGSScriptedDecisionAgent* Seat0 = nullptr;
	USGSScriptedDecisionAgent* Seat1 = nullptr;
	MakeScriptedAgent(Seat0);
	MakeScriptedAgent(Seat1);
	Seat0->EnqueueInvalidPlayCard(1);

	USGSGameDriver* Driver = RunTwoSeatScriptedGame(
		Seat0,
		Seat1,
		MakeTwoSeatDeck(/*bSeatOneHasDodge*/ false, /*bSeatZeroHasPeach*/ false));
	TestEqual(TEXT("Invalid play command does not damage target."), Driver->GetContext()->GetSeat(1)->Health, 4);
	TestTrue(TEXT("Invalid play command is rejected."), HasRejectedCommand(Driver));

	TArray<TScriptInterface<ISGSDecisionAgent>> EmptyAgents;
	EmptyAgents.SetNum(1);
	USGSGameContext* Context = NewObject<USGSGameContext>();
	Context->Initialize(EmptyAgents, 9);
	USGSCard* Dodge = Context->CreateCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2, SGSGameplayTags::CardType_Basic.GetTag());
	Context->DrawCards(0, 1);

	FSGSCommandRouter Router;
	FSGSCommandExecutionContext ExecutionContext;
	ExecutionContext.GameContext = Context;
	ExecutionContext.ExpectedCommandId = FSGSCommandId(1);
	ExecutionContext.ExpectedRequestId = 1;
	ExecutionContext.ExpectedSeatIndex = 0;
	ExecutionContext.ExpectedPhase = SGSGameplayTags::Phase_Play.GetTag();
	ExecutionContext.RequiredCardName = TEXT("Dodge");

	FSGSCommand ResponseOutsideWindow = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("ResponseOutsideWindow")),
		FSGSRespondCardCommandPayload(Dodge->CardId, TArray<int32>(), TEXT("Slash.Dodge")));
	TestTrue(TEXT("RespondCard outside a response window is rejected."),
		Router.SubmitCommand(ResponseOutsideWindow, ExecutionContext).HasError());

	FSGSCommand MissingUsePayload = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("MissingUsePayload")),
		FSGSUseCardCommandPayload(Dodge->CardId, TArray<int32>()));
	MissingUsePayload.Payload.Reset();
	TestTrue(TEXT("UseCard missing typed payload is rejected even when legacy mirrors are populated."),
		Router.SubmitCommand(MissingUsePayload, ExecutionContext).HasError());

	FSGSCommand WrongUsePayload = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("WrongUsePayload")),
		FSGSUseCardCommandPayload(Dodge->CardId, TArray<int32>()));
	WrongUsePayload.Payload = FInstancedStruct::Make<FSGSPassCommandPayload>();
	TestTrue(TEXT("UseCard with the wrong typed payload is rejected."),
		Router.SubmitCommand(WrongUsePayload, ExecutionContext).HasError());

	FSGSCommandExecutionContext ResponseExecutionContext = ExecutionContext;
	ResponseExecutionContext.ExpectedWindowName = TEXT("Slash.Dodge");
	FSGSCommand WindowMismatchResponse = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("WindowMismatchResponse")),
		FSGSRespondCardCommandPayload(Dodge->CardId, TArray<int32>(), TEXT("Dying.Peach")));
	TestTrue(TEXT("RespondCard with typed payload window mismatch is rejected."),
		Router.SubmitCommand(WindowMismatchResponse, ResponseExecutionContext).HasError());

	FSGSCommand DirtyPass = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("DirtyPass")),
		FSGSPassCommandPayload());
	DirtyPass.CardIds.Add(Dodge->CardId);
	TestTrue(TEXT("Pass command with illegal legacy card mirror is rejected."),
		Router.SubmitCommand(DirtyPass, ExecutionContext).HasError());

	FSGSCommand ValidTypedPass = FSGSCommandFactory::Make(
		MakeTestCommandBuildRequest(TEXT("ValidTypedPass")),
		FSGSPassCommandPayload());
	TestFalse(TEXT("Typed payload Pass command is accepted."),
		Router.SubmitCommand(ValidTypedPass, ExecutionContext).HasError());
	TestTrue(TEXT("Command log keeps invariants under typed payload commands."), Router.CheckInvariants());
	TestTrue(TEXT("Illegal command context keeps invariants."), Context->CheckInvariants());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
