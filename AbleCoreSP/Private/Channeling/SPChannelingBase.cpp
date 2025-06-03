// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Channeling/SPChannelingBase.h"

USPChannelingBase::USPChannelingBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USPChannelingBase::~USPChannelingBase()
{
}

bool USPChannelingBase::RequiresServerNotificationOfFailureBP_Implementation() const
{
	return false;
}

EAbleConditionResults USPChannelingBase::CheckConditionalBP_Implementation(UAbleAbilityContext* Context) const
{
	return EAbleConditionResults::ACR_Passed;
}
