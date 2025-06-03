// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePossessionTask.h"

#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if (!UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePossessionTaskScratchPad::UAblePossessionTaskScratchPad()
{

}

UAblePossessionTaskScratchPad::~UAblePossessionTaskScratchPad()
{

}

UAblePossessionTask::UAblePossessionTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_Possessor(EAbleAbilityTargetType::ATT_Self),
	  m_PossessionTarget(EAbleAbilityTargetType::ATT_TargetActor), m_UnPossessOnEnd(false)
{
}

UAblePossessionTask::~UAblePossessionTask()
{

}

FString UAblePossessionTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePossessionTask");
}

void UAblePossessionTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePossessionTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	AActor* SelfActor = GetSingleActorFromTargetType(Context, m_Possessor.GetValue());
	AActor* PossessionTarget = GetSingleActorFromTargetType(Context, m_PossessionTarget.GetValue());

	APawn* SelfPawn = Cast<APawn>(SelfActor);
	APawn* TargetPawn = Cast<APawn>(PossessionTarget);

	if (SelfPawn && TargetPawn)
	{
		if (APlayerController* PC = Cast<APlayerController>(SelfPawn->Controller))
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Actor %s is now possessing %s"), *SelfPawn->GetName(), *TargetPawn->GetName()));
			}
#endif
			PC->Possess(TargetPawn);

			if (m_UnPossessOnEnd)
			{
				UAblePossessionTaskScratchPad* ScratchPad = Cast<UAblePossessionTaskScratchPad>(Context->GetScratchPadForTask(this));
				if (!ScratchPad) return;
				ScratchPad->PossessorController = PC;
			}
		}
	}

}

void UAblePossessionTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAblePossessionTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const
{

	if (m_UnPossessOnEnd && Context)
	{
		UAblePossessionTaskScratchPad* ScratchPad = Cast<UAblePossessionTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		if (ScratchPad->PossessorController.IsValid())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose() && ScratchPad->PossessorController->GetOwner())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Calling Unpossess on %s"), *ScratchPad->PossessorController->GetOwner()->GetName()));
			}
#endif
			ScratchPad->PossessorController->UnPossess();
		}
	}
}

UAbleAbilityTaskScratchPad* UAblePossessionTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_UnPossessOnEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAblePossessionTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblePossessionTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblePossessionTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePossessionTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAblePossessionTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePossessionTaskFormat", "{0}: {1}");
	FString TargetName = FAbleSPLogHelper::GetTargetTargetEnumAsString(m_Possessor);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

bool UAblePossessionTask::IsSingleFrameBP_Implementation() const { return !m_UnPossessOnEnd; }

EAbleAbilityTaskRealm UAblePossessionTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_Server; }

#undef LOCTEXT_NAMESPACE

