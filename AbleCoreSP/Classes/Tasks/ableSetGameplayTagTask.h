// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableSetGameplayTagTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleSetGameplayTagTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleSetGameplayTagTaskScratchPad();
	virtual ~UAbleSetGameplayTagTaskScratchPad();

	/* Actors we tagged. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<AActor>> TaggedActors;
};

UCLASS()
class ABLECORESP_API UAbleSetGameplayTagTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleSetGameplayTagTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSetGameplayTagTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns the realm our Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler stat ID of our Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleSetGameplayTaskCategory", "Tags"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleSetGameplayTagTask", "Set Gameplay Tag"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleSetGameplayTagDesc", "Sets the supplied Gameplay Tags on the task targets."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(149.0f / 255.0f, 243.0f / 255.0f, 195.0f / 255.0f); }

	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const override { return ABLETASK_EST_SYNC; }

	/* Returns true if the user is allowed to edit the realm this Task belongs to. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* Tags to Set. */
	UPROPERTY(EditAnywhere, Category="Tags", meta=(DisplayName="Tag List"))
	TArray<FGameplayTag> m_TagList;

	/* If true, all the tags set by this Task will be removed once the task completes. */
	UPROPERTY(EditAnywhere, Category = "Tags", meta = (DisplayName = "Remove On End"))
	bool m_RemoveOnEnd;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE