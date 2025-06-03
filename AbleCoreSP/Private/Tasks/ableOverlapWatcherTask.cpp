// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableOverlapWatcherTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "Engine/World.h"
#include "Tasks/ableValidation.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleOverlapWatcherTaskScratchPad::UAbleOverlapWatcherTaskScratchPad()
    : AsyncHandle()
    , TaskComplete(false)
	, HasClearedInitialTargets(false)
{
}

UAbleOverlapWatcherTaskScratchPad::~UAbleOverlapWatcherTaskScratchPad()
{
}

UAbleOverlapWatcherTask::UAbleOverlapWatcherTask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
      , m_FireEvent(false)
      , m_Name(NAME_None)
      , m_QueryShape(nullptr)
      , m_CopyResultsToContext(false)
      , m_AllowDuplicateEntries(false)
      , m_ClearExistingTargets(false)
      , m_ContinuallyClearTargets(false)
      , m_TaskRealm(EAbleAbilityTaskRealm::ATR_ClientAndServer)
{
}

UAbleOverlapWatcherTask::~UAbleOverlapWatcherTask()
{
}

FString UAbleOverlapWatcherTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleOverlapWatcherTask");
}

void UAbleOverlapWatcherTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleOverlapWatcherTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAbleOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
	ScratchPad->AsyncHandle._Handle = 0;
	ScratchPad->TaskComplete = false;
	ScratchPad->HasClearedInitialTargets = false;
	ScratchPad->IgnoreActors.Empty();

    CheckForOverlaps(Context);
}

void UAbleOverlapWatcherTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
    Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleOverlapWatcherTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

    CheckForOverlaps(Context);
}

void UAbleOverlapWatcherTask::CheckForOverlaps(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
    UAbleOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAbleOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return;
    
    if (m_QueryShape)
    {
        // Step 1: Handle the last async query first
        if (m_QueryShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
        {
            UWorld* QueryWorld = m_QueryShape->GetQueryWorld(Context);
            check(QueryWorld);

            if (QueryWorld->IsTraceHandleValid(ScratchPad->AsyncHandle, true))
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
                    ProcessResults(Results, Context);
                }
                else
                {
                    // Still a valid query pending, don't do another one
                    return;
                }
            }
        }

        // Do the next query
        if (m_QueryShape->IsAsync() && USPAbleSettings::IsAsyncEnabled())
        {
            ScratchPad->AsyncHandle = m_QueryShape->DoAsyncQuery(Context, ScratchPad->QueryTransform);
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

			ProcessResults(Results, Context);
        }
    }
}

bool UAbleOverlapWatcherTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleOverlapWatcherTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
    UAbleOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAbleOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return true;

    if (ScratchPad->TaskComplete)
	{
        return true;
	}

    return UAbleAbilityTask::IsDone(Context);
}

UAbleAbilityTaskScratchPad* UAbleOverlapWatcherTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleOverlapWatcherTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

    return NewObject<UAbleOverlapWatcherTaskScratchPad>(Context.Get());
}

TStatId UAbleOverlapWatcherTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleOverlapWatcherTask, STATGROUP_Able);
}


void UAbleOverlapWatcherTask::CopyResultsToContext(const TArray<FAbleQueryResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context, bool ClearTargets) const
{
    if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
    {
        TArray<TWeakObjectPtr<AActor>> AdditionTargets;
        AdditionTargets.Reserve(InResults.Num());
        for (const FAbleQueryResult& Result : InResults)
        {
            AdditionTargets.Add(Result.Actor);
        }
        AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, ClearTargets);
    }
}


void UAbleOverlapWatcherTask::ProcessResults(TArray<FAbleQueryResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
    UAbleOverlapWatcherTaskScratchPad* ScratchPad = Cast<UAbleOverlapWatcherTaskScratchPad>(Context->GetScratchPadForTask(this));
    if (!ScratchPad) return;

    for (const UAbleCollisionFilter* CollisionFilter : m_Filters)
    {
        CollisionFilter->Filter(Context, InResults);

#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Filter %s executed. Entries remaining: %d"), *CollisionFilter->GetName(), InResults.Num()));
        }
#endif
    }

	// We either haven't called clear targets at all yet, or we have some results and we want to continually call clear.
	bool NeedsClearCall = m_CopyResultsToContext && ( (m_ClearExistingTargets && !ScratchPad->HasClearedInitialTargets) || (InResults.Num() && m_ContinuallyClearTargets ) );

    // We could have filtered out all our entries, so check again if the array is empty.
    if (InResults.Num() || NeedsClearCall )
    {
        if (m_CopyResultsToContext)
        {
#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), InResults.Num()));
            }
#endif
            CopyResultsToContext(InResults, Context, NeedsClearCall);

			ScratchPad->HasClearedInitialTargets = true;
        }

        if (m_FireEvent)
        {
#if !(UE_BUILD_SHIPPING)
            if (IsVerbose())
            {
                PrintVerbose(Context, FString::Printf(TEXT("Firing Collision Event %s with %d results."), *m_Name.ToString(), InResults.Num()));
            }
#endif
            switch (Context->GetAbility()->OnCollisionEventBP(Context.Get(), m_Name, InResults))
            {
            case EAbleCallbackResult::Complete:
                ScratchPad->TaskComplete = true;
                break;
            case EAbleCallbackResult::IgnoreActors:
                for (FAbleQueryResult& Result : InResults)
				{
                    ScratchPad->IgnoreActors.Add(Result.Actor);
				}
                break;
            }
        }
    }
}

#if WITH_EDITOR

FText UAbleOverlapWatcherTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleWatchForOverlapTaskFormat", "{0}");
	return FText::FormatOrdered(FormatText, GetTaskName());
}

FText UAbleOverlapWatcherTask::GetRichTextTaskSummary() const
{
    FTextBuilder StringBuilder;

    StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

    FString Summary = m_QueryShape != nullptr ? m_QueryShape->DescribeShape() : TEXT("NULL");
    StringBuilder.AppendLineFormat(LOCTEXT("AbleWatchForOverlapTaskRichFmt", "\t- WatchForOverlap Shape: {0}"), FText::FromString(Summary));

    return StringBuilder.ToText();
}

EDataValidationResult UAbleOverlapWatcherTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
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

void UAbleOverlapWatcherTask::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
    if (m_QueryShape != nullptr)
    {
        m_QueryShape->OnAbilityEditorTick(Context, DeltaTime);
    }
}

#endif

EAbleAbilityTaskRealm UAbleOverlapWatcherTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE
