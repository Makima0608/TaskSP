// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableBranchCondition.h"

#include "ableAbility.h"
#include "ableAbilityUtilities.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Tasks/ableBranchTask.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleBranchCondition::UAbleBranchCondition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Negate(false),
	m_DynamicPropertyIdentifer()
{

}

UAbleBranchCondition::~UAbleBranchCondition()
{

}

FName UAbleBranchCondition::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_BranchCondition_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

UAbleBranchConditionOnInput::UAbleBranchConditionOnInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_InputActions(),
	m_MustBeRecentlyPressed(false),
	m_RecentlyPressedTimeLimit(0.1f),
	m_UseEnhancedInput(false)
{

}

UAbleBranchConditionOnInput::~UAbleBranchConditionOnInput()
{

}

EAbleConditionResults UAbleBranchConditionOnInput::CheckCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context, UAbleBranchTaskScratchPad& ScratchPad) const
{
	// Build our cache
	if (ScratchPad.CachedKeys.Num() == 0 && !m_UseEnhancedInput)
	{
		TArray<FName> InputActions = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_InputActions);
		for (const FName& Action : InputActions)
		{
			ScratchPad.CachedKeys.Append(FAbleAbilityUtilities::GetKeysForInputAction(Action));
		}
	}

	if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		if (!AbilityComponent->IsOwnerLocallyControlled())
		{
			// We can only check Input on local clients.
			return EAbleConditionResults::ACR_Ignored;
		}
	}

	if (const APawn* Pawn = Cast<APawn>(Context->GetSelfActor()))
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			UEnhancedPlayerInput* EPI = Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput);
			if (EPI && m_UseEnhancedInput)
			{
				for (const UInputAction* Action : m_EnhancedInputActions)
				{
					if (EPI->GetActionValue(Action).Get<bool>())
					{
						return EAbleConditionResults::ACR_Passed;
					}
				}
			}
			else
			{
				for (const FKey& Key : ScratchPad.CachedKeys)
				{
					if (PlayerController->IsInputKeyDown(Key))
					{
						if (m_MustBeRecentlyPressed)
						{
							return PlayerController->GetInputKeyTimeDown(Key) <= m_RecentlyPressedTimeLimit ? EAbleConditionResults::ACR_Passed: EAbleConditionResults::ACR_Failed;
						}
						else
						{
							return EAbleConditionResults::ACR_Passed;
						}
					}
				}
			}
		}
	}

	return EAbleConditionResults::ACR_Failed;
}

void UAbleBranchConditionOnInput::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_InputActions, TEXT("Input Actions"));
}

#if WITH_EDITOR
EDataValidationResult UAbleBranchConditionOnInput::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_InputActions.Num() == 0 && m_EnhancedInputActions.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoInputActions", "No Input Actions Defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}

bool UAbleBranchCondition::FixUpObjectFlags()
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

UAbleBranchConditionAlways::UAbleBranchConditionAlways(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleBranchConditionAlways::~UAbleBranchConditionAlways()
{

}

#if WITH_EDITOR
EDataValidationResult UAbleBranchConditionAlways::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}
#endif

UAbleBranchConditionCustom::UAbleBranchConditionCustom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}


UAbleBranchConditionCustom::~UAbleBranchConditionCustom()
{

}

EAbleConditionResults UAbleBranchConditionCustom::CheckCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context, UAbleBranchTaskScratchPad& ScratchPad) const
{
	check(Context.IsValid() && ScratchPad.BranchAbility);

	return Context.Get()->GetAbility()->CustomCanBranchToBP(Context.Get(), ScratchPad.BranchAbility) ? EAbleConditionResults::ACR_Passed : EAbleConditionResults::ACR_Failed;
}

#if WITH_EDITOR
EDataValidationResult UAbleBranchConditionCustom::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    
    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CustomCanBranchToBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CustomCanBranchToBP_NotFound", "Function 'CustomCanBranchToBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}

#endif

UAbleAbilityBranchConditionRequireTarget::UAbleAbilityBranchConditionRequireTarget(
	const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer)
{
}

UAbleAbilityBranchConditionRequireTarget::~UAbleAbilityBranchConditionRequireTarget()
{
}

EAbleConditionResults UAbleAbilityBranchConditionRequireTarget::CheckCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context, UAbleBranchTaskScratchPad& ScratchPad) const
{
	if (Context->GetTargetActorsWeakPtr().Num() <= 0) return ACR_Failed;
	
	for (TWeakObjectPtr<AActor> Target : Context->GetTargetActorsWeakPtr())
	{
		if (!Target.IsValid())
		{
			return ACR_Failed;
		}
	}

	return ACR_Passed;
}

#undef LOCTEXT_NAMESPACE