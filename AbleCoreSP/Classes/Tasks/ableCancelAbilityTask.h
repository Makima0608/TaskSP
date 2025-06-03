// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableAbility.h"
#include "IAbleAbilityTask.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"

#include "ableCancelAbilityTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UENUM()
enum EAbleCancelAbilityPassiveBehavior
{
	RemoveOneStack UMETA(DisplayName = "Remove One"),
	RemoveOneStackWithRefresh UMETA(DisplayName = "Remove One and Refresh Duration"),
	RemoveEntireStack UMETA(DisplayName = "Clear Stack"),
};

UCLASS(EditInlineNew)
class UAbleCancelAbilityTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleCancelAbilityTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCancelAbilityTask();

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
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleCancelAbilityCategory", "Gameplay"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleCancelAbilityTask", "Cancel Ability"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleCancelAbilityTaskDesc", "Cancels Abilities on the Targets that match the provided critera."); }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(230.0f / 255.0f, 14.0f / 255.0f, 14.0f / 255.0f); }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	// Simple helper method.
	bool ShouldCancelAbility(const UAbleAbility& Ability, const UAbleAbilityContext& Context) const;

	/* The Ability to Cancel. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Ability", AbleBindableProperty))
	TSubclassOf<UAbleAbility> m_Ability;

	UPROPERTY()
	FGetAbleAbility m_AbilityDelegate;

	/* A Tag query to run on targets. All Abilities that match the query will be cancelled. */
	UPROPERTY(EditAnywhere, Category = "Ability|Tags", meta = (DisplayName = "Tag Query"))
	FGameplayTagQuery m_TagQuery;

	/* How to handle Passives Abilities that match the cancel criteria.*/
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Passive Behavior"))
	TEnumAsByte<EAbleCancelAbilityPassiveBehavior> m_PassiveBehavior;

	/* What result to pass Abilities that are canceled */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (DisplayName = "Cancel Result"))
	TEnumAsByte<EAbleAbilityTaskResult> m_CancelResult;

	/* A String identifier you can use to identify this specific task in the ability blueprint, when ShouldCancelAbility is called. */
	UPROPERTY(EditAnywhere, Category = "Ability|Dynamic", meta = (DisplayName = "Event Name"))
	FName m_EventName;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE
