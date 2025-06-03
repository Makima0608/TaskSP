// Fill out your copyright notice in the Description page of Project Settings.


#include "Classes/Tasks/SPAbilityCollisionFilterBase.h"

USPAbilityCollisionFilterBase::USPAbilityCollisionFilterBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

USPAbilityCollisionFilterBase::~USPAbilityCollisionFilterBase()
{
}

void USPAbilityCollisionFilterBase::Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	TArray<FAbleQueryResult>& InOutArray) const
{
	FilterBP(Context.Get(), InOutArray);
}

void USPAbilityCollisionFilterBase::FilterBP_Implementation(const UAbleAbilityContext* Context,
	TArray<FAbleQueryResult>& InOutArray) const
{
}
