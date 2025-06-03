// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityTypes.h"
#include "ableChannelingBase.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"

#include "InputAction.h"
#include "ableChannelingConditions.generated.h"

UCLASS(EditInlineNew, meta=(DisplayName="Input", ShortTooltip="Checks the provided Input Actions. If the key is pressed, it passes the condition - false otherwise"))
class ABLECORESP_API UAbleChannelingInputConditional : public UAbleChannelingBase
{
	GENERATED_BODY()
public:
	UAbleChannelingInputConditional(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleChannelingInputConditional();

	/* Requires Server notification. */
	virtual bool RequiresServerNotificationOfFailure() const { return true; }

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Override in your child class.*/
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override;

	// The input actions to look for. This expects the action keys to be bound to an action in the Input Settings.
	UPROPERTY(EditAnywhere, Category = "Conditional", meta = (DisplayName = "Input Actions"))
	TArray<FName> m_InputActions;

	UPROPERTY(EditAnywhere, Category = "Conditional|Enhanced", meta = (DisplayName = "Use Enhanced Input"))
	bool m_UseEnhancedInput;

	UPROPERTY(EditAnywhere, Category = "Conditional|Enhanced", meta = (DisplayName = "Enhanced Input Actions"))
	TArray<const UInputAction*> m_EnhancedInputActions;

	mutable TArray<struct FKey> m_InputKeyCache;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Velocity", ShortTooltip = "Returns false if the Actor's velocity is above the provided threshold."))
class ABLECORESP_API UAbleChannelingVelocityConditional : public UAbleChannelingBase
{
	GENERATED_BODY()
public:
	UAbleChannelingVelocityConditional(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleChannelingVelocityConditional();

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Override in your child class.*/
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override;

	// The speed (in cm/s) to check the actor velocity against.
	UPROPERTY(EditAnywhere, Category = "Conditional", meta = (DisplayName = "Velocity Threshold", AbleBindableProperty))
	float m_VelocityThreshold;

	UPROPERTY()
	FGetAbleFloat m_VelocityThresholdDelegate;

	// If true, we'll only check the 2D (XY) speed of the actor.
	UPROPERTY(EditAnywhere, Category = "Conditional", meta = (DisplayName = "Use XY Speed Only"))
	bool m_UseXYSpeed;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Distance", ShortTooltip = "Returns true if the distance from the Actor to the target is above the provided threshold."))
class ABLECORESP_API UAbleChannelingDistanceConditional : public UAbleChannelingBase
{
	GENERATED_BODY()
public:
	UAbleChannelingDistanceConditional(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleChannelingDistanceConditional();

#if WITH_EDITOR
	virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Override in your child class.*/
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override;

	// The speed (in cm/s) to check the actor velocity against.
	UPROPERTY(EditAnywhere, Category = "Conditional", meta = (DisplayName = "Distance Threshold"))
	float m_DistanceThreshold;

	/* If true, the Distance logic will only use the 2D (XY Plane) distance for calculations.*/
	UPROPERTY(EditInstanceOnly, Category = "Conditional", meta = (DisplayName="Use 2D Distance"))
	bool m_Use2DDistance;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Custom", ShortTooltip = "Calls the Ability's CheckCustomChannelConditional method and returns the result."))
class ABLECORESP_API UAbleChannelingCustomConditional : public UAbleChannelingBase
{
	GENERATED_BODY()
public:
	UAbleChannelingCustomConditional(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleChannelingCustomConditional();

#if WITH_EDITOR
    virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
protected:
	/* Override in your child class.*/
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override;

	// Optional Name to give this condition, it will be passed to the Ability when calling the CheckCustomChannelConditional method.
	UPROPERTY(EditAnywhere, Category = "Conditional", meta = (DisplayName = "Event Name"))
	FName m_EventName;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Block", ShortTooltip = "Only return failed result."))
class ABLECORESP_API UAbleChannelingBlockConditional : public UAbleChannelingBase
{
	GENERATED_BODY()
public:
	UAbleChannelingBlockConditional(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleChannelingBlockConditional();

#if WITH_EDITOR
	virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif
protected:
	/* Override in your child class.*/
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override;
};