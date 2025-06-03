// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCheckConditionTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Tasks/ableBranchCondition.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCheckConditionTaskScratchPad::UAbleCheckConditionTaskScratchPad()
    : ConditionMet(false)
{

}

UAbleCheckConditionTaskScratchPad::~UAbleCheckConditionTaskScratchPad()
{

}

UAbleCheckConditionTask::UAbleCheckConditionTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , m_ConditionEventName()
{

}

UAbleCheckConditionTask::~UAbleCheckConditionTask()
{

}

FString UAbleCheckConditionTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleCheckConditionTask");
}

bool UAbleCheckConditionTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleCheckConditionTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
    if (!Super::IsDone(Context))
	{
        return false;
	}

    UAbleCheckConditionTaskScratchPad* ScratchPad = Cast<UAbleCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return true;

    return ScratchPad->ConditionMet;
}

void UAbleCheckConditionTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleCheckConditionTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleCheckConditionTaskScratchPad* ScratchPad = Cast<UAbleCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

    ScratchPad->ConditionMet = CheckCondition(Context, *ScratchPad);
	if (ScratchPad->ConditionMet)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Condition met %s"), *(GetName())));
		}
#endif		
	}
}

void UAbleCheckConditionTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleCheckConditionTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

    UAbleCheckConditionTaskScratchPad* ScratchPad = CastChecked<UAbleCheckConditionTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return;

    if (ScratchPad->ConditionMet)
	{
        return;
	}

	if (CheckCondition(Context, *ScratchPad))
	{
        ScratchPad->ConditionMet = true;

#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Condition met %s"), *(GetName())));
        }
#endif
	}
}

UAbleAbilityTaskScratchPad* UAbleCheckConditionTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleCheckConditionTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleCheckConditionTaskScratchPad>(Context.Get());
}

TStatId UAbleCheckConditionTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCheckConditionTask, STATGROUP_Able);
}

bool UAbleCheckConditionTask::CheckCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const UAbleCheckConditionTaskScratchPad& ScratchPad) const
{
    return Context.Get()->GetAbility()->CheckCustomConditionEventBP(Context.Get(), m_ConditionEventName);
}

#if WITH_EDITOR

FText UAbleCheckConditionTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleCheckConditionTask", "{0}: {1}");
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromName(m_ConditionEventName));
}

#endif

// Client for Auth client, Server for AIs/Proxies.
EAbleAbilityTaskRealm UAbleCheckConditionTask::GetTaskRealmBP_Implementation() const
{
	return EAbleAbilityTaskRealm::ATR_ClientAndServer;
}

#undef LOCTEXT_NAMESPACE
