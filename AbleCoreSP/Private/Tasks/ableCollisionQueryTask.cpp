// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCollisionQueryTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "Tasks/ableRayCastQueryTask.h"
#include "Tasks/ableCollisionQueryTask.h"
#include "Engine/World.h"
#include "Tasks/ableValidation.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCollisionQueryTaskScratchPad::UAbleCollisionQueryTaskScratchPad()
	: AsyncHandle(),
	AsyncProcessed(false)
{

}

UAbleCollisionQueryTaskScratchPad::~UAbleCollisionQueryTaskScratchPad()
{

}

UAbleCollisionQueryTask::UAbleCollisionQueryTask(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{

}

UAbleCollisionQueryTask::~UAbleCollisionQueryTask()
{
	if (m_Filters.Num())
	{
		m_Filters.Empty();
	}
}

FString UAbleCollisionQueryTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleCollisionQueryTask");
}

void UAbleCollisionQueryTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleCollisionQueryTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	if (m_QueryShape)
	{
		if (m_QueryShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
		{
			UAbleCollisionQueryTaskScratchPad* ScratchPad = Cast<UAbleCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
			if (!ScratchPad) return;
			ScratchPad->AsyncHandle = m_QueryShape->DoAsyncQuery(Context, ScratchPad->QueryTransform);
			ScratchPad->AsyncProcessed = false;
		}
		else
		{
			TArray<FAbleQueryResult> Results;
			m_QueryShape->DoQuery(Context, Results);

#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Query found %d results."), Results.Num()));
			}
#endif

			if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
			{
				for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
				{
					CollisionFilter->Filter(Context, Results);

#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), Results.Num()));
					}
#endif
				}

				// We could have filtered out all our entries, so check again if the array is empty.
				if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
				{
					if (m_CopyResultsToContext)
					{

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), Results.Num()));
						}
#endif

						CopyResultsToContext(Results, Context);
					}

					if (m_FireEvent)
					{

#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), Results.Num()));
						}
#endif

						Context->GetAbility()->OnCollisionEventBP(Context, m_Name, Results);
					}
				}

			}

		}
	}
}

void UAbleCollisionQueryTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleCollisionQueryTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	if (m_QueryShape && m_QueryShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
	{
		UWorld* QueryWorld = m_QueryShape->GetQueryWorld(Context);
		check(QueryWorld);

		UAbleCollisionQueryTaskScratchPad* ScratchPad = Cast<UAbleCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		if (!ScratchPad->AsyncProcessed && QueryWorld->IsTraceHandleValid(ScratchPad->AsyncHandle, true))
		{
			FOverlapDatum Datum;
			if (QueryWorld->QueryOverlapData(ScratchPad->AsyncHandle, Datum))
			{
				TArray<FAbleQueryResult> Results;
				m_QueryShape->ProcessAsyncOverlaps(Context, ScratchPad->QueryTransform, Datum.OutOverlaps, Results);

#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Query found %d results."), Results.Num()));
				}
#endif

				if (Results.Num() || (m_CopyResultsToContext && m_ClearExistingTargets))
				{
					for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
					{
						CollisionFilter->Filter(Context, Results);
#if !(UE_BUILD_SHIPPING)
						if (IsVerbose())
						{
							PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), Results.Num()));
						}
#endif
					}

					if (Results.Num() || ( m_CopyResultsToContext && m_ClearExistingTargets ))
					{
						if (m_CopyResultsToContext)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), Results.Num()));
							}
#endif
							CopyResultsToContext(Results, Context);
						}

						if (m_FireEvent)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), Results.Num()));
							}
#endif
							Context->GetAbility()->OnCollisionEventBP(Context, m_Name, Results);
						}
					}
				}

				ScratchPad->AsyncProcessed = true;
			}
		}
	}
}

void UAbleCollisionQueryTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	const EAbleAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);
	OnTaskEndBP(Context.Get(), Result);
}

void UAbleCollisionQueryTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
	const EAbleAbilityTaskResult Result) const
{
}

bool UAbleCollisionQueryTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleCollisionQueryTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_QueryShape && m_QueryShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
	{
		UAbleCollisionQueryTaskScratchPad* ScratchPad = Cast<UAbleCollisionQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return true;
		return ScratchPad->AsyncProcessed;
	}
	else
	{
		return UAbleAbilityTask::IsDone(Context);
	}
}

UAbleAbilityTaskScratchPad* UAbleCollisionQueryTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if(m_QueryShape && m_QueryShape->IsAsync())
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleCollisionQueryTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAbleCollisionQueryTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleCollisionQueryTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCollisionQueryTask, STATGROUP_Able);
}

void UAbleCollisionQueryTask::CopyResultsToContext(const TArray<FAbleQueryResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
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

void UAbleCollisionQueryTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_QueryShape)
	{
		m_QueryShape->BindDynamicDelegates(Ability);
	}

	for (UAbleCollisionFilter* Filter : m_Filters)
	{
		if (Filter)
		{
			Filter->BindDynamicDelegates(Ability);
		}
	}
}

#if WITH_EDITOR

FText UAbleCollisionQueryTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleCollisionQueryFormat", "{0}: {1}");
	FString ShapeDescription = TEXT("<none>");
	if (m_QueryShape)
	{
		ShapeDescription = m_QueryShape->DescribeShape();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAbleCollisionQueryTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
    {
        if (CollisionFilter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    if (m_QueryShape != nullptr && m_QueryShape->IsTaskDataValid(AbilityContext, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
	{
        result = EDataValidationResult::Invalid;
	}

    if (m_QueryShape == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryShape", "No Query Shape Defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    else if (m_QueryShape->GetQueryLocation().GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
    {
        if (AbilityContext->RequiresTarget())
        {
            // trust that the ability cannot run unless it has a target, so don't do any dependency validation
            if (AbilityContext->GetTargeting() == nullptr)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoTargeting", "No Targeting method Defined: {0} with RequiresTarget"), AssetName));
                result = EDataValidationResult::Invalid;
            }
        }
        else if (AbilityContext->GetTargeting() == nullptr)
        {
            // if we have a target actor, we should have a dependency task that copies results
            bool hasValidDependency = UAbleValidation::CheckDependencies(GetTaskDependencies());
            if (!hasValidDependency)
            {
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), AssetName));
                result = EDataValidationResult::Invalid;
            }
        }
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnCollisionEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnCollisionEventBP_NotFound", "Function 'OnCollisionEventBP' not found: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }
    
    return result;
}

void UAbleCollisionQueryTask::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
    if (m_QueryShape != nullptr)
    {
        m_QueryShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

bool UAbleCollisionQueryTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_QueryShape)
	{
		modified |= m_QueryShape->FixUpObjectFlags();
	}

	for (UAbleCollisionFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

bool UAbleCollisionQueryTask::IsSingleFrameBP_Implementation() const
{
	return m_QueryShape ? !m_QueryShape->IsAsync() : true;
}

EAbleAbilityTaskRealm UAbleCollisionQueryTask::GetTaskRealmBP_Implementation() const
{
	return m_TaskRealm;
}

#undef LOCTEXT_NAMESPACE