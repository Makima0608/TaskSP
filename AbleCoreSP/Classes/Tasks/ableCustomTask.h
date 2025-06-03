// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableCustomTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class ABLECORESP_API UAbleCustomTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleCustomTaskScratchPad();
	virtual ~UAbleCustomTaskScratchPad();
};


UCLASS(Abstract, EditInlineNew, hidecategories = ("Optimization"))
class ABLECORESP_API UAbleCustomTask : public UAbleAbilityTask
{
	GENERATED_BODY()
public:
	UAbleCustomTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCustomTask();

	/**
	* Callback that happens when this Task starts.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	/**
	* Callback that happens when this Task starts.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/**
	* Callback that happens every frame while this Task is executing.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param DeltaTime The time between frames.
	*
	* @return none
	*/
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	/**
	* Callback that happens every frame while this Task is executing.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param DeltaTime The time between frames.
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float DeltaTime) const;

	/**
	* Callback that happens when this Task has ended.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param result The Task Result for this Task (e.g., How it ended - Successful, Interrupted, Failed).
	*
	* @return none
	*/
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	/**
	* Callback that happens when this Task has ended.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param result The Task Result for this Task (e.g., How it ended - Successful, Interrupted, Failed).
	*
	* @return none
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/**
	* User provided logic that can be used to determine if this Task has finished or not.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if the task is done and should be ended, False otherwise.
	*/
	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	/**
	* User provided logic that can be used to determine if this Task has finished or not. 
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return True if the task is done and should be ended, False otherwise.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/**
	* Gets the Scratchpad Class of this Task.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The class to use for the Scratchpad of this Task (https://able.extralifestudios.com/wiki/index.php/Task_Scratchpad)
	*/
	virtual TSubclassOf<UAbleAbilityTaskScratchPad> GetTaskScratchPadClass(const UAbleAbilityContext* Context) const;

	/**
	* Gets the Scratchpad Class of this Task.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The class to use for the Scratchpad of this Task (https://able.extralifestudios.com/wiki/index.php/Task_Scratchpad)
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskScratchPadClass"))
	TSubclassOf<UAbleAbilityTaskScratchPad> GetTaskScratchPadClassBP(const UAbleAbilityContext* Context) const;

	/**
	* Creates the Scratchpad of this Task.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The Scratchpad of this Task (https://able.extralifestudios.com/wiki/index.php/Task_Scratchpad)
	*/
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const override;

	/**
	* Creates the Scratchpad of this Task.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The Scratchpad of this Task (https://able.extralifestudios.com/wiki/index.php/Task_Scratchpad)
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "CreateScratchPad", DeprecatedFunction, DeprecationMessage="CreateScratchPad is deprecated. Please use GetTaskScratchPadClass instead."))
	UAbleCustomTaskScratchPad* CreateScratchPadBP(UAbleAbilityContext* Context) const;


	/**
	* Resets the ScratchPad to it's initial state (if available). If you are re-using Scratchpads, this method is for you to do that re-initializing work.
	*
	* @param ScratchPad The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return None
	*/
	virtual void ResetScratchPad(UAbleAbilityTaskScratchPad* ScratchPad) const;


	/**
	* Resets the ScratchPad to it's initial state (if available). If you are re-using Scratchpads, this method is for you to do that re-initializing work.
	*
	* @param ScratchPad The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return None
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Able|Custom Task", meta = (DisplayName = "ResetScratchPad"))
	void ResetScratchPadBP(UAbleAbilityTaskScratchPad* ScratchPad) const;

	/**
	* Returns the Scratchpad of this Task.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	*
	* @return The Scratchpad of this Task (https://able.extralifestudios.com/wiki/index.php/Task_Scratchpad)
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetScratchPad"))
	UAbleCustomTaskScratchPad* GetScratchPad(UAbleAbilityContext* Context) const;

	/**
	* Returns whether or not to run this Task in Async.
	*
	* @return True if the Task should be executed Async, or false if otherwise.
	*/
	virtual bool IsAsyncFriendly() const override { return false; }

	/**
	* Returns whether or not to treat this Task as only running for one frame (end time is ignored).
	*
	* @return True if the Task should be Single frame, or false if otherwise.
	*/
	virtual bool IsSingleFrame() const override;

	/**
	* Returns whether or not to treat this Task as only running for one frame (end time is ignored).
	*
	* @return True if the Task should be Single frame, or false if otherwise.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/**
	* Returns the Ability Task Realm (Client / Server / Both)
	*
	* @return Ability Task Realm
	*/
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override;

	/**
	* Returns the Ability Task Realm (Client / Server / Both)
	*
	* @return Ability Task Realm
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Returns the category of this Task. */
	/**
	* Returns the category of this Task. Supports subcategories using "Cat|SubCat"
	*
	* @return The category of this Task.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskCategory"))
	FText GetTaskCategoryBP() const;

	/**
	* Returns the simple name of this Task.
	*
	* @return The name of this Task.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskName"))
	FText GetTaskNameBP() const;

	/**
	* Returns dynamic, descriptive name of this Task.
	* This can contain info about the asset you referencing,
	* or any other specific information. It's shown in the Timeline of the Ability Editor.
	*
	* @return The descriptive name of this Task.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetDescriptiveTaskName"))
	FText GetDescriptiveTaskNameBP() const;

	/**
	* Returns the description of this Task.
	*
	* @return The description of this Task.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskDescription"))
	FText GetTaskDescriptionBP() const;

	/**
	* Returns the color of this Task.
	*
	* @return The color of this Task.
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskColor"))
	FLinearColor GetTaskColorBP() const;

	/**
	* Returns the start time of this Task.
	*
	* @return The start time of this Task.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskStartTime"))
	float GetTaskStartTimeBP() const { return GetStartTime(); }

	/**
	* Returns the end time of this Task.
	*
	* @return The end time of this Task.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskEndTime"))
	float GetTaskEndTimeBP() const { return GetEndTime(); }

	/** 
	* Returns the length (end - start) of this Task. 
	*
	* @return The length of this Task. 
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskLength"))
	float GetTaskLengthBP() const { return GetEndTime() - GetStartTime(); }

#if WITH_EDITOR
	/* Returns the category of this Task. */
	virtual FText GetTaskCategory() const override;

	/* Returns the name of this Task. */
	virtual FText GetTaskName() const override;

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of this Task. */
	virtual FText GetTaskDescription() const override;

	/* Returns the color of this Task. */
	virtual FLinearColor GetTaskColor() const override;

	/* Returns the estimated runtime cost of this Task. */
	virtual float GetEstimatedTaskCost() const override { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_BP_EVENT; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }

	/* Returns true if the user is allowed to edit the Tasks realm. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Data Validation Tests. */
    EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors);
#endif

#if WITH_EDITORONLY_DATA
	virtual bool HasCompactData() const override { return true; }
#endif

};

#undef LOCTEXT_NAMESPACE