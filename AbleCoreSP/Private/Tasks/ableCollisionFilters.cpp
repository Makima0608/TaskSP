// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCollisionFilters.h"

#include "ableAbilityBlueprintLibrary.h"
#include "AbleCoreSPPrivate.h"
#include "ableAbilityContext.h"
#include "ableAbilityDebug.h"
#include "ableAbilityTypes.h"
#include "ableAbilityUtilities.h"
#include "ableSettings.h"

#include "Async/Future.h"
#include "Async/Async.h"

#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCollisionFilter::UAbleCollisionFilter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleCollisionFilter::~UAbleCollisionFilter()
{

}

void UAbleCollisionFilter::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	verifyf(false, TEXT("This method should never be called. Did you forget to override it in your child class?"));
}

FName UAbleCollisionFilter::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_CollisionFilter_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

UAbleCollisionFilterSelf::UAbleCollisionFilterSelf(const FObjectInitializer& ObjectInitializer)
	: Super (ObjectInitializer)
{

}

UAbleCollisionFilterSelf::~UAbleCollisionFilterSelf()
{

}

void UAbleCollisionFilterSelf::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAbleQueryResult& LHS) { return LHS.Actor == Context->GetSelfActor(); });
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterSelf::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

bool UAbleCollisionFilter::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));
	
	if (oldFlags != GetFlags())
	{
		Modify();
		return true;
	}

	return false;
}
#endif

UAbleCollisionFilterOwner::UAbleCollisionFilterOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleCollisionFilterOwner::~UAbleCollisionFilterOwner()
{

}

void UAbleCollisionFilterOwner::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAbleQueryResult& LHS) { return LHS.Actor == Context->GetOwner(); });
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterOwner::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleCollisionFilterInstigator::UAbleCollisionFilterInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleCollisionFilterInstigator::~UAbleCollisionFilterInstigator()
{

}

void UAbleCollisionFilterInstigator::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	InOutArray.RemoveAll([&](const FAbleQueryResult& LHS) { return LHS.Actor == Context->GetInstigator(); });
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterInstigator::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleCollisionFilterByClass::UAbleCollisionFilterByClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_Class(nullptr), m_Negate(false)
{
}

UAbleCollisionFilterByClass::~UAbleCollisionFilterByClass()
{

}

void UAbleCollisionFilterByClass::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	bool Remove = false;
	for (int i = 0; i < InOutArray.Num(); )
	{
		if (AActor* Actor = InOutArray[i].Actor.Get())
		{
			Remove = Actor->GetClass()->IsChildOf(m_Class);
			Remove = m_Negate ? !Remove : Remove;
		}
		else
		{
			Remove = true;
		}

		if (Remove)
		{
			InOutArray.RemoveAt(i, 1, false);
		}
		else
		{
			++i;
		}
	}

	InOutArray.Shrink();
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterByClass::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_Class == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoClassDefined", "No Class Defined for Filtering: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAbleCollisionFilterSortByDistance::UAbleCollisionFilterSortByDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_SortDirection(AbleFitlerSort_Ascending), m_Use2DDistance(false)
{
}

UAbleCollisionFilterSortByDistance::~UAbleCollisionFilterSortByDistance()
{

}

void UAbleCollisionFilterSortByDistance::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	FTransform SourceTransform;
	FAbleAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
	Location.GetTransform(*Context.Get(), SourceTransform);
	InOutArray.Sort(FAbleAbilityResultSortByDistance(SourceTransform.GetLocation(), m_Use2DDistance, m_SortDirection == EAbleCollisionFilterSort::AbleFitlerSort_Ascending));
}

void UAbleCollisionFilterSortByDistance::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Source Location"));
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterSortByDistance::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleCollisionFilterMaxResults::UAbleCollisionFilterMaxResults(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_MaxEntities(0)
{
}

UAbleCollisionFilterMaxResults::~UAbleCollisionFilterMaxResults()
{

}

void UAbleCollisionFilterMaxResults::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	if (InOutArray.Num() > m_MaxEntities)
	{
		int32 NumberToTrim = InOutArray.Num() - m_MaxEntities;
		if (NumberToTrim > 0 && NumberToTrim < InOutArray.Num())
		{
			InOutArray.RemoveAt(InOutArray.Num() - NumberToTrim, NumberToTrim);
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterMaxResults::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_MaxEntities == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("InvalidFilterCount", "Filter is 0, will exclude all entities: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAbleCollisionFilterCustom::UAbleCollisionFilterCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_EventName(NAME_None),
	m_UseAsync(false)
{

}

UAbleCollisionFilterCustom::~UAbleCollisionFilterCustom()
{

}

void UAbleCollisionFilterCustom::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	check(Context.IsValid());
	const UAbleAbility* Ability = Context->GetAbility();
	check(Ability);

	if (m_UseAsync && USPAbleSettings::IsAsyncEnabled())
	{
		TArray<TFuture<bool>> FutureResults;
		FutureResults.Reserve(InOutArray.Num());

		FName EventName = m_EventName;
		for (FAbleQueryResult& Result : InOutArray)
		{
			FutureResults.Add(Async(EAsyncExecution::TaskGraph, [&Ability, &Context, EventName, &Result]
			{
				return Ability->CustomFilterConditionBP(Context.Get(), EventName, Result.Actor.Get());
			}));
		}

		check(FutureResults.Num() == InOutArray.Num());
		int32 InOutIndex = 0;
		for (TFuture<bool>& Future : FutureResults)
		{
			if (!Future.IsReady())
			{
				static const FTimespan OneMillisecond = FTimespan::FromMilliseconds(1.0);
				Future.WaitFor(OneMillisecond);
			}

			if (!Future.Get())
			{
				// Target passed filtering, moving on.
				InOutArray.RemoveAt(InOutIndex, 1, false);
			}
			else
			{
				++InOutIndex;
			}
		}

		InOutArray.Shrink();
	}
	else
	{
		for (int i = 0; i < InOutArray.Num(); )
		{
			if (!Ability->CustomFilterConditionBP(Context.Get(), m_EventName, InOutArray[i].Actor.Get()))
			{
				InOutArray.RemoveAt(i, 1, false);
			}
			else
			{
				++i;
			}
		}

		InOutArray.Shrink();
	}
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterCustom::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomFilterConditionBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomFilterConditionBP_NotFound", "Function 'CustomFilterConditionBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}
#endif

UAbleCollisionFilterTeamAttitude::UAbleCollisionFilterTeamAttitude(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, m_IgnoreAttitude(0)
{
}

UAbleCollisionFilterTeamAttitude::~UAbleCollisionFilterTeamAttitude()
{
}

void UAbleCollisionFilterTeamAttitude::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	AActor* SelfActor = Context->GetSelfActor();

	InOutArray.RemoveAll([&](const FAbleQueryResult& LHS)
	{
		ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(SelfActor, LHS.Actor.Get());

		if (((1 << (int32)Attitude) & m_IgnoreAttitude) != 0)
		{
			return true;
		}
		return false;
	});
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterTeamAttitude::IsAbilityDataValid(const UAbleAbility* AbilityContext, TArray<FText>& ValidationErrors)
{
	EDataValidationResult result = EDataValidationResult::Valid;

	if (m_IgnoreAttitude == 0)
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("FilterAttitudeInvalid", "Not filtering any attitudes: {0}"), FText::FromString(AbilityContext->GetAbilityName())));
		result = EDataValidationResult::Invalid;
	}

	return result;
}
#endif

UAbleCollisionFilterLineOfSight::UAbleCollisionFilterLineOfSight(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAbleCollisionFilterLineOfSight::~UAbleCollisionFilterLineOfSight()
{

}

void UAbleCollisionFilterLineOfSight::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	UWorld* CurrentWorld = Context->GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Invalid World Object. Cannot run Line of Sight filter."));
		return;
	}

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
	for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionQueryParams CollisionParams;

	AActor* SelfActor = Context->GetSelfActor();
	FHitResult Hit;
	FTransform SourceTransform;
	
	FAbleAbilityTargetTypeLocation SourceLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
	
	SourceLocation.GetTransform(*Context.Get(), SourceTransform);

	FVector RaySource = SourceTransform.GetTranslation();
	FVector RayEnd;

	for (int i = 0; i < InOutArray.Num();)
	{
		Hit.Reset(1.0f, false);

		CollisionParams.ClearIgnoredActors();
		CollisionParams.AddIgnoredActor(SelfActor);
		CollisionParams.AddIgnoredActor(InOutArray[i].Actor.Get());

		RayEnd = InOutArray[i].Actor->GetActorLocation();

		if (CurrentWorld->LineTraceSingleByObjectType(Hit, RaySource, RayEnd, ObjectQuery, CollisionParams))
		{
			InOutArray.RemoveAt(i, 1, false);
		}
		else
		{
			++i;
		}

#if !UE_BUILD_SHIPPING
		if (FAbleAbilityDebug::ShouldDrawQueries())
		{
			DrawDebugLine(CurrentWorld, RaySource, RayEnd, Hit.bBlockingHit ? FColor::Red : FColor::Green, FAbleAbilityDebug::ShouldDrawInEditor(), FAbleAbilityDebug::GetDebugQueryLifetime());
		}
#endif
	}

	InOutArray.Shrink();
}

void UAbleCollisionFilterLineOfSight::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Source Location"));
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterLineOfSight::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
	EDataValidationResult result = EDataValidationResult::Valid;

	if (m_ChannelPresent == ACP_NonePresent && m_CollisionChannels.Num() == 0)
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("CollisionFilterLOSNoChannels", "No Collision Channels set for Line of Sight filter. {0}"), AssetName));
		result = EDataValidationResult::Invalid;
	}

	return result;
}
#endif

UAbleCollisionFilterUniqueActors::UAbleCollisionFilterUniqueActors(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAbleCollisionFilterUniqueActors::~UAbleCollisionFilterUniqueActors()
{

}

void UAbleCollisionFilterUniqueActors::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const
{
	for (int32 i = 0; i < InOutArray.Num(); ++i)
	{
		const FAbleQueryResult& currentEntry = InOutArray[i];
		for (int32 j = i + 1; j < InOutArray.Num();)
		{
			if (currentEntry.Actor != nullptr && currentEntry.Actor == InOutArray[j].Actor)
			{
				// Swap this entry with the last entry, decrement the count - saves shifting the array around.
				InOutArray[j] = InOutArray.Last();
				InOutArray.RemoveAt(InOutArray.Num() - 1, 1, false);

				// Redo this iteration since we could have just swapped in another dupe.
			}
			else
			{
				++j;
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionFilterUniqueActors::IsAbilityDataValid(const UAbleAbility* AbilityContext, TArray<FText>& ValidationErrors)
{
	return EDataValidationResult::Valid;
}
#endif