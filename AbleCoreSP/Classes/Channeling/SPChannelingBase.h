// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityTypes.h"
#include "ableAbilityContext.h"
#include "ableChannelingBase.h"
#include "UnLuaInterface.h"
#include "UObject/ObjectMacros.h"
#include "SPChannelingBase.generated.h"

UCLASS(Abstract, EditInlineNew, Blueprintable)
class ABLECORESP_API USPChannelingBase : public UAbleChannelingBase, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	USPChannelingBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USPChannelingBase();

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override
	{
	};

	virtual bool RequiresServerNotificationOfFailure() const override
	{
		return RequiresServerNotificationOfFailureBP();
	}

	UFUNCTION(BlueprintNativeEvent)
	bool RequiresServerNotificationOfFailureBP() const;

	virtual bool SkipObjectReferencer_Implementation() const override { return true; };
protected:
	virtual EAbleConditionResults CheckConditional(UAbleAbilityContext& Context) const override
	{
		return CheckConditionalBP(&Context);
	}

	UFUNCTION(BlueprintNativeEvent)
	EAbleConditionResults CheckConditionalBP(UAbleAbilityContext* Context) const;
};
