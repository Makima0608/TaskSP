// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableIgnoreInputTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"

#if (!UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleIgnoreInputTaskScratchPad::UAbleIgnoreInputTaskScratchPad()
{

}

UAbleIgnoreInputTaskScratchPad::~UAbleIgnoreInputTaskScratchPad()
{

}

UAbleIgnoreInputTask::UAbleIgnoreInputTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , m_MoveInput(true)
    , m_LookInput(true)
    , m_Input(false)
{
}

UAbleIgnoreInputTask::~UAbleIgnoreInputTask()
{
}

FString UAbleIgnoreInputTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleIgnoreInputTask");
}

void UAbleIgnoreInputTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleIgnoreInputTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

    if (!m_MoveInput && !m_LookInput && !m_Input)
	{
        return;
	}

    UAbleIgnoreInputTaskScratchPad* ScratchPad = Cast<UAbleIgnoreInputTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	ScratchPad->Pawns.Empty();

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

    for (TWeakObjectPtr<AActor>& Target : TargetArray)
    {
        if (Target.IsValid())
        {
            APawn* pawn = Cast<APawn>(Target);
            if (IsValid(pawn))
            {
                AController* controller = pawn->GetController();
                if (IsValid(controller))
                {
                    ScratchPad->Pawns.Add(pawn);

                    if (m_MoveInput)
                    {
                        controller->SetIgnoreMoveInput(true);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreMoveInput(+)"), { controller->GetName() }));
                        }
#endif
                    }

                    if(m_LookInput)
                    {
                        controller->SetIgnoreLookInput(true);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreLookInput(+)"), { controller->GetName() }));
                        }
#endif
                    }

                    if (m_Input)
                    {
                        pawn->DisableInput(nullptr);

#if !(UE_BUILD_SHIPPING)
                        if (IsVerbose())
                        {
                            PrintVerbose(Context, FString::Format(TEXT("Pawn {0} DisableInput(+)"), { pawn->GetName() }));
                        }
#endif
                    }
                }
            }
        }
    }
}

void UAbleIgnoreInputTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleIgnoreInputTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (!Context)
	{
		return;
	}

	UAbleIgnoreInputTaskScratchPad* ScratchPad = Cast<UAbleIgnoreInputTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

    for (TWeakObjectPtr<APawn>& Target : ScratchPad->Pawns)
    {
        if (Target.IsValid())
        {
            AController* controller = Target->GetController();
            if (IsValid(controller))
            {
                if (m_MoveInput)
                {
                    controller->SetIgnoreMoveInput(false);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreMoveInput(-)"), { Target->GetName() }));
                    }
#endif
                }

                if (m_LookInput)
                {
                    controller->SetIgnoreLookInput(false);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} IgnoreLookInput(-)"), { Target->GetName() }));
                    }
#endif
                }

                if (m_Input)
                {
                    Target->EnableInput(nullptr);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Pawn {0} DisableInput(-)"), { Target->GetName() }));
                    }
#endif
                }
            }
        }
    }
}

UAbleAbilityTaskScratchPad* UAbleIgnoreInputTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleIgnoreInputTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleIgnoreInputTaskScratchPad>(Context.Get());
}

TStatId UAbleIgnoreInputTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleIgnoreInputTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleIgnoreInputTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleTIgnoreInputTaskFormat", "{0}: {1}");
    FString TargetName = FString::Format(TEXT("Ignore({0} {1} {2})"), { m_MoveInput ? TEXT("Move") : TEXT(""), m_LookInput ? TEXT("Look") : TEXT(""), m_Input ? TEXT("Input") : TEXT("") });
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

void UAbleIgnoreInputTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	
}

#endif

bool UAbleIgnoreInputTask::IsSingleFrameBP_Implementation() const { return !m_MoveInput && !m_LookInput; }

EAbleAbilityTaskRealm UAbleIgnoreInputTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE