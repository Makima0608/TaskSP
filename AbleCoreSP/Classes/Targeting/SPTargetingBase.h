// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnLuaInterface.h"
#include "Targeting/ableTargetingBase.h"
#include "UObject/ObjectMacros.h"

#include "SPTargetingBase.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class USPTargetingBase : public UAbleTargetingBase, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	USPTargetingBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USPTargetingBase();

	virtual void FindTargets(UAbleAbilityContext& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="FindTargets"))
	void FindTargetsBP(UAbleAbilityContext* Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

	virtual bool SkipObjectReferencer_Implementation() const override { return true; };

#if WITH_EDITOR
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="OnAbilityEditorTick"))
	void OnAbilityEditorTickBP(const UAbleAbilityContext* Context, float DeltaTime) const;

protected:
	virtual float CalculateRange() const override;
	
	void ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const;

	UFUNCTION(BlueprintCallable)
	void ProcessActorResults(UAbleAbilityContext* Context, const TArray<AActor*>& Results) const;
};

#undef LOCTEXT_NAMESPACE
