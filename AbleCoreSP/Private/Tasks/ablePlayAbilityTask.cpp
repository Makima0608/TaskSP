// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlayAbilityTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableAbilityContext.h"
#include "ableAbilityTypes.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePlayAbilityTask::UAblePlayAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Ability(),
	m_Owner(EAbleAbilityTargetType::ATT_Owner),
	m_Instigator(EAbleAbilityTargetType::ATT_Self),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server),
	m_CopyTargets(false)
{

}

UAblePlayAbilityTask::~UAblePlayAbilityTask()
{

}

FString UAblePlayAbilityTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePlayAbilityTask");
}

void UAblePlayAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePlayAbilityTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	TSubclassOf<UAbleAbility> AbilityClass = m_Ability;

	if (const UAbleAbility* Ability = Cast<const UAbleAbility>(AbilityClass->GetDefaultObject()))
	{
		EAbleAbilityTargetType Instigator = m_Instigator;
		EAbleAbilityTargetType Owner = m_Owner;

		TArray<TWeakObjectPtr<AActor>> TaskTargets;
		GetActorsForTask(Context, TaskTargets);

		AActor* InstigatorActor = GetSingleActorFromTargetType(Context, Instigator);
		AActor* OwnerActor = GetSingleActorFromTargetType(Context, Owner);
		UAbleAbilityComponent* AbilityComponent = nullptr;
		
		for (const TWeakObjectPtr<AActor>& TaskTarget : TaskTargets)
		{
			if (TaskTarget.IsValid())
			{
				// Target Actor with GetSingleActorFromTargetType always returns the first entry, so we need to
				// update the value as we work if we're set to use that Target Type.
				if (Instigator == EAbleAbilityTargetType::ATT_TargetActor)
				{
					InstigatorActor = TaskTarget.Get();
				}

				if (Owner == EAbleAbilityTargetType::ATT_TargetActor)
				{
					OwnerActor = TaskTarget.Get();
				}

				AbilityComponent = TaskTarget->FindComponentByClass<UAbleAbilityComponent>();
				if (AbilityComponent)
				{
					UAbleAbilityContext* NewContext = UAbleAbilityContext::MakeContext(Ability, AbilityComponent, OwnerActor, InstigatorActor);
					bool CopyTargets = m_CopyTargets;
					if (CopyTargets && NewContext)
					{
						NewContext->GetMutableTargetActors().Append(Context->GetTargetActors());
					}

#if !(UE_BUILD_SHIPPING)
					if (IsVerbose() && NewContext)
					{
						TStringBuilder<2048> TargetStringBuilder;
						if (NewContext->GetTargetActorsWeakPtr().Num() == 0)
						{
							TargetStringBuilder.Append(TEXT("None"));
						}
						else
						{
							for (const AActor* CurrentTarget : NewContext->GetTargetActors())
							{
								TargetStringBuilder.Appendf(TEXT(", %s"), *CurrentTarget->GetName());
							}
						}

						PrintVerbose(Context, FString::Printf(TEXT("Calling ActivateAbility with Actor %s with Ability %s, Owner %s, Instigator %s, Targets ( %s )."), *TaskTarget->GetName(),
							*Ability->GetAbilityName(), InstigatorActor ? *InstigatorActor->GetName() : *FString("None"), OwnerActor ? *OwnerActor->GetName() : *FString("None"), TargetStringBuilder.ToString()));
					}
#endif
					// todo...
					// AbilityComponent->ActivateAbility(NewContext);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("No Ability set for PlayAbilityTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
		return;
	}
}

TStatId UAblePlayAbilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePlayAbilityTask, STATGROUP_Able);
}

void UAblePlayAbilityTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Ability, TEXT("Ability"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_CopyTargets, TEXT("Copy Targets"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Owner, TEXT("Owner"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Instigator, TEXT("Instigator"));
}

#if WITH_EDITOR

FText UAblePlayAbilityTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlayAbilityTaskFormat", "{0}: {1}");
	FString AbilityName = TEXT("<null>");
	if (*m_Ability)
	{
		if (UAbleAbility* Ability = Cast<UAbleAbility>(m_Ability->GetDefaultObject()))
		{
			AbilityName = Ability->GetAbilityName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAblePlayAbilityTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AbilityName = TEXT("NULL");
	if (m_AbilityDelegate.IsBound())
	{
		AbilityName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AbilityDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AbilityName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Ability\" Filter=\"/Script/AbleCore.AbleAbility\">{0}</>"), { m_Ability ? m_Ability->GetDefaultObjectName().ToString() : AbilityName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AblePlayAbilityTaskRichFmt", "\t- Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

#endif

bool UAblePlayAbilityTask::IsSingleFrameBP_Implementation() const { return true; }

EAbleAbilityTaskRealm UAblePlayAbilityTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE