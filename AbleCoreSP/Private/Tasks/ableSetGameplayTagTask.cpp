// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSetGameplayTagTask.h"

#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleSetGameplayTagTaskScratchPad::UAbleSetGameplayTagTaskScratchPad()
{

}

UAbleSetGameplayTagTaskScratchPad::~UAbleSetGameplayTagTaskScratchPad()
{

}

UAbleSetGameplayTagTask::UAbleSetGameplayTagTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TagList(),
	m_RemoveOnEnd(false),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{

}

UAbleSetGameplayTagTask::~UAbleSetGameplayTagTask()
{

}

FString UAbleSetGameplayTagTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleSetGameplayTagTask");
}

void UAbleSetGameplayTagTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleSetGameplayTagTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	UAbleSetGameplayTagTaskScratchPad* Scratchpad = nullptr;
	if (m_RemoveOnEnd)
	{
		Scratchpad = Cast<UAbleSetGameplayTagTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(Scratchpad);
		Scratchpad->TaggedActors.Empty();
	}

	for (const TWeakObjectPtr<AActor> & Actor : TaskTargets)
	{
		if (Actor.IsValid())
		{
			if (UAbleAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAbleAbilityComponent>())
			{
				for (const FGameplayTag& Tag : m_TagList)
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Adding Tag %s to Actor %s."), *Actor->GetName(), *Tag.ToString()));
					}
#endif
					AbilityComponent->AddTag(Tag);
				}

				if (Scratchpad)
				{
					Scratchpad->TaggedActors.Add(Actor);
				}
			}
		}
	}
}

void UAbleSetGameplayTagTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleSetGameplayTagTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (m_RemoveOnEnd && Context)
	{
		UAbleSetGameplayTagTaskScratchPad* Scratchpad = Cast<UAbleSetGameplayTagTaskScratchPad>(Context->GetScratchPadForTask(this));
		check(Scratchpad);

		for (const TWeakObjectPtr<AActor> & Actor : Scratchpad->TaggedActors)
		{
			if (Actor.IsValid())
			{
				if (UAbleAbilityComponent* AbilityComponent = Actor->FindComponentByClass<UAbleAbilityComponent>())
				{
					for (const FGameplayTag& Tag : m_TagList)
					{
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Removing Tag %s from Actor %s."), *Actor->GetName(), *Tag.ToString()));
						}
#endif
						AbilityComponent->RemoveTag(Tag);
					}
				}
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleSetGameplayTagTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_RemoveOnEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleSetGameplayTagTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAbleSetGameplayTagTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleSetGameplayTagTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleSetGameplayTagTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleSetGameplayTagTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlaySetGameplayTagTaskFormat", "{0}: {1}");
	FString GameplayTagNames = TEXT("<none>");
	if (m_TagList.Num())
	{
		if (m_TagList.Num() == 1)
		{
			GameplayTagNames = m_TagList[0].ToString();
		}
		else
		{
			GameplayTagNames = FString::Printf(TEXT("%s, ... (%d Total Tags)"), *m_TagList[0].ToString(), m_TagList.Num());
		}

	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(GameplayTagNames));
}

#endif

bool UAbleSetGameplayTagTask::IsSingleFrameBP_Implementation() const { return !m_RemoveOnEnd; }

EAbleAbilityTaskRealm UAbleSetGameplayTagTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE
