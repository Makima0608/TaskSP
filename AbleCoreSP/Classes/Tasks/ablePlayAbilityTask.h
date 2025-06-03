// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableAbility.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePlayAbilityTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UCLASS(EditInlineNew)
class UAblePlayAbilityTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAblePlayAbilityTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePlayAbilityTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns the realm our Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAbleAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePlayAbilityCategory", "Gameplay"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePlayAbilityTask", "Play Ability"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePlayAbilityTaskDesc", "Creates an Ability Context and calls Activate Ability on the Target's Ability Component. This will cause an interrupt if valid."); }
	
	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(166.0f / 255.0f, 77.0f / 255.0f, 121.0f / 255.0f); } 

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* The Ability to Play. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Ability", AbleBindableProperty))
	TSubclassOf<UAbleAbility> m_Ability;

	UPROPERTY()
	FGetAbleAbility m_AbilityDelegate;

	/* Who to set as the "Source" of this damage. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Owner", AbleBindableProperty))
	TEnumAsByte<EAbleAbilityTargetType> m_Owner;

	UPROPERTY()
	FGetAbleTargetType m_OwnerDelegate;

	/* Who to set as the "Source" of this damage. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Instigator", AbleBindableProperty))
	TEnumAsByte<EAbleAbilityTargetType> m_Instigator;

	UPROPERTY()
	FGetAbleTargetType m_InstigatorDelegate;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;

	// If true, you're existing targets will be carried over to the new Ability.
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Copy Targets", AbleBindableProperty))
	bool m_CopyTargets;

	UPROPERTY()
	FGetAbleBool m_CopyTargetsDelegate;

};

#undef LOCTEXT_NAMESPACE
