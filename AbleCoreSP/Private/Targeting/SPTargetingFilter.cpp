// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/SPAbilityTargetingFilterBase.h"

USPAbilityTargetingFilterBase::USPAbilityTargetingFilterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USPAbilityTargetingFilterBase::~USPAbilityTargetingFilterBase()
{
}

void USPAbilityTargetingFilterBase::Filter(UAbleAbilityContext& Context, const UAbleTargetingBase& TargetBase) const
{
	FilterBP(&Context, &TargetBase);
}

void USPAbilityTargetingFilterBase::FilterBP_Implementation(UAbleAbilityContext* Context,
                                                            const UAbleTargetingBase* TargetBase) const
{
}
