// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingMisc.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, HideCategories=(Location, Targeting, Optimization), meta = (DisplayName = "Instigator", ShortToolTip = "Sets the Instigator as the target."))
class UAbleTargetingInstigator : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingInstigator(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingInstigator();

	/* Returns the Instigator Context Target. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Targeting, Optimization), meta = (DisplayName = "Self", ShortToolTip = "Sets Self as the target."))
class UAbleTargetingSelf : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingSelf(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingSelf();

	/* Returns the Self Context Target. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Targeting, Optimization), meta = (DisplayName = "Owner", ShortToolTip = "Sets Owner as the target."))
class UAbleTargetingOwner : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingOwner(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingOwner();

	/* Returns the Owner Context Target. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Custom", ShortToolTip = "Execute custom targeting logic."))
class UAbleTargetingCustom : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingCustom(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingCustom();

	/* Returns the any found Targets returned by the Ability's CustomTargetingFindTargets method. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};


UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Blackboard", ShortToolTip = "Grab a target from a Blackboard."))
class UAbleTargetingBlackboard : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingBlackboard(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingBlackboard();

	/* Returns the any found Targets returned by the Ability's CustomTargetingFindTargets method. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	/* What Keys to use when attempting to grab an Actor from the Blackboard */
	UPROPERTY(EditInstanceOnly, Category = "Blackboard", meta = (DisplayName = "Blackboard Keys"))
	TArray<FName> m_BlackboardKeys;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

UCLASS(EditInlineNew, HideCategories = (Location, Optimization), meta = (DisplayName = "Location", ShortToolTip = "Grab a Location from the Ability."))
class UAbleTargetingLocation : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingLocation(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingLocation();

	/* Populates the Location we're targeting by calling the Ability's GetTargetLocation method.*/
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
};

#undef LOCTEXT_NAMESPACE