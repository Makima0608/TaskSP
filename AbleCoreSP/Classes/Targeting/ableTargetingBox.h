// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingBox.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta=(DisplayName="Box", ShortToolTip="A box based targeting volume."))
class UAbleTargetingBox : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingBox();

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

	/* Helper method to return an AABB that fits the Query's rotation.*/
	FVector GetAlignedBox(const UAbleAbilityContext& Context, FTransform& OutQueryTransform) const;

	/* Process the Targeting Query.*/
	void ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AbleBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAbleVector m_HalfExtentsDelegate;
};

#undef LOCTEXT_NAMESPACE