// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ableTargetingFilters.h"
#include "UnLuaInterface.h"
#include "SPAbilityTargetingFilterBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, meta = (DisplayName = "FilterBase"))
class ABLECORESP_API USPAbilityTargetingFilterBase : public UAbleAbilityTargetingFilter, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	USPAbilityTargetingFilterBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USPAbilityTargetingFilterBase();

	virtual void Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const override;

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="Filter"))
	void FilterBP(UAbleAbilityContext* Context, const UAbleTargetingBase* TargetBase) const;

	virtual bool SkipObjectReferencer_Implementation() const override { return true; };
#if WITH_EDITOR
	// bool FixUpObjectFlags();
#endif
};
