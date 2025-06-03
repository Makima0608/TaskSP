// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingSphere.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta = (DisplayName = "Sphere", ShortToolTip = "A sphere based targeting volume."))
class UAbleTargetingSphere : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingSphere(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingSphere();

	/* Find all the Targets within our Query. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	/* Bind dynamic delegates. */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif
private:
	/* Calculate the range of the Query. */
	virtual float CalculateRange() const override;

	/* Helper method to process the results of the Query. */
	void ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

	/* Radius of the Sphere. */
	UPROPERTY(EditInstanceOnly, Category = "Sphere", meta = (DisplayName = "Radius", ClampMin=0.1f, AbleBindablePropety))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;
};

#undef LOCTEXT_NAMESPACE