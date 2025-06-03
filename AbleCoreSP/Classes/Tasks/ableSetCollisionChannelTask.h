// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Engine/EngineTypes.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableSetCollisionChannelTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UPrimitiveComponent;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleSetCollisionChannelTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleSetCollisionChannelTaskScratchPad();
	virtual ~UAbleSetCollisionChannelTaskScratchPad();

	/* Map of Primitive Components to Collision Channels. */
	UPROPERTY(transient)
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionChannel>> PrimitiveToChannelMap;
};

UCLASS()
class UAbleSetCollisionChannelTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleSetCollisionChannelTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSetCollisionChannelTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleSetCollisionChannelCategory", "Collision"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleSetCollisionChannelTask", "Set Collision Channel"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleSetCollisionChannelTaskDesc", "Sets the Collision Channel on any targets, can optionally restore the previous channel as well."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(83.0f / 255.0f, 214.0f / 255.0f, 93.0f / 255.0f); }
#endif

protected:
	/* The collision channel to set our Task targets to. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Channel"))
	TEnumAsByte<ECollisionChannel> m_Channel;

	/* If true, the original channel values will be restored when the Task ends. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Restore Channel On End"))
	bool m_RestoreOnEnd;
};

#undef LOCTEXT_NAMESPACE