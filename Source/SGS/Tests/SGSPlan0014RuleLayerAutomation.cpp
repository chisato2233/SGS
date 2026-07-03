#include "Misc/AutomationTest.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/SGSRuleInvocation.h"
#include "Server/Rules/SGSRuleRegistry.h"
#include "Server/Rules/SGSResolutionStack.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
USGSGameContext* MakePlan0014Context(int32 SeatCount = 2)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.SetNum(SeatCount);

	USGSGameContext* Context = NewObject<USGSGameContext>();
	Context->Initialize(Agents, 14);
	return Context;
}

FSGSCommandBuildRequest MakePlan0014CommandBuildRequest(FName SourceName, int32 SeatIndex = 0)
{
	FSGSCommandBuildRequest Request;
	Request.CommandId = FSGSCommandId(1);
	Request.RequestId = 1;
	Request.SeatIndex = SeatIndex;
	Request.Phase = SGSGameplayTags::Phase_Play.GetTag();
	Request.SourceChannel = FName(TEXT("Test"));
	Request.SourceName = SourceName;
	return Request;
}

FSGSCommandExecutionContext MakePlan0014ExecutionContext(USGSGameContext* Context, int32 SeatIndex = 0)
{
	FSGSCommandExecutionContext ExecutionContext;
	ExecutionContext.GameContext = Context;
	ExecutionContext.ExpectedCommandId = FSGSCommandId(1);
	ExecutionContext.ExpectedRequestId = 1;
	ExecutionContext.ExpectedSeatIndex = SeatIndex;
	ExecutionContext.ExpectedPhase = SGSGameplayTags::Phase_Play.GetTag();
	return ExecutionContext;
}

FSGSRuleInvocation MakePlan0014LookupInvocation(
	FName RuleKindTag,
	FGameplayTag IntentTag,
	FName SubjectName,
	FName WindowName)
{
	FSGSPassRulePayload Payload;
	Payload.WindowName = WindowName;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = RuleKindTag;
	Invocation.IntentTag = IntentTag;
	Invocation.SubjectName = SubjectName;
	Invocation.ActorSeat = 0;
	Invocation.WindowName = WindowName;
	Invocation.SourceCommandId = FSGSCommandId(1);
	Invocation.SourceRequestId = 1;
	Invocation.Payload = FInstancedStruct::Make(Payload);
	return Invocation;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0014RuleInvocationBasicPayloadsTest,
	"SGS.Plan0014.RuleInvocation.BasicPayloads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0014RuleInvocationBasicPayloadsTest::RunTest(const FString& Parameters)
{
	USGSGameContext* Context = MakePlan0014Context();
	USGSCard* Slash = Context->CreateCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7, SGSGameplayTags::CardType_Basic.GetTag());
	USGSCard* Dodge = Context->CreateCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2, SGSGameplayTags::CardType_Basic.GetTag());
	Context->DrawCards(0, 2);

	FSGSCommandRouter Router;
	FSGSCommandExecutionContext ExecutionContext = MakePlan0014ExecutionContext(Context);

	const FSGSCommand PassCommand = FSGSCommandFactory::Make(
		MakePlan0014CommandBuildRequest(TEXT("Pass")),
		FSGSPassCommandPayload());
	TestFalse(TEXT("Pass command validates."), Router.SubmitCommand(PassCommand, ExecutionContext).HasError());
	TSGSResult<FSGSRuleInvocation> PassInvocationResult = Router.BuildRuleInvocation(PassCommand, ExecutionContext);
	TestTrue(TEXT("Pass builds a rule invocation."), PassInvocationResult.HasValue());
	if (PassInvocationResult.HasValue())
	{
		const FSGSRuleInvocation& Invocation = PassInvocationResult.GetValue();
		TestEqual(TEXT("Pass invocation kind is action."), Invocation.RuleKindTag, SGSRuleKinds::Action());
		TestEqual(TEXT("Pass invocation subject is none."), Invocation.SubjectName, NAME_None);
		TestNotNull(TEXT("Pass invocation has pass payload."), Invocation.GetPayload<FSGSPassRulePayload>());
		TestTrue(TEXT("Pass invocation keeps invariants."), Invocation.CheckInvariants());
	}

	const FSGSCommand UseCardCommand = FSGSCommandFactory::Make(
		MakePlan0014CommandBuildRequest(TEXT("UseSlash")),
		FSGSUseCardCommandPayload(Slash->CardId, TArray<int32>{ 1 }));
	TestFalse(TEXT("UseCard command validates."), Router.SubmitCommand(UseCardCommand, ExecutionContext).HasError());
	TSGSResult<FSGSRuleInvocation> UseCardInvocationResult = Router.BuildRuleInvocation(UseCardCommand, ExecutionContext);
	TestTrue(TEXT("UseCard builds a rule invocation."), UseCardInvocationResult.HasValue());
	if (UseCardInvocationResult.HasValue())
	{
		const FSGSRuleInvocation& Invocation = UseCardInvocationResult.GetValue();
		const FSGSUseCardRulePayload* Payload = Invocation.GetPayload<FSGSUseCardRulePayload>();
		TestEqual(TEXT("UseCard invocation subject is Slash."), Invocation.SubjectName, FName(TEXT("Slash")));
		TestNotNull(TEXT("UseCard invocation has use-card payload."), Payload);
		if (Payload != nullptr)
		{
			TestEqual(TEXT("UseCard payload keeps the card id."), Payload->CardId, Slash->CardId);
			TestEqual(TEXT("UseCard payload keeps first target."), Payload->TargetSeatIndices[0], 1);
		}
		TestTrue(TEXT("UseCard invocation keeps invariants."), Invocation.CheckInvariants());
	}

	FSGSCommandExecutionContext ResponseContext = ExecutionContext;
	ResponseContext.ExpectedWindowName = FName(TEXT("Slash.Dodge"));
	ResponseContext.RequiredCardName = FName(TEXT("Dodge"));
	ResponseContext.EffectSourceSeatIndex = 1;
	ResponseContext.EffectTargetSeatIndex = 0;

	const FSGSCommand RespondCommand = FSGSCommandFactory::Make(
		MakePlan0014CommandBuildRequest(TEXT("RespondDodge")),
		FSGSRespondCardCommandPayload(Dodge->CardId, TArray<int32>(), FName(TEXT("Slash.Dodge"))));
	TestFalse(TEXT("RespondCard command validates."), Router.SubmitCommand(RespondCommand, ResponseContext).HasError());
	TSGSResult<FSGSRuleInvocation> RespondInvocationResult = Router.BuildRuleInvocation(RespondCommand, ResponseContext);
	TestTrue(TEXT("RespondCard builds a rule invocation."), RespondInvocationResult.HasValue());
	if (RespondInvocationResult.HasValue())
	{
		const FSGSRuleInvocation& Invocation = RespondInvocationResult.GetValue();
		const FSGSRespondCardRulePayload* Payload = Invocation.GetPayload<FSGSRespondCardRulePayload>();
		TestEqual(TEXT("Respond invocation kind is response."), Invocation.RuleKindTag, SGSRuleKinds::Response());
		TestEqual(TEXT("Respond invocation subject is Dodge."), Invocation.SubjectName, FName(TEXT("Dodge")));
		TestEqual(TEXT("Respond invocation window is Slash.Dodge."), Invocation.WindowName, FName(TEXT("Slash.Dodge")));
		TestNotNull(TEXT("Respond invocation has respond-card payload."), Payload);
		TestTrue(TEXT("Respond invocation keeps invariants."), Invocation.CheckInvariants());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0014RuleRegistryIndexedLookupTest,
	"SGS.Plan0014.RuleRegistry.IndexedLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0014RuleRegistryIndexedLookupTest::RunTest(const FString& Parameters)
{
	FSGSRuleRegistry Registry;
	TestTrue(TEXT("Default rule registry keeps invariants."), Registry.CheckInvariants());

	const TArray<FName> SlashCandidates = Registry.FindCandidateRuleNames(MakePlan0014LookupInvocation(
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Slash")),
		NAME_None));
	TestTrue(TEXT("Slash lookup finds Slash rule."), SlashCandidates.Contains(FName(TEXT("SGS.Rule.Slash"))));
	TestEqual(TEXT("Slash lookup is sorted with Slash first."), SlashCandidates.Num() > 0 ? SlashCandidates[0] : NAME_None, FName(TEXT("SGS.Rule.Slash")));

	const TArray<FName> PeachCandidates = Registry.FindCandidateRuleNames(MakePlan0014LookupInvocation(
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Peach")),
		NAME_None));
	TestTrue(TEXT("Peach lookup finds Peach rule."), PeachCandidates.Contains(FName(TEXT("SGS.Rule.Peach"))));
	TestEqual(TEXT("Peach lookup is sorted with Peach first."), PeachCandidates.Num() > 0 ? PeachCandidates[0] : NAME_None, FName(TEXT("SGS.Rule.Peach")));

	const TArray<FName> DodgeCandidates = Registry.FindCandidateRuleNames(MakePlan0014LookupInvocation(
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Dodge")),
		FName(TEXT("Slash.Dodge"))));
	TestTrue(TEXT("Slash.Dodge lookup finds DodgeResponse rule."), DodgeCandidates.Contains(FName(TEXT("SGS.Rule.DodgeResponse"))));
	TestEqual(TEXT("Slash.Dodge lookup is sorted with DodgeResponse first."), DodgeCandidates.Num() > 0 ? DodgeCandidates[0] : NAME_None, FName(TEXT("SGS.Rule.DodgeResponse")));

	const TArray<FName> DyingPassCandidates = Registry.FindCandidateRuleNames(MakePlan0014LookupInvocation(
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		FName(TEXT("Dying.Peach"))));
	TestTrue(TEXT("Dying.Peach pass lookup finds DyingPeachPass rule."), DyingPassCandidates.Contains(FName(TEXT("SGS.Rule.DyingPeachPass"))));
	TestEqual(TEXT("Dying.Peach lookup is sorted with DyingPeachPass first."), DyingPassCandidates.Num() > 0 ? DyingPassCandidates[0] : NAME_None, FName(TEXT("SGS.Rule.DyingPeachPass")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0014ResolutionStackPopResumeTest,
	"SGS.Plan0014.ResolutionStack.PopResume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0014ResolutionStackPopResumeTest::RunTest(const FString& Parameters)
{
	FSGSResolutionStack Stack;

	FSGSSlashResolutionState SlashState;
	SlashState.SlashCardId = 100;
	SlashState.SourceSeat = 0;
	SlashState.TargetSeat = 1;

	FSGSResolutionFrame SlashFrame;
	SlashFrame.SourceRuleName = FName(TEXT("SGS.Rule.Slash"));
	SlashFrame.SourceCommandId = FSGSCommandId(1);
	SlashFrame.ActorSeat = 0;
	SlashFrame.SourceSeat = 0;
	SlashFrame.TargetSeat = 1;
	SlashFrame.ProcessingCardId = 100;
	SlashFrame.OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
	SlashFrame.FrameState = FInstancedStruct::Make(SlashState);
	const FSGSStableHandle SlashHandle = Stack.PushFrame(MoveTemp(SlashFrame));

	FSGSDyingPeachResolutionState DyingState;
	DyingState.DyingSeat = 1;
	DyingState.ResponderSeatIndices = TArray<int32>{ 1, 0 };

	FSGSResolutionFrame DyingFrame;
	DyingFrame.SourceRuleName = FName(TEXT("SGS.Rule.DyingPeach"));
	DyingFrame.SourceCommandId = FSGSCommandId(2);
	DyingFrame.ActorSeat = 1;
	DyingFrame.SourceSeat = 0;
	DyingFrame.TargetSeat = 1;
	DyingFrame.WindowName = FName(TEXT("Dying.Peach"));
	DyingFrame.RequiredCardName = FName(TEXT("Peach"));
	DyingFrame.FrameState = FInstancedStruct::Make(DyingState);
	const FSGSStableHandle DyingHandle = Stack.PushFrame(MoveTemp(DyingFrame));

	TestEqual(TEXT("Dying frame parent is Slash."), Stack.GetParentFrameHandle(DyingHandle), SlashHandle);
	const FSGSResolutionFrame* CurrentChildFrame = Stack.GetCurrentFrame();
	TestNotNull(TEXT("Current frame exists."), CurrentChildFrame);
	if (CurrentChildFrame != nullptr)
	{
		TestNotNull(TEXT("Current frame is child Dying."), CurrentChildFrame->GetState<FSGSDyingPeachResolutionState>());
	}

	TSGSResult<FSGSResolutionFrame> CompletedChild = Stack.CompleteCurrentFrame(FName(TEXT("Test.ChildComplete")));
	TestTrue(TEXT("Completing child frame succeeds."), CompletedChild.HasValue());
	TestEqual(TEXT("Parent Slash resumes as current frame."), Stack.GetCurrentFrameHandle(), SlashHandle);
	const FSGSResolutionFrame* ResumedParentFrame = Stack.GetCurrentFrame();
	TestNotNull(TEXT("Resumed parent frame exists."), ResumedParentFrame);
	if (ResumedParentFrame != nullptr)
	{
		TestNotNull(TEXT("Resumed parent keeps Slash state."), ResumedParentFrame->GetState<FSGSSlashResolutionState>());
	}
	TestTrue(TEXT("Stack invariants hold after child pop."), Stack.CheckInvariants());

	TSGSResult<FSGSResolutionFrame> CompletedParent = Stack.CompleteCurrentFrame(FName(TEXT("Test.ParentComplete")));
	TestTrue(TEXT("Completing parent frame succeeds."), CompletedParent.HasValue());
	TestFalse(TEXT("Stack is empty after parent completion."), Stack.GetCurrentFrameHandle().IsValid());
	TestFalse(TEXT("Completing an empty stack fails cleanly."), Stack.CompleteCurrentFrame(FName(TEXT("Test.EmptyComplete"))).HasValue());
	TestTrue(TEXT("Empty stack keeps invariants."), Stack.CheckInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0014ResolutionStackNestedDyingTest,
	"SGS.Plan0014.ResolutionStack.NestedDying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0014ResolutionStackNestedDyingTest::RunTest(const FString& Parameters)
{
	FSGSResolutionStack Stack;

	FSGSDyingPeachResolutionState ParentDyingState;
	ParentDyingState.DyingSeat = 1;
	ParentDyingState.ResponderSeatIndices = TArray<int32>{ 1, 0 };

	FSGSResolutionFrame ParentDyingFrame;
	ParentDyingFrame.SourceRuleName = FName(TEXT("SGS.Rule.DyingPeach"));
	ParentDyingFrame.SourceCommandId = FSGSCommandId(1);
	ParentDyingFrame.ActorSeat = 1;
	ParentDyingFrame.SourceSeat = 0;
	ParentDyingFrame.TargetSeat = 1;
	ParentDyingFrame.WindowName = FName(TEXT("Dying.Peach"));
	ParentDyingFrame.RequiredCardName = FName(TEXT("Peach"));
	ParentDyingFrame.OnChildCompletedContinuation = SGSResolutionContinuations::ResumeDyingPeach();
	ParentDyingFrame.FrameState = FInstancedStruct::Make(ParentDyingState);
	const FSGSStableHandle ParentHandle = Stack.PushFrame(MoveTemp(ParentDyingFrame));

	FSGSDyingPeachResolutionState ChildDyingState;
	ChildDyingState.DyingSeat = 2;
	ChildDyingState.ResponderSeatIndices = TArray<int32>{ 2, 1, 0 };

	FSGSResolutionFrame ChildDyingFrame;
	ChildDyingFrame.SourceRuleName = FName(TEXT("SGS.Rule.DyingPeach"));
	ChildDyingFrame.SourceCommandId = FSGSCommandId(2);
	ChildDyingFrame.ActorSeat = 2;
	ChildDyingFrame.SourceSeat = 1;
	ChildDyingFrame.TargetSeat = 2;
	ChildDyingFrame.WindowName = FName(TEXT("Dying.Peach"));
	ChildDyingFrame.RequiredCardName = FName(TEXT("Peach"));
	ChildDyingFrame.FrameState = FInstancedStruct::Make(ChildDyingState);
	Stack.PushFrame(MoveTemp(ChildDyingFrame));

	const FSGSResolutionFrame* LatestDyingFrame = Stack.FindLatestFrameWithState<FSGSDyingPeachResolutionState>();
	TestNotNull(TEXT("Latest dying frame exists."), LatestDyingFrame);
	if (LatestDyingFrame != nullptr)
	{
		TestEqual(TEXT("Latest dying frame is child seat."), LatestDyingFrame->TargetSeat, 2);
	}
	TestNotNull(TEXT("Can find parent dying frame by seat."), Stack.FindLatestDyingFrameForSeat(1));

	TSGSResult<FSGSResolutionFrame> CompletedChild = Stack.CompleteCurrentFrame(FName(TEXT("Test.ChildDyingComplete")));
	TestTrue(TEXT("Completing nested child dying succeeds."), CompletedChild.HasValue());
	TestEqual(TEXT("Parent dying resumes as current frame."), Stack.GetCurrentFrameHandle(), ParentHandle);

	FSGSResolutionFrame* DuplicateSeatFrame = Stack.FindLatestDyingFrameForSeat(1);
	TestNotNull(TEXT("Duplicate same-seat dying resolves to existing frame."), DuplicateSeatFrame);
	if (DuplicateSeatFrame != nullptr)
	{
		FSGSDyingPeachResolutionState* ExistingState = DuplicateSeatFrame->GetMutableState<FSGSDyingPeachResolutionState>();
		TestNotNull(TEXT("Existing dying state is mutable."), ExistingState);
		if (ExistingState != nullptr)
		{
			ExistingState->bNeedsHealthRecheck = true;
			TestTrue(TEXT("Existing same-seat frame can be marked for recheck."), ExistingState->bNeedsHealthRecheck);
		}
	}

	TestTrue(TEXT("Nested dying stack keeps invariants."), Stack.CheckInvariants());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
