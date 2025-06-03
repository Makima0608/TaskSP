// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingFilters.h"

#include "AbleCoreSPPrivate.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityContext.h"
#include "ableAbilityDebug.h"
#include "ableAbilityTypes.h"
#include "ableSettings.h"

#include "Async/Future.h"
#include "Async/Async.h"

#include "DrawDebugHelpers.h"
#include "Logging/LogMacros.h"
#include "Targeting/ableTargetingBase.h"

UAbleAbilityTargetingFilter::UAbleAbilityTargetingFilter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilter::~UAbleAbilityTargetingFilter()
{

}

void UAbleAbilityTargetingFilter::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	checkNoEntry();
}

#if WITH_EDITOR

bool UAbleAbilityTargetingFilter::FixUpObjectFlags()
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

// Sorter helpers
struct FAbleAbilityActorLocationTargetSorter
{
	FVector SourceLocation;
	bool XYOnly;
	bool Ascending; // Ascending for Nearest to Farthest, descending for the opposite.
	FAbleAbilityActorLocationTargetSorter(FVector& InSourceLocation, bool InXYOnly, bool InAscending)
		: SourceLocation(InSourceLocation), XYOnly(InXYOnly), Ascending(InAscending) {}

	FORCEINLINE bool operator()(const TWeakObjectPtr<AActor>& A, const TWeakObjectPtr<AActor>& B) const
	{
		if (XYOnly)
		{
			// I could probably ditch this if branch: (return DistanceCheck || !Ascending)
			// but I'd rather keep things explicit and branch prediction should take care of things anyway since its constant.
			if (Ascending) // We want nearest
			{
				return FVector::DistSquaredXY(A->GetActorLocation(), SourceLocation) < FVector::DistSquaredXY(B->GetActorLocation(), SourceLocation);
			}
			else
			{
				return FVector::DistSquaredXY(A->GetActorLocation(), SourceLocation) > FVector::DistSquaredXY(B->GetActorLocation(), SourceLocation);
			}
		}
		else
		{
			if (Ascending) // We want nearest
			{
				return FVector::DistSquared(A->GetActorLocation(), SourceLocation) < FVector::DistSquared(B->GetActorLocation(), SourceLocation);
			}
			else
			{
				return FVector::DistSquared(A->GetActorLocation(), SourceLocation) > FVector::DistSquared(B->GetActorLocation(), SourceLocation);
			}
		}
	}
};
/////

UAbleAbilityTargetingFilterSortByDistance::UAbleAbilityTargetingFilterSortByDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Use2DDistance(true),
	m_SortDirection(EAbleTargetingFilterSort::AbleTargetFilterSort_Ascending)
{

}

UAbleAbilityTargetingFilterSortByDistance::~UAbleAbilityTargetingFilterSortByDistance()
{

}

void UAbleAbilityTargetingFilterSortByDistance::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	FVector SourceLocation = GetSourceLocation(Context, TargetBase.GetSource());
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
	const int32 Count = TargetActors.RemoveAll([](const TWeakObjectPtr<AActor> Actor)
	{
		return !Actor.IsValid();
	});
	if (Count > 0)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("UAbleAbilityTargetingFilterSortByDistance::Filter TargetActor is not Valid Count = %d"), Count);
	}
	TargetActors.Sort(FAbleAbilityActorLocationTargetSorter(SourceLocation, m_Use2DDistance, m_SortDirection.GetValue() == EAbleTargetingFilterSort::AbleTargetFilterSort_Ascending));
}

FVector UAbleAbilityTargetingFilterSortByDistance::GetSourceLocation(const UAbleAbilityContext& Context, EAbleAbilityTargetType SourceType) const
{
	FVector ReturnVal(0.0f, 0.0f, 0.0f);
	if (SourceType == EAbleAbilityTargetType::ATT_Self)
	{
		if (AActor* Owner = Context.GetSelfActor())
		{
			ReturnVal = Owner->GetActorLocation();
		}
	}
	else if (SourceType == EAbleAbilityTargetType::ATT_Instigator)
	{
		if (AActor* Instigator = Context.GetInstigator())
		{
			ReturnVal = Instigator->GetActorLocation();
		}
	}
	else if (SourceType == EAbleAbilityTargetType::ATT_Owner)
	{
		if (AActor* Owner = Context.GetOwner())
		{
			ReturnVal = Owner->GetActorLocation();
		}
	}
	else if (SourceType == EAbleAbilityTargetType::ATT_Location)
	{
		ReturnVal = Context.GetTargetLocation();
	}
	else if (SourceType == EAbleAbilityTargetType::ATT_TargetActor)
	{
		UE_LOG(LogAbleSP, Error, TEXT("Targeting Filter has source set as 'TargetActor' or 'TargetComponent'. This is invalid as the target has not been found yet."));
	}
	else
	{
		checkNoEntry();
	}

	return ReturnVal;
}

UAbleAbilityTargetingFilterSelf::UAbleAbilityTargetingFilterSelf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilterSelf::~UAbleAbilityTargetingFilterSelf()
{

}

void UAbleAbilityTargetingFilterSelf::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetSelfActor(); });
}


UAbleAbilityTargetingFilterOwner::UAbleAbilityTargetingFilterOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilterOwner::~UAbleAbilityTargetingFilterOwner()
{

}

void UAbleAbilityTargetingFilterOwner::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetOwner(); });
}

UAbleAbilityTargetingFilterInstigator::UAbleAbilityTargetingFilterInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilterInstigator::~UAbleAbilityTargetingFilterInstigator()
{

}

void UAbleAbilityTargetingFilterInstigator::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll([&](const TWeakObjectPtr<AActor>& LHS) { return LHS == Context.GetInstigator(); });
}

UAbleAbilityTargetingFilterClass::UAbleAbilityTargetingFilterClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Class(nullptr),
	m_Negate(false)
{

}

UAbleAbilityTargetingFilterClass::~UAbleAbilityTargetingFilterClass()
{

}

// Filter Predicate.
struct FAbleAbilityTargetingClassFilterPredicate
{
	const UClass* ClassFilter;
	bool Negate;
	FAbleAbilityTargetingClassFilterPredicate(const UClass& InClass, bool InNegate)
		: ClassFilter(&InClass), Negate(InNegate) {}

	FORCEINLINE bool operator()(const TWeakObjectPtr<AActor>& A) const
	{
		if (A.IsValid())
		{
			bool IsChild = A->GetClass()->IsChildOf(ClassFilter);
			return Negate ? !IsChild : IsChild;
		}
		
		return true; // Default to removal
	}
};

void UAbleAbilityTargetingFilterClass::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	Context.GetMutableTargetActors().RemoveAll(FAbleAbilityTargetingClassFilterPredicate(*m_Class, m_Negate));
}

UAbleAbilityTargetingFilterMaxTargets::UAbleAbilityTargetingFilterMaxTargets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_MaxTargets(1)
{

}

UAbleAbilityTargetingFilterMaxTargets::~UAbleAbilityTargetingFilterMaxTargets()
{

}

void UAbleAbilityTargetingFilterMaxTargets::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	if (TargetActors.Num() > m_MaxTargets)
	{
		int32 NumberToTrim = TargetActors.Num() - m_MaxTargets;
		if (NumberToTrim > 0 && NumberToTrim < TargetActors.Num())
		{
			TargetActors.RemoveAt(TargetActors.Num() - NumberToTrim, NumberToTrim);
		}
	}
}

UAbleAbilityTargetingFilterCustom::UAbleAbilityTargetingFilterCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseAsync(false)
{

}

UAbleAbilityTargetingFilterCustom::~UAbleAbilityTargetingFilterCustom()
{

}

void UAbleAbilityTargetingFilterCustom::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
	const UAbleAbility* Ability = Context.GetAbility();
	check(Ability);

	if (m_UseAsync && USPAbleSettings::IsAsyncEnabled())
	{
		TArray<TFuture<bool>> FutureResults;
		FutureResults.Reserve(TargetActors.Num());

		FName EventName = m_EventName;
		for (TWeakObjectPtr<AActor>& TargetActor : TargetActors)
		{
			FutureResults.Add(Async(EAsyncExecution::TaskGraph, [&Ability, &Context, EventName, &TargetActor]
			{
				return Ability->CustomFilterConditionBP(&Context, EventName, TargetActor.Get());
			}));
		}

		check(FutureResults.Num() == TargetActors.Num());

		int TargetIndex = 0;
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
				TargetActors.RemoveAt(TargetIndex, 1, false);
			}
			else
			{
				++TargetIndex;
			}

		}

		TargetActors.Shrink();
	}
	else
	{
		for (int i = 0; i < TargetActors.Num(); )
		{
			if (!Ability->CustomFilterConditionBP(&Context, m_EventName, TargetActors[i].Get()))
			{
				TargetActors.RemoveAt(i, 1, false);
			}
			else
			{
				++i;
			}
		}

		TargetActors.Shrink();
	}
}

UAbleAbilityTargetingFilterLineOfSight::UAbleAbilityTargetingFilterLineOfSight(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilterLineOfSight::~UAbleAbilityTargetingFilterLineOfSight()
{

}

void UAbleAbilityTargetingFilterLineOfSight::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	UWorld* CurrentWorld = Context.GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Invalid World Object. Cannot run Line of Sight filter."));
		return;
	}

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		&Context, m_ChannelPresent, m_CollisionChannels);
	for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}
	
	FCollisionQueryParams CollisionParams;

	AActor* SelfActor = Context.GetSelfActor();
	FHitResult Hit;
	FTransform SourceTransform;
	m_SourceLocation.GetTransform(Context, SourceTransform);

	FVector RaySource = SourceTransform.GetTranslation();
	FVector RayEnd;

	TArray<TWeakObjectPtr<AActor>>& MutableTargets = Context.GetMutableTargetActors();
	for (int i = 0; i < MutableTargets.Num();)
	{
		Hit.Reset(1.0f, false);

		CollisionParams.ClearIgnoredActors();
		CollisionParams.AddIgnoredActor(SelfActor);
		CollisionParams.AddIgnoredActor(MutableTargets[i].Get());

		RayEnd = MutableTargets[i]->GetActorLocation();

		if (CurrentWorld->LineTraceSingleByObjectType(Hit, RaySource, RayEnd, ObjectQuery, CollisionParams))
		{
			MutableTargets.RemoveAt(i, 1, false);
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

	MutableTargets.Shrink();
}

UAbleAbilityTargetingFilterUniqueActors::UAbleAbilityTargetingFilterUniqueActors(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleAbilityTargetingFilterUniqueActors::~UAbleAbilityTargetingFilterUniqueActors()
{

}

void UAbleAbilityTargetingFilterUniqueActors::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
	for (int32 i = 0; i < TargetActors.Num(); ++i)
	{
		const TWeakObjectPtr<AActor>& currentEntry = TargetActors[i];
		for (int32 j = i + 1; j < TargetActors.Num();)
		{
			if (currentEntry == TargetActors[j])
			{
				// Swap this entry with the last entry, decrement the count - saves shifting the array around.
				TargetActors[j] = TargetActors.Last();
				TargetActors.RemoveAt(TargetActors.Num() - 1, 1, false);

				// Redo this iteration since we could have just swapped in another dupe.
			}
			else
			{
				++j;
			}
		}
	}
}
