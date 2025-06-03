// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "Tasks/IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingRaycast.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(EditInlineNew, meta = (DisplayName = "Raycast", ShortToolTip = "A ray cast based targeting query."))
class UAbleTargetingRaycast : public UAbleTargetingBase
{
	GENERATED_BODY()
public:
	UAbleTargetingRaycast(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingRaycast();

	/* Find all the Targets with our Query. */
	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	/* Bind Dynamic Delegates. */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif
private:
	/* Calculate the range of the Query. */
	virtual float CalculateRange() const override;

	/* Helper Method to process the Targets of this query. */
	void ProcessResults(UAbleAbilityContext& Context, const TArray<struct FHitResult>& Results) const;

	/* Length of the Ray */
	UPROPERTY(EditInstanceOnly, Category = "Raycast", meta = (DisplayName = "Length", ClampMin=0.01f, AbleBindableProperty))
	float m_Length;

	UPROPERTY()
	FGetAbleFloat m_LengthDelegate;

	/* If True, this Target will *Only* return the object that caused the blocking hit. */
	UPROPERTY(EditInstanceOnly, Category = "Raycast", meta = (DisplayName = "Return Blocking Object", ClampMin = 0.01f))
	bool m_OnlyWantBlockingObject;
};

#undef LOCTEXT_NAMESPACE