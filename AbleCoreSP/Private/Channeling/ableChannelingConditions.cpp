// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Channeling/ableChannelingConditions.h"

#include "ableAbility.h"
#include "ableAbilityContext.h"
#include "ableAbilityUtilities.h"
#include "AbleCoreSPPrivate.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleChannelingInputConditional::UAbleChannelingInputConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseEnhancedInput(false)
{

}

UAbleChannelingInputConditional::~UAbleChannelingInputConditional()
{

}

EAbleConditionResults UAbleChannelingInputConditional::CheckConditional(UAbleAbilityContext& Context) const
{
	if (UAbleAbilityComponent* AbilityComponent = Context.GetSelfAbilityComponent())
	{
		if (!AbilityComponent->IsOwnerLocallyControlled())
		{
			// We can only check Input on local clients (due to keybindings, etc), assume its valid unless Client tells us otherwise.
			return EAbleConditionResults::ACR_Ignored;
		}
	}

#if WITH_EDITOR
	// UI Settings could change out from under us.
	m_InputKeyCache.Empty(m_InputKeyCache.Num());
#endif


	if (!m_InputKeyCache.Num() && !m_UseEnhancedInput)
	{
		for (const FName& InputAction : m_InputActions)
		{
			m_InputKeyCache.Append(FAbleAbilityUtilities::GetKeysForInputAction(InputAction));
		}
	}

	if (const AActor* SelfActor = Context.GetSelfActor())
	{
		if (const APawn* Pawn = Cast<APawn>(SelfActor))
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
					for (const FKey& Key : m_InputKeyCache)
					{
						if (PlayerController->IsInputKeyDown(Key))
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

#if WITH_EDITOR
EDataValidationResult UAbleChannelingInputConditional::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_InputActions.Num() == 0 && m_EnhancedInputActions.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoInputActions", "No Input actions for channel conditional: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAbleChannelingVelocityConditional::UAbleChannelingVelocityConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_VelocityThreshold(1.0f),
	m_UseXYSpeed(true)
{

}

UAbleChannelingVelocityConditional::~UAbleChannelingVelocityConditional()
{

}

void UAbleChannelingVelocityConditional::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_VelocityThreshold, "Velocity Threshold");
}

EAbleConditionResults UAbleChannelingVelocityConditional::CheckConditional(UAbleAbilityContext& Context) const
{
	bool success = false;
	if (const AActor* SelfActor = Context.GetSelfActor())
	{
		const FVector Velocity = SelfActor->GetVelocity();
		const float Threshold = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_VelocityThreshold);
		const float ThresholdSquared = Threshold * Threshold;

		if (m_UseXYSpeed)
		{
			success = Velocity.SizeSquared2D() < ThresholdSquared;
		}
		else
		{
			success = Velocity.SizeSquared() < ThresholdSquared;
		}
	}

	return success ? EAbleConditionResults::ACR_Passed : EAbleConditionResults::ACR_Failed;
}

UAbleChannelingDistanceConditional::UAbleChannelingDistanceConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_DistanceThreshold(0.0f),
	m_Use2DDistance(true)
{
}

UAbleChannelingDistanceConditional::~UAbleChannelingDistanceConditional()
{
}

#if WITH_EDITOR
EDataValidationResult UAbleChannelingDistanceConditional::IsTaskDataValid(const UAbleAbility* AbilityContext,
	const FText& AssetName, TArray<FText>& ValidationErrors)
{
	EDataValidationResult result = EDataValidationResult::Valid;
	return result;
}
#endif

EAbleConditionResults UAbleChannelingDistanceConditional::CheckConditional(UAbleAbilityContext& Context) const
{
	bool bSuccess = false;
	if (const AActor* SelfActor = Context.GetSelfActor())
	{
		const AActor* TargetActor = Context.GetTargetActorsWeakPtr().Num() > 0 ? Context.GetTargetActorsWeakPtr()[0].Get() : nullptr;
		if (TargetActor != nullptr)
		{
			const float ThresholdSquared = m_DistanceThreshold * m_DistanceThreshold;
			if (m_Use2DDistance)
			{
				bSuccess = FVector::DistSquaredXY(SelfActor->GetActorLocation(), TargetActor->GetActorLocation()) > ThresholdSquared;
			}
			else
			{
				bSuccess = FVector::DistSquared(SelfActor->GetActorLocation(), TargetActor->GetActorLocation()) > ThresholdSquared;
			}	
		}
	}
	return bSuccess ? EAbleConditionResults::ACR_Passed : EAbleConditionResults::ACR_Failed;
}

#if WITH_EDITOR
EDataValidationResult UAbleChannelingVelocityConditional::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    if (m_VelocityThreshold == 0.0f)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("InvalidVelocity", "Velocity threshold is zero, condition will never be true: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    return result;
}
#endif

UAbleChannelingCustomConditional::UAbleChannelingCustomConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_EventName(NAME_None)
{

}

UAbleChannelingCustomConditional::~UAbleChannelingCustomConditional()
{

}

EAbleConditionResults UAbleChannelingCustomConditional::CheckConditional(UAbleAbilityContext& Context) const
{
	check(Context.GetAbility() != nullptr);
	return Context.GetAbility()->CheckCustomChannelConditionalBP(&Context, m_EventName);
}

#if WITH_EDITOR
EDataValidationResult UAbleChannelingCustomConditional::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_EventName.IsNone())
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoEventName", "No Event Name For Custom Channel Conditional: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("CheckCustomChannelConditionalBP"));
    if (function == nullptr || function->Script.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("CheckCustomChannelConditionalBP_NotFound", "Function 'CheckCustomChannelConditionalBP' not found: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }
    
    return result;
}
#endif

UAbleChannelingBlockConditional::UAbleChannelingBlockConditional(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UAbleChannelingBlockConditional::~UAbleChannelingBlockConditional()
{
	
}

#if WITH_EDITOR
EDataValidationResult UAbleChannelingBlockConditional::IsTaskDataValid(const UAbleAbility* AbilityContext,
	const FText& AssetName, TArray<FText>& ValidationErrors)
{
	return EDataValidationResult::Valid;
}
#endif

EAbleConditionResults UAbleChannelingBlockConditional::CheckConditional(UAbleAbilityContext& Context) const
{
	return EAbleConditionResults::ACR_Failed;
}

#undef LOCTEXT_NAMESPACE
