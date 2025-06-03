// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlayForcedFeedbackTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ForceFeedbackEffect.h"

#if (!UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePlayForcedFeedbackTaskScratchPad::UAblePlayForcedFeedbackTaskScratchPad()
{

}

UAblePlayForcedFeedbackTaskScratchPad::~UAblePlayForcedFeedbackTaskScratchPad()
{

}

UAblePlayForcedFeedbackTask::UAblePlayForcedFeedbackTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    , ForceFeedbackEffect(nullptr)
    , StartTagName()
    , bLooping(false)
    , bIgnoreTimeDilation(true)
    , bPlayWhilePaused(false)
    , StopTagName()
    , StopOnTaskExit(true)
{
}

UAblePlayForcedFeedbackTask::~UAblePlayForcedFeedbackTask()
{
}

FString UAblePlayForcedFeedbackTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePlayForcedFeedbackTask");
}

void UAblePlayForcedFeedbackTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePlayForcedFeedbackTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

    if (ForceFeedbackEffect == nullptr)
	{
        return;
	}

    UAblePlayForcedFeedbackTaskScratchPad* ScratchPad = Cast<UAblePlayForcedFeedbackTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	ScratchPad->Controllers.Empty();

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

    for (TWeakObjectPtr<AActor>& Target : TargetArray)
    {
        if (Target.IsValid())
        {
            APawn* pawn = Cast<APawn>(Target);
            if (IsValid(pawn))
            {
                APlayerController* playerController = Cast<APlayerController>(pawn->GetController());
                if (IsValid(playerController))
                {
                    ScratchPad->Controllers.Add(playerController);
                    
                    FForceFeedbackParameters parameters;
                    parameters.Tag = StartTagName;
                    parameters.bLooping = bLooping;
                    parameters.bIgnoreTimeDilation = bIgnoreTimeDilation;
                    parameters.bPlayWhilePaused = bPlayWhilePaused;
                    playerController->ClientPlayForceFeedback(ForceFeedbackEffect, parameters);

#if !(UE_BUILD_SHIPPING)
                    if (IsVerbose())
                    {
                        PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Force Feedback {1}"), { playerController->GetName(), ForceFeedbackEffect->GetName() }));
                    }
#endif
                }
            }
        }
    }
}

void UAblePlayForcedFeedbackTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAblePlayForcedFeedbackTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (!Context || !StopOnTaskExit)
	{
		return;
	}

	UAblePlayForcedFeedbackTaskScratchPad* ScratchPad = Cast<UAblePlayForcedFeedbackTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

    for (TWeakObjectPtr<APlayerController>& PC : ScratchPad->Controllers)
    {
        if (PC.IsValid())
        {
            PC->ClientStopForceFeedback(ForceFeedbackEffect, StopTagName);

#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Format(TEXT("Controller {0} Played Force Feedback {1}"), { PC->GetName(), ForceFeedbackEffect->GetName() }));
            }
#endif
        }
    }
}

UAbleAbilityTaskScratchPad* UAblePlayForcedFeedbackTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAblePlayForcedFeedbackTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblePlayForcedFeedbackTaskScratchPad>(Context.Get());
}

TStatId UAblePlayForcedFeedbackTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePlayForcedFeedbackTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblePlayForcedFeedbackTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlayForcedFeedbackTaskFormat", "{0}: {1}");
    FString TargetName = FString::Format(TEXT("({0})"), { ForceFeedbackEffect ? ForceFeedbackEffect->GetName() : TEXT("<null>") });
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

bool UAblePlayForcedFeedbackTask::IsSingleFrameBP_Implementation() const { return false; }

EAbleAbilityTaskRealm UAblePlayForcedFeedbackTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_Client; }

#undef LOCTEXT_NAMESPACE