// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableRemoveGameplayTagTask.h"

#include "ableAbilityComponent.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleRemoveGameplayTagTask::UAbleRemoveGameplayTagTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{

}

UAbleRemoveGameplayTagTask::~UAbleRemoveGameplayTagTask()
{

}

FString UAbleRemoveGameplayTagTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleRemoveGameplayTagTask");
}

void UAbleRemoveGameplayTagTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleRemoveGameplayTagTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

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
						PrintVerbose(Context, FString::Printf(TEXT("Removing Tag %s from Actor %s."), *Tag.ToString(), *Actor->GetName()));
					}
#endif
					AbilityComponent->RemoveTag(Tag);
				}
			}
		}
	}
}

TStatId UAbleRemoveGameplayTagTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleRemoveGameplayTagTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleRemoveGameplayTagTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlayRemoveGameplayTagTaskFormat", "{0}: {1}");
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

bool UAbleRemoveGameplayTagTask::IsSingleFrameBP_Implementation() const { return true; }

EAbleAbilityTaskRealm UAbleRemoveGameplayTagTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE
