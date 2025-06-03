// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingMisc.h"

#include "ableAbility.h"
#include "ableAbilityContext.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ableAbilityUtilities.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UAbleTargetingInstigator::UAbleTargetingInstigator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleTargetingInstigator::~UAbleTargetingInstigator()
{

}

void UAbleTargetingInstigator::FindTargets(UAbleAbilityContext& Context) const
{
	if (AActor* InstigatorActor = Context.GetInstigator())
	{
		Context.GetMutableTargetActors().Add(InstigatorActor);
	}
	
	// No need to run filters.
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingInstigator::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleTargetingSelf::UAbleTargetingSelf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleTargetingSelf::~UAbleTargetingSelf()
{

}

void UAbleTargetingSelf::FindTargets(UAbleAbilityContext& Context) const
{
	if (AActor* SelfActor = Context.GetSelfActor())
	{
		Context.GetMutableTargetActors().Add(SelfActor);
	}

	// Skip filters.
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingSelf::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleTargetingOwner::UAbleTargetingOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleTargetingOwner::~UAbleTargetingOwner()
{

}

void UAbleTargetingOwner::FindTargets(UAbleAbilityContext& Context) const
{
	if (AActor* OwnerActor = Context.GetOwner())
	{
		Context.GetMutableTargetActors().Add(OwnerActor);
	}

	// Skip filters.
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingOwner::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleTargetingCustom::UAbleTargetingCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleTargetingCustom::~UAbleTargetingCustom()
{

}

void UAbleTargetingCustom::FindTargets(UAbleAbilityContext& Context) const
{
	TArray<AActor*> FoundTargets;
	Context.GetAbility()->CustomTargetingFindTargetsBP(&Context, FoundTargets);
	Context.GetMutableTargetActors().Append(FoundTargets);

	// Run any extra filters.
	FilterTargets(Context);
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingCustom::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomTargetingFindTargetsBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomTargetingFindTargetsBP_NotFound", "Function 'CustomTargetingFindTargetsBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}
#endif

UAbleTargetingBlackboard::UAbleTargetingBlackboard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleTargetingBlackboard::~UAbleTargetingBlackboard()
{

}

void UAbleTargetingBlackboard::FindTargets(UAbleAbilityContext& Context) const
{
	if (UBlackboardComponent* SelfBlackboard = FAbleAbilityUtilities::GetBlackboard(Context.GetSelfActor()))
	{
		TArray<TWeakObjectPtr<AActor>>& MutableTargets = Context.GetMutableTargetActors();
		for (const FName& BBKey : m_BlackboardKeys)
		{
			if (AActor* targetActor = Cast<AActor>(SelfBlackboard->GetValueAsObject(BBKey)))
			{
				MutableTargets.Add(targetActor);
			}
		}
	}

	FilterTargets(Context);
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingBlackboard::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_BlackboardKeys.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoBlackboardKeys", "No Blackboard keys defined for Blackboard targeting: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAbleTargetingLocation::UAbleTargetingLocation(const FObjectInitializer& ObjectInitializer)
	: Super (ObjectInitializer)
{

}

UAbleTargetingLocation::~UAbleTargetingLocation()
{

}

void UAbleTargetingLocation::FindTargets(UAbleAbilityContext& Context) const
{
	if (Context.GetTargetLocation().SizeSquared() < SMALL_NUMBER)
	{
		Context.SetTargetLocation(Context.GetAbility()->GetTargetLocationBP(&Context));
	}
}

#if WITH_EDITOR
EDataValidationResult UAbleTargetingLocation::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;    
    return result;
}
#endif
