#pragma once

// SGS 工具层的统一错误词汇，供 TValueOrError 返回可恢复失败。
// 业务校验失败用 FSGSError；不变量诊断仍交给 check / ensure。
// 错误码必须遵循 SGS.<Domain>.<Reason>，方便日志与回放校验归类。

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

struct SGS_API FSGSError
{
	FName Code;
	FString Context;
	FText Message;

	FSGSError() = default;

	FSGSError(FName InCode, FString InContext = FString(), FText InMessage = FText::GetEmpty())
		: Code(InCode)
		, Context(MoveTemp(InContext))
		, Message(MoveTemp(InMessage))
	{
	}

	bool IsValid() const { return !Code.IsNone(); }

	bool CheckInvariants() const
	{
		return !IsValid() || ensureMsgf(IsValidCode(Code), TEXT("SGS error code must follow SGS.<Domain>.<Reason>: %s"), *Code.ToString());
	}

	FString ToLogString() const
	{
		if (Message.IsEmpty())
		{
			return FString::Printf(TEXT("%s | %s"), *Code.ToString(), *Context);
		}

		return FString::Printf(TEXT("%s | %s | %s"), *Code.ToString(), *Context, *Message.ToString());
	}

	static FSGSError Make(FName InCode, FString InContext = FString(), FText InMessage = FText::GetEmpty())
	{
		ensureMsgf(IsValidCode(InCode), TEXT("SGS error code must follow SGS.<Domain>.<Reason>: %s"), *InCode.ToString());
		return FSGSError(InCode, MoveTemp(InContext), MoveTemp(InMessage));
	}

	static FSGSError MakeCode(FName InDomain, FName InReason, FString InContext = FString(), FText InMessage = FText::GetEmpty())
	{
		return Make(FName(*FString::Printf(TEXT("SGS.%s.%s"), *InDomain.ToString(), *InReason.ToString())),
			MoveTemp(InContext),
			MoveTemp(InMessage));
	}

	static FSGSError InvalidArgument(FName InDomain, FString InContext, FText InMessage = FText::GetEmpty())
	{
		return MakeCode(InDomain, FName(TEXT("InvalidArgument")), MoveTemp(InContext), MoveTemp(InMessage));
	}

	static FSGSError InvalidState(FName InDomain, FString InContext, FText InMessage = FText::GetEmpty())
	{
		return MakeCode(InDomain, FName(TEXT("InvalidState")), MoveTemp(InContext), MoveTemp(InMessage));
	}

	static FSGSError NotFound(FName InDomain, FString InContext, FText InMessage = FText::GetEmpty())
	{
		return MakeCode(InDomain, FName(TEXT("NotFound")), MoveTemp(InContext), MoveTemp(InMessage));
	}

	static FSGSError InvariantViolation(FName InDomain, FString InContext, FText InMessage = FText::GetEmpty())
	{
		return MakeCode(InDomain, FName(TEXT("InvariantViolation")), MoveTemp(InContext), MoveTemp(InMessage));
	}

	static bool IsValidCode(FName InCode)
	{
		if (InCode.IsNone())
		{
			return false;
		}

		TArray<FString> Parts;
		InCode.ToString().ParseIntoArray(Parts, TEXT("."), false);
		return Parts.Num() >= 3
			&& Parts[0] == TEXT("SGS")
			&& !Parts[1].IsEmpty()
			&& !Parts.Last().IsEmpty();
	}
};

template <typename ValueType>
using TSGSResult = TValueOrError<ValueType, FSGSError>;

using FSGSStatus = TValueOrError<void, FSGSError>;
