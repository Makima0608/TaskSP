// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCustomEventTask.h"

#include "ableAbility.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCustomEventTask::UAbleCustomEventTask(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_EventName(NAME_None)
{

}

UAbleCustomEventTask::~UAbleCustomEventTask()
{

}

FString UAbleCustomEventTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleCustomEventTask");
}

void UAbleCustomEventTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleCustomEventTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Firing Custom Event %s."), *m_EventName.ToString()));
	}
#endif

	// Call our parent.
	Context->GetAbility()->OnCustomEventBP(Context, m_EventName);
}

TStatId UAbleCustomEventTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCustomEventTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleCustomEventTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleCustomEventTaskFormat", "{0}: {1}");
	FString EventName = m_EventName.IsNone() ? ("<none>") : m_EventName.ToString();
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(EventName));
}

EDataValidationResult UAbleCustomEventTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCustomEventBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("OnCustomEventBP_NotFound", "Function 'OnCustomEventBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

#endif

bool UAbleCustomEventTask::IsSingleFrameBP_Implementation() const { return true; }

EAbleAbilityTaskRealm UAbleCustomEventTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE
