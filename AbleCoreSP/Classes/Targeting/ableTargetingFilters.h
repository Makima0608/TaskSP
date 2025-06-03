// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityContext.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingFilters.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

class UAbleAbilityContext;
struct FAbleQueryResult;
class UAbleTargetingBase;

/* Base class for all Targeting Filters. */
UCLASS(Abstract, BlueprintType)
class ABLECORESP_API UAbleAbilityTargetingFilter : public UObject
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilter(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilter();

	/* Override and filter out whatever you deem invalid.*/
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const;

#if WITH_EDITOR
	/* Fix up our flags. */
	bool FixUpObjectFlags();
#endif
};

UENUM()
enum EAbleTargetingFilterSort
{
	AbleTargetFilterSort_Ascending = 0 UMETA(DisplayName = "Ascending"),
	AbleTargetFilterSort_Descending UMETA(DisplayName = "Descending")
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sort by Distance", ShortToolTip = "Sorts Targets from Nearest to Furthest (Ascending), or Furthest to Nearest (Descending)."))
class ABLECORESP_API UAbleAbilityTargetingFilterSortByDistance : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterSortByDistance(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterSortByDistance();

	/* Sort the Targets by filter in either ascending or descending mode. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;

protected:
	/* Helper method to return the location for our distance logic. */
	FVector GetSourceLocation(const UAbleAbilityContext& Context, EAbleAbilityTargetType SourceType) const;

	/* If true, the Distance logic will only use the 2D (XY Plane) distance for calculations.*/
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName="Use 2D Distance"))
	bool m_Use2DDistance;

	/* What direction to sort the results. Near to Far (Ascending) or Far to Near (Descending). */
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Sort Direction"))
	TEnumAsByte<EAbleTargetingFilterSort> m_SortDirection;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter Self", ShortToolTip = "Filters out the Self Actor."))
class UAbleAbilityTargetingFilterSelf : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterSelf(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterSelf();

	/* Filter out the Self Context Target. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories=(Filter), meta = (DisplayName = "Filter Owner", ShortToolTip = "Filters out the Owner Actor."))
class UAbleAbilityTargetingFilterOwner : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterOwner(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterOwner();

	/* Filter out the Owner Context Target. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter Instigator", ShortToolTip = "Filters out the Instigator Actor."))
class UAbleAbilityTargetingFilterInstigator : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterInstigator(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterInstigator();

	/* Filter out the Instigator Context Target. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
};

UCLASS(EditInlineNew, hidecategories = (Filter), meta = (DisplayName = "Filter By Class", ShortToolTip = "Filters out any instances of the provided class, can also be negated to keep only instances of the provided class."))
class UAbleAbilityTargetingFilterClass : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterClass(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterClass();

	/* Filter by the provided class. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;

protected:
	/* The Class to filter by. */
	UPROPERTY(EditInstanceOnly, Category = "Class Filter", meta = (DisplayName = "Class"))
	const UClass* m_Class;

	/* If true, the filter will keep only items that are of the provided class rather than filter them out. */
	UPROPERTY(EditInstanceOnly, Category = "Class Filter", meta = (DisplayName = "Negate"))
	bool m_Negate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Max Targets", ShortToolTip = "Limit results to N Targets."))
class UAbleAbilityTargetingFilterMaxTargets : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterMaxTargets(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterMaxTargets();

	/* Keep all but N Targets.*/
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
protected:
	/* The Maximum Amount of Targets allowed. */
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Max Targets", ClampMin = 1))
	int32 m_MaxTargets;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Custom", ShortToolTip = "Calls the Ability's IsValidForActor Blueprint Event. If the event returns true, the actor is kept. If false, it is discarded."))
class UAbleAbilityTargetingFilterCustom : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterCustom(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterCustom();

	/* Call into the Ability's CustomFilter method. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
protected:
	// Optional Name identifier for this event in case you are using IsValidForActor multiple times in the Ability.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Event Name"))
	FName m_EventName;

	// If true, the event is run across multiple actors on various cores. This can help speed things up if the potential actor list is large, or the BP logic is complex.
	UPROPERTY(EditInstanceOnly, Category = "Optimize", meta = (DisplayName = "Use Async"))
	bool m_UseAsync;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Line Of Sight", ShortToolTip = "Casts a ray between the Target and the Source Location(Actor, Location, etc). If a blocking hit is found between the two, the target is discarded."))
class UAbleAbilityTargetingFilterLineOfSight : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterLineOfSight(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterLineOfSight();

	/* Call into the Ability's CustomFilter method. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
protected:
	// The Location to use as our source for our raycast.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Source Location"))
	FAbleAbilityTargetTypeLocation m_SourceLocation;

	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Collision Channel Present"))
	TEnumAsByte<EAbleChannelPresent> m_ChannelPresent;
	
	// The Collision Channels to run the Raycast against.
	UPROPERTY(EditInstanceOnly, Category = "Filter", meta = (DisplayName = "Collision Channels"))
	TArray<TEnumAsByte<ESPAbleTraceType>> m_CollisionChannels;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Unique Actors", ShortToolTip = "Reduce Collision results to only the unique results based on Actors hit rather than components hit."))
class UAbleAbilityTargetingFilterUniqueActors : public UAbleAbilityTargetingFilter
{
	GENERATED_BODY()
public:
	UAbleAbilityTargetingFilterUniqueActors(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityTargetingFilterUniqueActors();

	/* Filter the results. */
	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;
};


#undef LOCTEXT_NAMESPACE