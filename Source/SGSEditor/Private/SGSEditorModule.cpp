#include "Containers/Queue.h"
#include "Containers/Ticker.h"
#include "Logging/MessageLog.h"
#include "MessageLogInitializationOptions.h"
#include "MessageLogModule.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "SGSEditor"

namespace
{
const FName SGSMessageLogName(TEXT("SGS"));

struct FSGSEditorLogEntry
{
	FName Category;
	ELogVerbosity::Type Verbosity = ELogVerbosity::Log;
	FString Message;
};

class FSGSEditorLogOutputDevice final : public FOutputDevice
{
public:
	virtual void Serialize(
		const TCHAR* Message,
		ELogVerbosity::Type Verbosity,
		const FName& Category) override
	{
		if (Message == nullptr || !Category.ToString().StartsWith(TEXT("LogSGS")))
		{
			return;
		}

		FSGSEditorLogEntry Entry;
		Entry.Category = Category;
		Entry.Verbosity = static_cast<ELogVerbosity::Type>(
			Verbosity & ELogVerbosity::VerbosityMask);
		Entry.Message = Message;
		PendingEntries.Enqueue(MoveTemp(Entry));
	}

	bool Dequeue(FSGSEditorLogEntry& OutEntry)
	{
		return PendingEntries.Dequeue(OutEntry);
	}

private:
	TQueue<FSGSEditorLogEntry, EQueueMode::Mpsc> PendingEntries;
};
}

class FSGSEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FMessageLogModule& MessageLogModule =
			FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
		FMessageLogInitializationOptions Options;
		Options.bShowFilters = true;
		Options.bShowPages = false;
		Options.bAllowClear = true;
		Options.bDiscardDuplicates = false;
		Options.bScrollToBottom = true;
		MessageLogModule.RegisterLogListing(
			SGSMessageLogName,
			LOCTEXT("SGSMessageLogLabel", "SGS"),
			Options);

		OutputDevice = MakeUnique<FSGSEditorLogOutputDevice>();
		if (GLog != nullptr)
		{
			GLog->AddOutputDevice(OutputDevice.Get());
		}

		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FSGSEditorModule::FlushPendingLogs),
			0.1f);
	}

	virtual void ShutdownModule() override
	{
		if (TickerHandle.IsValid())
		{
			FTSTicker::RemoveTicker(TickerHandle);
			TickerHandle.Reset();
		}

		if (GLog != nullptr && OutputDevice.IsValid())
		{
			GLog->RemoveOutputDevice(OutputDevice.Get());
		}
		OutputDevice.Reset();

		if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
		{
			FMessageLogModule& MessageLogModule =
				FModuleManager::GetModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
			MessageLogModule.UnregisterLogListing(SGSMessageLogName);
		}
	}

private:
	bool FlushPendingLogs(float DeltaTime)
	{
		if (!OutputDevice.IsValid())
		{
			return true;
		}

		FMessageLog MessageLog(SGSMessageLogName);
		MessageLog.SuppressLoggingToOutputLog();
		FSGSEditorLogEntry Entry;
		int32 FlushCount = 0;
		while (FlushCount < 512 && OutputDevice->Dequeue(Entry))
		{
			const FText Text = FText::FromString(FString::Printf(
				TEXT("[%s] %s"),
				*Entry.Category.ToString(),
				*Entry.Message));
			switch (Entry.Verbosity)
			{
			case ELogVerbosity::Fatal:
			case ELogVerbosity::Error:
				MessageLog.Error(Text);
				break;
			case ELogVerbosity::Warning:
				MessageLog.Warning(Text);
				break;
			default:
				MessageLog.Info(Text);
				break;
			}
			++FlushCount;
		}
		return true;
	}

	TUniquePtr<FSGSEditorLogOutputDevice> OutputDevice;
	FTSTicker::FDelegateHandle TickerHandle;
};

IMPLEMENT_MODULE(FSGSEditorModule, SGSEditor)

#undef LOCTEXT_NAMESPACE
