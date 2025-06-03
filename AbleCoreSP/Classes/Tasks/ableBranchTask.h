// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once
#include "UnLuaInterface.h"
#include "ableAbility.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableBranchTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UAbleBranchCondition;
class UInputSettings;

UCLASS(Transient)
class UAbleBranchTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleBranchTaskScratchPad();
	virtual ~UAbleBranchTaskScratchPad();

	/* Cached for the Custom Branch Conditional*/
	UPROPERTY(transient)
	const UAbleAbility* BranchAbility;

	/* Keys to check for the Input Conditional */
	UPROPERTY(transient)
	TArray<struct FKey> CachedKeys;

    UPROPERTY(transient)
    bool BranchConditionsMet;
};

UCLASS(EditInlineNew, hidecategories = ("Targets"))
class ABLECORESP_API UAbleBranchTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleBranchTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleBranchTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* On Task Tick*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

    /* On Task End*/
    virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
    void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task is Async supported. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns true if this task needs its Tick method called. */
	virtual bool NeedsTick() const override { return true; }

	/* Returns the Realm to execute this task in. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Create the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Return the Profiler Stat Id for this Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

	/* Return this Branch Tasks' conditions. */
	const TArray<UAbleBranchCondition*>& GetBranchConditions() const { return m_BranchConditions; }

#if WITH_EDITOR
	/* Returns the category for this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleBranchCategory", "Logic"); }
	
	/* Returns the name of the Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleBranchTask", "Branch"); }
	
	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of the Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleBranchTaskDesc", "Allows this ability to immediately branch into another ability if the branch condition passes."); }

	/* Returns the Rich Text description, with mark ups. */
	virtual FText GetRichTextTaskSummary() const override;

	/* Returns what color to use for this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(79.0f / 255.0f, 159.0f / 255.0f, 206.0f / 255.0f); } 
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_BP_EVENT; } // Assume worst case and they are using a BP Custom condition.

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
#endif
protected:
	/* Helper method to check our conditions. */
	bool CheckBranchCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	/* The Ability to Branch to. */
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Ability", AbleBindableProperty, AbleDefaultBinding = "OnGetBranchAbilityBP"))
	TSubclassOf<UAbleAbility> m_BranchAbility;

	UPROPERTY()
	FGetAbleAbility m_BranchAbilityDelegate;

	/* The Conditions for the Ability to Branch. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Branch", meta = (DisplayName = "Conditions"))
	TArray<UAbleBranchCondition*> m_BranchConditions;

	/* The Conditions for the Ability to Branch. */
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Must Pass All Conditions", AbleBindableProperty))
	bool m_MustPassAllBranchConditions;

	UPROPERTY()
	FGetAbleBool m_MustPassAllBranchConditionsDelegate;

	// If true, you're existing targets will be carried over to the branched Ability.
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Copy Targets on Branch", AbleBindableProperty))
	bool m_CopyTargetsOnBranch;

	UPROPERTY()
	FGetAbleBool m_CopyTargetsOnBranchDelegate;

	// If true, the branch conditions will be checked during the task window, but the actual branch itself will activate at the end of the task window.
	UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Branch on Task End", AbleBindableProperty))
	bool m_BranchOnTaskEnd;

	UPROPERTY()
	FGetAbleBoolWithResult m_BranchOnTaskEndDelegate;

private:
	/* Helper method to consolidate logic. */
	void InternalDoBranch(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
};

#undef LOCTEXT_NAMESPACE