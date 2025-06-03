// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Channeling/ableChannelingBase.h"

UAbleChannelingBase::UAbleChannelingBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Negate(false)
{

}

UAbleChannelingBase::~UAbleChannelingBase()
{

}

EAbleConditionResults UAbleChannelingBase::GetConditionResult(UAbleAbilityContext& Context) const
{
	EAbleConditionResults Result = CheckConditional(Context);

	if (m_Negate)
	{
		switch(Result)
		{
			case EAbleConditionResults::ACR_Passed: Result = ACR_Failed; break;
			case EAbleConditionResults::ACR_Failed: Result = ACR_Passed; break;
			default: break;
		}
	}

	return Result;
}

FName UAbleChannelingBase::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Channeling_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

#if WITH_EDITOR
bool UAbleChannelingBase::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}
#endif