// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableCheckConditionTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UAbleBranchCondition;
class UInputSettings;

UCLASS(Transient)
class UAbleCheckConditionTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleCheckConditionTaskScratchPad();
	virtual ~UAbleCheckConditionTaskScratchPad();

    /* Cached for the Custom Branch Conditional*/
    UPROPERTY(transient)
    bool ConditionMet;
};

UCLASS(EditInlineNew, hidecategories = ("Targets"))
class UAbleCheckConditionTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleCheckConditionTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCheckConditionTask();

    /* Called to determine if a Task can end. Default behavior is to see if our context time is > than our end time. */
    virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
    bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* On Task Tick*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

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

#if WITH_EDITOR
	/* Returns the category for this Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleCheckConditionCategory", "Logic"); }
	
	/* Returns the name of the Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleCheckConditionTask", "Check Condition"); }
	
	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of the Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleCheckConditionTaskDesc", "Polls the ability blueprint for a condition."); }
	
	/* Returns what color to use for this Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0f, 0.0f, 1.0f); } //Purple
	
	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_BP_EVENT; } // Assume worst case and they are using a BP Custom condition.
#endif
protected:
	/* Helper method to check our conditions. */
	bool CheckCondition(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const UAbleCheckConditionTaskScratchPad& ScratchPad) const;

    // If using Dynamic Branch Ability, this name will be passed along when calling the function (optional).
    UPROPERTY(EditAnywhere, Category = "Branch", meta = (DisplayName = "Custom Event Name"))
    FName m_ConditionEventName;
};

#undef LOCTEXT_NAMESPACE