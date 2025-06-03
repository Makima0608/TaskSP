// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableBranchTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Tasks/ableBranchCondition.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleBranchTaskScratchPad::UAbleBranchTaskScratchPad()
    : BranchAbility(nullptr),
    BranchConditionsMet(false)
{
}

UAbleBranchTaskScratchPad::~UAbleBranchTaskScratchPad()
{

}

UAbleBranchTask::UAbleBranchTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_BranchAbility(nullptr),
	m_MustPassAllBranchConditions(false),
	m_CopyTargetsOnBranch(false),
    m_BranchOnTaskEnd(false)
{

}

UAbleBranchTask::~UAbleBranchTask()
{

}

FString UAbleBranchTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleBranchTask");
}

void UAbleBranchTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleBranchTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleBranchTaskScratchPad* ScratchPad = Cast<UAbleBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

    TSubclassOf<UAbleAbility> Ability = m_BranchAbility;
    ScratchPad->BranchAbility = Ability.GetDefaultObject();
	ScratchPad->BranchConditionsMet = false;
	ScratchPad->CachedKeys.Empty();

	if (!ScratchPad->BranchAbility)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
            PrintVerbose(Context, FString::Printf(TEXT("Branch Ability is null.")));
		}
#endif
		return;
	}

	bool BranchOnTaskEnd = m_BranchOnTaskEnd;
	if (CheckBranchCondition(Context))
	{
        ScratchPad->BranchConditionsMet = true;

        // deferred until task end
        if (BranchOnTaskEnd)
		{ 
            return;
		}

		InternalDoBranch(Context);
	}
}

void UAbleBranchTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleBranchTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

    UAbleBranchTaskScratchPad* ScratchPad = Cast<UAbleBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return;

    if (ScratchPad->BranchConditionsMet)
	{ 
        return;
	}

    TSubclassOf<UAbleAbility> Ability = m_BranchAbility;
    ScratchPad->BranchAbility = Ability.GetDefaultObject();
    if (!ScratchPad->BranchAbility)
    {
#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Branch Ability is null.")));
        }
#endif
        return;
    }

	bool BranchOnTaskEnd = m_BranchOnTaskEnd;
	if (CheckBranchCondition(Context))
	{
        ScratchPad->BranchConditionsMet = true;

        // deferred until task end
        if (BranchOnTaskEnd)
		{ 
            return;
		}

		InternalDoBranch(Context);
	}
}

void UAbleBranchTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
    Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleBranchTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	bool BranchOnTaskEnd = m_BranchOnTaskEnd;
    if (BranchOnTaskEnd)
    {
        UAbleBranchTaskScratchPad* ScratchPad = Cast<UAbleBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
        if (!ScratchPad) return;

        if (ScratchPad->BranchConditionsMet)
        {
			InternalDoBranch(Context);
        }
    }
}

UAbleAbilityTaskScratchPad* UAbleBranchTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleBranchTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleBranchTaskScratchPad>(Context.Get());
}

TStatId UAbleBranchTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleBranchTask, STATGROUP_Able);
}

void UAbleBranchTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BranchAbility, TEXT("Ability"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_MustPassAllBranchConditions, TEXT("Must Pass All Conditions"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_CopyTargetsOnBranch, TEXT("Copy Targets on Branch"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BranchOnTaskEnd, TEXT("Branch on Task End"));
}

void UAbleBranchTask::InternalDoBranch(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	UAbleBranchTaskScratchPad* ScratchPad = Cast<UAbleBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return;
#if !(UE_BUILD_SHIPPING)
    if (IsVerbose())
    {
		PrintVerbose(Context, FString::Printf(TEXT("Conditions passed. Branching to Ability %s"), *(ScratchPad->BranchAbility->GetName())));
    }
#endif
	if (ScratchPad->BranchAbility)
    {
		UAbleAbilityContext* NewContext = UAbleAbilityContext::MakeContext(ScratchPad->BranchAbility, Context->GetSelfAbilityComponent(), Context->GetOwner(), Context->GetInstigator());
		bool CopyTargetsOnBranch = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_CopyTargetsOnBranch);
        if (CopyTargetsOnBranch && NewContext)
        {
            NewContext->GetMutableTargetActors().Append(Context->GetTargetActors());
        }

        Context->GetSelfAbilityComponent()->BranchAbility(NewContext);
    }
#if !(UE_BUILD_SHIPPING)
    else if (IsVerbose())
    {
        PrintVerbose(Context, FString::Printf(TEXT("Branching Failed. Branch Ability was null.")));
    }
#endif
}

bool UAbleBranchTask::CheckBranchCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	UAbleBranchTaskScratchPad* ScratchPad = Cast<UAbleBranchTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return false;

	bool MustPassAllConditions = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_MustPassAllBranchConditions);
	EAbleConditionResults Result = EAbleConditionResults::ACR_Failed;
	for (UAbleBranchCondition* Condition : m_BranchConditions)
	{
		if (!Condition)
		{
			continue;
		}

		Result = Condition->CheckCondition(Context, *ScratchPad);
		if (Condition->IsNegated())
		{
			if (Result == EAbleConditionResults::ACR_Passed)
			{
				Result = EAbleConditionResults::ACR_Failed;
			}
			else if (Result == EAbleConditionResults::ACR_Failed)
			{
				Result = EAbleConditionResults::ACR_Passed;
			}
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			FString ConditionResult = TEXT("Passed");
			if (Result == EAbleConditionResults::ACR_Ignored)
			{
				ConditionResult = TEXT("Ignored");
			}
			else if (Result == EAbleConditionResults::ACR_Failed)
			{
				ConditionResult = TEXT("Failed");
			}
			PrintVerbose(Context, FString::Printf(TEXT("Condition %s returned %s."), *Condition->GetName(), *ConditionResult));
		}
#endif

		// Check our early out cases.
		if (MustPassAllConditions && Result == EAbleConditionResults::ACR_Failed)
		{
			// Failed
			break;
		}
		else if (!MustPassAllConditions && Result == EAbleConditionResults::ACR_Passed)
		{
			// Success
			break;
		}
	}

	return Result == EAbleConditionResults::ACR_Passed;
}

#if WITH_EDITOR

FText UAbleBranchTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleBranchTaskFormat", "{0}({1}): {2}");
	FString AbilityName = ("<null>");
	if (m_BranchAbilityDelegate.IsBound())
	{
		AbilityName = ("Dynamic");
	}
	else if (*m_BranchAbility)
	{
		if (UAbleAbility* Ability = Cast<UAbleAbility>(m_BranchAbility->GetDefaultObject()))
		{
			AbilityName = Ability->GetAbilityName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AbilityName));
}

FText UAbleBranchTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AbilityName = TEXT("NULL");
	if (m_BranchAbilityDelegate.IsBound())
	{
		AbilityName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_BranchAbilityDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AbilityName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_BranchAbility\" Filter=\"/Script/AbleCore.AbleAbility\">{0}</>"), { m_BranchAbility ? m_BranchAbility->GetDefaultObjectName().ToString() : AbilityName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AbleBranchAbilityTaskRichFmt", "\t- Branch Ability: {0}"), FText::FromString(AbilityName));

	return StringBuilder.ToText();
}

EDataValidationResult UAbleBranchTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    for (UAbleBranchCondition* Condition : m_BranchConditions)
    {
		if (!Condition)
		{
			continue;
		}

        if (Condition->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
		{
            result = EDataValidationResult::Invalid;
		}
    }
    return result;
}

bool UAbleBranchTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	for (UAbleBranchCondition* Condition : m_BranchConditions)
	{
		if (!Condition)
		{
			continue;
		}

		modified |= Condition->FixUpObjectFlags();
	}

	return modified;
}

#endif

// Client for Auth client, Server for AIs/Proxies.
EAbleAbilityTaskRealm UAbleBranchTask::GetTaskRealmBP_Implementation() const
{
	return EAbleAbilityTaskRealm::ATR_ClientAndServer;
}

#undef LOCTEXT_NAMESPACE
