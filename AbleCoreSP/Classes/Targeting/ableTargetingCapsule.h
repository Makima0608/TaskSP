// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"
#include "Tasks/IAbleAbilityTask.h"

#include "ableTargetingCapsule.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based targeting volume."))
class UAbleTargetingCapsule : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingCapsule();

	/* Find all the Targets within our query.*/
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif

private:
	/* Calculate the range of our Query.*/
	virtual float CalculateRange() const override;

	/* Process the results of our Query. */
	void ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

	/* Height of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Height", AbleBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAbleFloat m_HeightDelegate;

	/* Radius of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Radius", AbleBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;
};

#undef LOCTEXT_NAMESPACE