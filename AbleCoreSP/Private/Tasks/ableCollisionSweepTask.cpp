// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCollisionSweepTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "Tasks/ableValidation.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCollisionSweepTaskScratchPad::UAbleCollisionSweepTaskScratchPad()
	: SourceTransform(FTransform::Identity),
	AsyncHandle(),
	AsyncProcessed(false)
{

}

UAbleCollisionSweepTaskScratchPad::~UAbleCollisionSweepTaskScratchPad()
{

}

UAbleCollisionSweepTask::UAbleCollisionSweepTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_ClientAndServer)
{

}

UAbleCollisionSweepTask::~UAbleCollisionSweepTask()
{

}

FString UAbleCollisionSweepTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleCollisionSweepTask");
}

void UAbleCollisionSweepTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleCollisionSweepTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_SweepShape)
	{
		UAbleCollisionSweepTaskScratchPad* ScratchPad = Cast<UAbleCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		m_SweepShape->GetQueryTransform(Context, ScratchPad->SourceTransform);
		ScratchPad->AsyncProcessed = false;
		ScratchPad->AsyncHandle._Handle = 0;
	}
}

void UAbleCollisionSweepTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleCollisionSweepTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	UAbleCollisionSweepTaskScratchPad* ScratchPad = Cast<UAbleCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	if (m_SweepShape && m_SweepShape->IsAsync() && 
		USPAbleSettings::IsAsyncEnabled() && 
		UAbleAbilityTask::IsDone(Context))
	{
		if (ScratchPad->AsyncHandle._Handle == 0)
		{
			// We're at the end of the our task, so we need to perform our sweep. If We're Async, we need to queue
			// the query up and then process it next frame.
			ScratchPad->AsyncHandle = m_SweepShape->DoAsyncSweep(Context, ScratchPad->SourceTransform);
		}
	}
}

void UAbleCollisionSweepTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleCollisionSweepTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{
	TArray<FAbleQueryResult> OutResults;
	if (m_SweepShape && Context)
	{
		UAbleCollisionSweepTaskScratchPad* ScratchPad = Cast<UAbleCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		if (m_SweepShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
		{
			m_SweepShape->GetAsyncResults(Context, ScratchPad->AsyncHandle, OutResults);
			ScratchPad->AsyncProcessed = true;
		}
		else
		{
			m_SweepShape->DoSweep(Context, ScratchPad->SourceTransform, OutResults);
		}
	}

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Sweep found %d results."), OutResults.Num()));
	}
#endif

	if (OutResults.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
	{
		for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
		{
			CollisionFilter->Filter(Context, OutResults);

#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), OutResults.Num()));
			}
#endif
		}

		if (OutResults.Num() || (m_CopyResultsToContext && m_ClearExistingTargets)) // Early out if we filtered everything out.
		{
			if (m_CopyResultsToContext)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), OutResults.Num()));
				}
#endif
				CopyResultsToContext(OutResults, Context);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), OutResults.Num()));
				}
#endif
				if (Context && Context->GetAbility())
				{
					EAbleCallbackResult Ret = Context->GetAbility()->OnCollisionEventBP(Context, m_Name, OutResults);
				}
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleCollisionSweepTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleCollisionSweepTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleCollisionSweepTaskScratchPad>(Context.Get());
}

TStatId UAbleCollisionSweepTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCollisionSweepTask, STATGROUP_Able);
}

void UAbleCollisionSweepTask::CopyResultsToContext(const TArray<FAbleQueryResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		TArray<TWeakObjectPtr<AActor>> AdditionTargets;
		AdditionTargets.Reserve(InResults.Num());
		for (const FAbleQueryResult& Result : InResults)
		{
			AdditionTargets.Add(Result.Actor);
		}
		AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, m_ClearExistingTargets);
	}
}

void UAbleCollisionSweepTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_SweepShape)
	{
		m_SweepShape->BindDynamicDelegates(Ability);
	}

	for (UAbleCollisionFilter* Filter : m_Filters)
	{
		if (Filter)
		{
			Filter->BindDynamicDelegates(Ability);
		}
	}
}

bool UAbleCollisionSweepTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleCollisionSweepTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_SweepShape && m_SweepShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
	{
		UAbleCollisionSweepTaskScratchPad* ScratchPad = Cast<UAbleCollisionSweepTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return true;
		return ScratchPad->AsyncHandle._Handle != 0;
	}
	else
	{
		return UAbleAbilityTask::IsDone(Context);
	}
}

#if WITH_EDITOR

FText UAbleCollisionSweepTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleCollisionSweepFormat", "{0}: {1}");
	FString ShapeDescription = TEXT("<none>");
	if (m_SweepShape)
	{
		ShapeDescription = m_SweepShape->DescribeShape();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAbleCollisionSweepTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
    {
        if (CollisionFilter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), FText::FromString(AbilityContext->GetAbilityName())));
            result = EDataValidationResult::Invalid;
        }
    }

    if (m_SweepShape != nullptr && m_SweepShape->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
        result = EDataValidationResult::Invalid;

    if (m_SweepShape == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoSweepShape", "No Sweep Shape Defined: {0}"), FText::FromString(AbilityContext->GetAbilityName())));
        result = EDataValidationResult::Invalid;
    }
    else if (m_SweepShape->GetSweepLocation().GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
    {
        if (AbilityContext->RequiresTarget())
        {
            // trust that the ability cannot run unless it has a target, so don't do any dependency validation
            if (AbilityContext->GetTargeting() == nullptr)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoTargeting", "No Targeting method Defined: {0} with RequiresTarget"), FText::FromString(AbilityContext->GetAbilityName())));
                result = EDataValidationResult::Invalid;
            }
        }
        else if (AbilityContext->GetTargeting() == nullptr)
        {
            // if we have a target actor, we should have a dependency task that copies results
            bool hasValidDependency = UAbleValidation::CheckDependencies(GetTaskDependencies());
            if (!hasValidDependency)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), FText::FromString(AbilityContext->GetAbilityName())));
                result = EDataValidationResult::Invalid;
            }
        }
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCollisionEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnCollisionEventBP_NotFound", "Function 'OnCollisionEventBP' not found: {0}"), FText::FromString(AbilityContext->GetAbilityName())));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

void UAbleCollisionSweepTask::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
    if (m_SweepShape != nullptr)
    {
        m_SweepShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

bool UAbleCollisionSweepTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_SweepShape)
	{
		modified |= m_SweepShape->FixUpObjectFlags();
	}

	for (UAbleCollisionFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

bool UAbleCollisionSweepTask::IsSingleFrameBP_Implementation() const { return false; }

EAbleAbilityTaskRealm UAbleCollisionSweepTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE