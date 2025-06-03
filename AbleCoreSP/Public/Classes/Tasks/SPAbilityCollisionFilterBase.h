// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnLuaInterface.h"
#include "Tasks/ableCollisionFilters.h"
#include "SPAbilityCollisionFilterBase.generated.h"

/* Base class for all Collision Filters. */
UCLASS(Abstract, EditInlineNew, Blueprintable)
class ABLECORESP_API USPAbilityCollisionFilterBase : public UAbleCollisionFilter, public IUnLuaInterface
{
	GENERATED_BODY()
public:
	USPAbilityCollisionFilterBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USPAbilityCollisionFilterBase();

	/* Perform our filter logic. */
	virtual void Filter(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& InOutArray) const override;

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="Filter"))
	void FilterBP(const UAbleAbilityContext* Context, TArray<FAbleQueryResult>& InOutArray) const;

	virtual bool SkipObjectReferencer_Implementation() const override { return true; };
#if WITH_EDITOR
	/* Data Validation Tests. */
	virtual EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) { return EDataValidationResult::Valid; }
#endif
	
};