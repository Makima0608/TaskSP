// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingRandomLocations.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UENUM()
enum ETargetingShape
{
	Sphere,
	Box,
};

UCLASS(EditInlineNew, meta=(DisplayName="Random Location", ShortToolTip="Use assigned number of random locations as target."))
class UAbleTargetingRandomLocations : public UAbleTargetingBase
{
	GENERATED_BODY()

public:
	UAbleTargetingRandomLocations(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingRandomLocations();

	/* Find all the Targets within our query.*/
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	/* Bind our Delegates. */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif

private:
	/* Calculate the range of our query. */
	virtual float CalculateRange() const override;

	UPROPERTY(EditInstanceOnly, Category = "Random Locations", meta = (DisplayName = "Targeting Shape", AbleBindableProperty, ToolTip="Shape that used to generate random location inside, 0: Sphere, 1: Box"))
	int32 m_TargetingShape = 0;

	UPROPERTY()
	FGetAbleInt m_TargetingShapeDelegate;

	UPROPERTY(EditInstanceOnly, Category = "Random Locations", meta = (DisplayName = "Location Numbers", AbleBindableProperty))
	int32 m_LocationNum = 0;

	UPROPERTY()
	FGetAbleInt m_LocationNumDelegate;

	UPROPERTY(EditInstanceOnly, Category = "Random Locations", meta = (DisplayName = "Ignore Z Axis", AbleBindableProperty, ToolTip="Targeting shape will created as same height as targeting source."))
	bool m_IgnoreZAxis = true;

	UPROPERTY()
	FGetAbleBool m_IgnoreZAxisDelegate;

	/* Min range from ability owner's location */
	UPROPERTY(EditInstanceOnly, Category = "Random Locations|Sphere", meta = (DisplayName = "Inner Radius", AbleBindableProperty, EditCondition="m_TargetingShape==0"))
	float m_InnerRadius = 0;

	UPROPERTY()
	FGetAbleFloat m_InnerRadiusDelegate;

	/* Max range from ability owner's location */
	UPROPERTY(EditInstanceOnly, Category = "Random Locations|Sphere", meta = (DisplayName = "Outer Radius", AbleBindableProperty, EditCondition="m_TargetingShape==0"))
	float m_OuterRadius = 0;

	UPROPERTY()
	FGetAbleFloat m_OuterRadiusDelegate;

	/* Min range from ability owner's location */
	UPROPERTY(EditInstanceOnly, Category = "Random Locations|Box", meta = (DisplayName = "Inner Extent", AbleBindableProperty, EditCondition="m_TargetingShape==1"))
	FVector m_InnerExtent;

	UPROPERTY()
	FGetAbleVector m_InnerExtentDelegate;

	/* Max range from ability owner's location */
	UPROPERTY(EditInstanceOnly, Category = "Random Locations|Box", meta = (DisplayName = "Outer Extent", AbleBindableProperty, EditCondition="m_TargetingShape==1"))
	FVector m_OuterExtent;

	UPROPERTY()
	FGetAbleVector m_OuterExtentDelegate;
};

#undef LOCTEXT_NAMESPACE
