// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableModifyContextTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UCLASS()
class ABLECORESP_API UAbleModifyContextTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleModifyContextTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleModifyContextTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleModifyContextCategory", "Context"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleModifyContextTask", "Modify Context"); }

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleModifyContextTaskDesc", "Allows the user to modify the Ability Context at runtime. This can cause a desync in a network environment, so be careful. Takes one frame to complete."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(195.0f / 255.0f, 50.0f / 255.0f, 105.0f / 255.0f); }
#endif

protected:
	/* Target Location to set within the Context.  */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Target Location", AbleBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAbleVector m_TargetLocationDelegate;

	/* If true, we'll call the Ability's FindCustomTargets method and add those to the Target Actors in the Context. */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Additional Targets", AbleBindableProperty))
	bool m_AdditionalTargets;

	UPROPERTY()
	FGetAbleBool m_AdditionalTargetsDelegate;

	/* If true, we'll clear the current Target Actors first. */
	UPROPERTY(EditAnywhere, Category = "Context", meta = (DisplayName = "Clear Current Targets", AbleBindableProperty))
	bool m_ClearCurrentTargets;

	UPROPERTY()
	FGetAbleBool m_ClearCurrentTargetsDelegate;
};

#undef LOCTEXT_NAMESPACE