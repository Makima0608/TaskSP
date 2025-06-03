// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCancelAbilityTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableAbilityContext.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCancelAbilityTask::UAbleCancelAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Ability(),
	m_TagQuery(),
	m_PassiveBehavior(RemoveOneStack),
	m_CancelResult(Interrupted),
	m_EventName(NAME_None),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{

}

UAbleCancelAbilityTask::~UAbleCancelAbilityTask()
{

}

FString UAbleCancelAbilityTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleCancelAbilityTask");
}

void UAbleCancelAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleCancelAbilityTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor> TargetActor : TaskTargets)
	{
		if (!TargetActor.IsValid())
		{
			continue;
		}

		if (UAbleAbilityComponent* AbilityComponent = TargetActor->FindComponentByClass<UAbleAbilityComponent>())
		{
			if (const UAbleAbility* ActiveAbility = AbilityComponent->GetActiveAbility())
			{
				if (ShouldCancelAbility(*ActiveAbility, *Context))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Cancelling Ability %s on Actor %s."), *ActiveAbility->GetAbilityName(),
							*TargetActor->GetName()));
					}
#endif
					AbilityComponent->CancelAbility(ActiveAbility, m_CancelResult.GetValue());
				}
			}

			TArray<UAbleAbility*> CurrentPassives;
			AbilityComponent->GetCurrentPassiveAbilities(CurrentPassives);

			for (UAbleAbility* Passive : CurrentPassives)
			{
				if (ShouldCancelAbility(*Passive, *Context))
				{
					switch (m_PassiveBehavior.GetValue())
					{
					case RemoveOneStack:
					case RemoveOneStackWithRefresh:
					{
						int32 StackCount = AbilityComponent->GetCurrentStackCountForPassiveAbility(Passive);
						int32 NewStackCount = FMath::Max(StackCount - 1, 0);

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Setting Passive Ability %s Stack on Actor %s from %d to %d."), *Passive->GetAbilityName(),
								*TargetActor->GetName(), StackCount, NewStackCount));
						}
#endif

						AbilityComponent->SetPassiveStackCount(Passive, NewStackCount, m_PassiveBehavior.GetValue() == RemoveOneStackWithRefresh, m_CancelResult.GetValue());
					}
					break;
					case RemoveEntireStack:
					default:
					{
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Cancelling Passive Ability %s on Actor %s."), *Passive->GetAbilityName(),
								*TargetActor->GetName()));
						}
#endif
						AbilityComponent->CancelAbility(Passive, m_CancelResult.GetValue());
					}
					break;
					}
				}
			}
		}
	}

}

TStatId UAbleCancelAbilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCancelAbilityTask, STATGROUP_Able);
}

void UAbleCancelAbilityTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Ability, TEXT("Ability"));
}

bool UAbleCancelAbilityTask::ShouldCancelAbility(const UAbleAbility& Ability, const UAbleAbilityContext& Context) const
{
	// Give BP the first shot.
	if (Ability.ShouldCancelAbilityBP(&Context, &Ability, m_EventName))
	{
		return true;
	}

	if (*m_Ability && m_Ability->GetDefaultObject<UAbleAbility>()->GetAbilityNameHash() == Ability.GetAbilityNameHash())
	{
		return true;
	}

	if (!m_TagQuery.IsEmpty())
	{
		return Ability.GetAbilityTagContainer().MatchesQuery(m_TagQuery);
	}

	return false;
}

#if WITH_EDITOR

FText UAbleCancelAbilityTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleCancelAbilityTaskFormat", "{0}: {1}");
	FString AbilityName = TEXT("<null>");
	if (*m_Ability)
	{
		if (UAbleAbility* Ability = Cast<UAbleAbility>(m_Ability->GetDefaultObject()))
		{
			AbilityName = Ability->GetAbilityName();
		}
	}
	else if (!m_TagQuery.IsEmpty())
	{
		AbilityName = m_TagQuery.GetDescription();
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAbleCancelAbilityTask::GetRichTextTaskSummary() const
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
	StringBuilder.AppendLineFormat(LOCTEXT("AbleCancelAbilityTaskRichFmt", "\t- Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

#endif

bool UAbleCancelAbilityTask::IsSingleFrameBP_Implementation() const
{
	return true;
}

EAbleAbilityTaskRealm UAbleCancelAbilityTask::GetTaskRealmBP_Implementation() const
{
	return m_TaskRealm;
}

#undef LOCTEXT_NAMESPACE