// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableRayCastQueryTask.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityComponent.h"
#include "ableAbilityDebug.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tasks/ableValidation.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleRayCastQueryTaskScratchPad::UAbleRayCastQueryTaskScratchPad()
	: AsyncHandle(),
	AsyncProcessed(false)
{

}

UAbleRayCastQueryTaskScratchPad::~UAbleRayCastQueryTaskScratchPad()
{

}

UAbleRayCastQueryTask::UAbleRayCastQueryTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Length(100.0f),
	m_OnlyReturnBlockingHit(false),
	m_ReturnFaceIndex(false),
	m_ReturnPhysicalMaterial(false),
	m_QueryLocation(),
	m_UseEndLocation(false),
	m_QueryEndLocation(),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_CopyResultsToContext(true),
	m_AllowDuplicateEntries(false),
	m_ClearExistingTargets(false),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server),
	m_UseAsyncQuery(false)
{

}

UAbleRayCastQueryTask::~UAbleRayCastQueryTask()
{

}

FString UAbleRayCastQueryTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleRayCastQueryTask");
}

void UAbleRayCastQueryTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleRayCastQueryTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	FAbleAbilityTargetTypeLocation QueryLocation = m_QueryLocation;
	AActor* SourceActor = QueryLocation.GetSourceActor(*Context);
    if (SourceActor == nullptr)
	{
        return;
	}

	UWorld* World = SourceActor->GetWorld();

	FTransform QueryTransform;
	QueryLocation.GetTransform(*Context, QueryTransform);

	float Length = m_Length;
	const bool OnlyReturnBlockingHit = m_OnlyReturnBlockingHit;
	const bool ReturnFaceIndex = m_ReturnFaceIndex;
	const bool ReturnPhysMaterial = m_ReturnPhysicalMaterial;

	const FVector RayStart = QueryTransform.GetLocation();
	FVector RayEnd = RayStart + QueryTransform.GetRotation().GetForwardVector() * Length;

	if (m_UseEndLocation)
	{
		FAbleAbilityTargetTypeLocation QueryEndLocation = m_QueryEndLocation;
		FTransform QueryEndTransform;
		QueryEndLocation.GetTransform(*Context, QueryEndTransform);
		RayEnd = QueryEndTransform.GetLocation();
	}

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context, m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnFaceIndex = ReturnFaceIndex;
	QueryParams.bReturnPhysicalMaterial = ReturnPhysMaterial;

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Starting Raycast from %s to %s."), *RayStart.ToString(), *RayEnd.ToString()));
	}
#endif

	if (m_UseAsyncQuery && USPAbleSettings::IsAsyncEnabled())
	{
		UAbleRayCastQueryTaskScratchPad* ScratchPad = Cast<UAbleRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		ScratchPad->AsyncProcessed = false;
		if (OnlyReturnBlockingHit)
		{
			ScratchPad->AsyncHandle = World->AsyncLineTraceByObjectType(EAsyncTraceType::Single, RayStart, RayEnd, ObjectQuery, QueryParams);
		}
		else
		{
			ScratchPad->AsyncHandle = World->AsyncLineTraceByObjectType(EAsyncTraceType::Multi, RayStart, RayEnd, ObjectQuery, QueryParams);
		}
	}
	else
	{
		TArray<FHitResult> HitResults;
		FHitResult TraceResult;
		if (OnlyReturnBlockingHit)
		{
			if (World->LineTraceSingleByObjectType(TraceResult, RayStart, RayEnd, ObjectQuery, QueryParams))
			{
				HitResults.Add(TraceResult);
			}
		}
		else
		{
			World->LineTraceMultiByObjectType(HitResults, RayStart, RayEnd, ObjectQuery, QueryParams);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Raycast found %d results."), HitResults.Num()));
		}
#endif
		// Run any filters.
		if (m_Filters.Num())
		{
			TArray<FAbleQueryResult> QueryResults;
			QueryResults.Reserve(HitResults.Num());

			for (const FHitResult& Hit : HitResults)
			{
				QueryResults.Add(FAbleQueryResult(Hit));
			}

			// Run filters.
			for (UAbleCollisionFilter* Filter : m_Filters)
			{
				Filter->Filter(Context, QueryResults);
			}

			// Fix up our raw List.
			HitResults.RemoveAll([&](const FHitResult& RHS)
			{
				return !QueryResults.Contains(FAbleQueryResult(RHS));
			});
		}

		if (HitResults.Num())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				// Quick distance print help to see if we hit ourselves.
				float DistanceToBlocker = HitResults[HitResults.Num() - 1].Distance;
				PrintVerbose(Context, FString::Printf(TEXT("Raycast blocking hit distance: %4.2f."), DistanceToBlocker));
			}
#endif

			if (m_CopyResultsToContext)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Copying %d results into Context."), HitResults.Num()));
				}
#endif
				CopyResultsToContext(HitResults, Context);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Firing Raycast Event %s with %d results."), *m_Name.ToString(), HitResults.Num()));
				}
#endif
				Context->GetAbility()->OnRaycastEventBP(Context, m_Name, HitResults);
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawRaycastQuery(World, RayStart, RayEnd);
	}
#endif
}

void UAbleRayCastQueryTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleRayCastQueryTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

	if (IsAsyncFriendly() && USPAbleSettings::IsAsyncEnabled())
	{
		UAbleRayCastQueryTaskScratchPad* ScratchPad = Cast<UAbleRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		if (!ScratchPad->AsyncProcessed && ScratchPad->AsyncHandle._Handle != 0)
		{
			FAbleAbilityTargetTypeLocation QueryLocation = m_QueryLocation;

			AActor* SourceActor = QueryLocation.GetSourceActor(*Context);
            if (SourceActor == nullptr)
			{
                return;
			}

			UWorld* World = SourceActor->GetWorld();

			FTraceDatum Datum;
			if (World->QueryTraceData(ScratchPad->AsyncHandle, Datum))
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Async Raycast found %d results."), Datum.OutHits.Num()));
				}
#endif
				// Run any filters.
				if (m_Filters.Num())
				{
					TArray<FAbleQueryResult> QueryResults;
					QueryResults.Reserve(Datum.OutHits.Num());
					for (const FHitResult& Hit : Datum.OutHits)
					{
						QueryResults.Add(FAbleQueryResult(Hit));
					}

					// Run filters.
					for (UAbleCollisionFilter* Filter : m_Filters)
					{
						Filter->Filter(Context, QueryResults);
					}

					// Fix up our raw List.
					Datum.OutHits.RemoveAll([&](const FHitResult& RHS)
					{
						return !QueryResults.Contains(FAbleQueryResult(RHS));
					});
				}

				if (m_CopyResultsToContext)
				{
					CopyResultsToContext(Datum.OutHits, Context);
				}

				if (m_FireEvent)
				{
					Context->GetAbility()->OnRaycastEventBP(Context, m_Name, Datum.OutHits);
				}

				ScratchPad->AsyncProcessed = true;
			}

		}
	}
}

bool UAbleRayCastQueryTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleRayCastQueryTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (IsAsyncFriendly() && USPAbleSettings::IsAsyncEnabled())
	{
		UAbleRayCastQueryTaskScratchPad* ScratchPad = Cast<UAbleRayCastQueryTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return true;
		return ScratchPad->AsyncProcessed;
	}
	else
	{
		return UAbleAbilityTask::IsDone(Context);
	}
}

UAbleAbilityTaskScratchPad* UAbleRayCastQueryTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (IsAsyncFriendly() && USPAbleSettings::IsAsyncEnabled())
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleRayCastQueryTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}
		
		return NewObject<UAbleRayCastQueryTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleRayCastQueryTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleRayCastQueryTask, STATGROUP_Able);
}

void UAbleRayCastQueryTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Length, TEXT("Length"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OnlyReturnBlockingHit, TEXT("Only Return Blocking Hit"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ReturnFaceIndex, TEXT("Return Face Index"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ReturnPhysicalMaterial, TEXT("Return Physical Material"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_QueryLocation, TEXT("Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_QueryEndLocation, TEXT("End Location"));
}

void UAbleRayCastQueryTask::CopyResultsToContext(const TArray<FHitResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		TArray<TWeakObjectPtr<AActor>> AdditionTargets;
		AdditionTargets.Reserve(InResults.Num());
		for (const FHitResult& Result : InResults)
		{
			AdditionTargets.Add(Result.GetActor());
		}

		AbilityComponent->AddAdditionTargetsToContext(Context, AdditionTargets, m_AllowDuplicateEntries, m_ClearExistingTargets);
	}
}

#if WITH_EDITOR

FText UAbleRayCastQueryTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleRayCastQueryFormat", "{0}: {1}");
	FString ShapeDescription = FString::Printf(TEXT("%.1fm"), m_Length * 0.01f);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ShapeDescription));
}

EDataValidationResult UAbleRayCastQueryTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_QueryLocation.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
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
                ValidationErrors.Add(FText::Format(LOCTEXT("NoQueryDependency", "Trying to use Target Actor but there's no Ability Targeting or Query( with GetCopyResultsToContext )and a Dependency Defined on this Task: {0}. You need one of those conditions to properly use this target."), FText::FromString(AbilityContext->GetAbilityName())));
                result = EDataValidationResult::Invalid;
            }
        }
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnRaycastEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnRaycastEventBP_NotFound", "Function 'OnRaycastEventBP' not found: {0}"), FText::FromString(AbilityContext->GetAbilityName())));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

void UAbleRayCastQueryTask::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
    FTransform QueryTransform;
    m_QueryLocation.GetTransform(Context, QueryTransform);
	FVector Start = QueryTransform.GetLocation();
	FVector End = Start + QueryTransform.GetRotation().GetForwardVector() * m_Length;
    FAbleAbilityDebug::DrawRaycastQuery(Context.GetWorld(), Start, End);
}

#endif

bool UAbleRayCastQueryTask::IsSingleFrameBP_Implementation() const { return !m_UseAsyncQuery; }

EAbleAbilityTaskRealm UAbleRayCastQueryTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE