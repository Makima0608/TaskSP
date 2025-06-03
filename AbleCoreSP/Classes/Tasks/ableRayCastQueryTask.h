// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableCollisionFilters.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "WorldCollision.h"

#include "ableRayCastQueryTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleRayCastQueryTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleRayCastQueryTaskScratchPad();
	virtual ~UAbleRayCastQueryTaskScratchPad();

	/* Async Handle for our Query. */
	FTraceHandle AsyncHandle;

	/* Whether or not our Async query has been processed. */
	UPROPERTY(transient)
	bool AsyncProcessed;
};

UCLASS()
class ABLECORESP_API UAbleRayCastQueryTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleRayCastQueryTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleRayCastQueryTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* On Task Tick. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

	/* Returns true if our Tick method needs to be called. */
	FORCEINLINE bool NeedsTick() const override { return m_UseAsyncQuery; }

	/* Returns true if our Task is Async. */
	FORCEINLINE bool IsAsyncFriendly() const override { return m_UseAsyncQuery && !m_FireEvent; } 
	
	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns the realm our Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* The End time of our Task. */
	virtual float GetEndTime() const override { return GetStartTime() + 0.05f; }

	/* Returns true if our Task is done and ready for clean up. */
	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;
	
	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

    /* Returns the Copy Results to Context Parameter. */
    FORCEINLINE bool GetCopyResultsToContext() const { return m_CopyResultsToContext; }

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

	/* Query Filters */
	const TArray<UAbleCollisionFilter*> GetFilters() const { return m_Filters; }

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const { return LOCTEXT("AbleRayCastQueryTaskCategory", "Collision"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const { return LOCTEXT("AbleRayCastQueryTask", "Raycast Query"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const { return LOCTEXT("AbleRayCastQueryTaskDesc", "Performs a raycast query in the collision scene."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const { return FLinearColor(240.0f / 255.0f, 243.0f / 255.0f, 48.0f / 255.0f); }
	
	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_COLLISION_QUERY_RAYCAST; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const override { return EVisibility::Collapsed; }
	
	/* Returns true if the user is allowed to edit the realm this Task belongs to. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Returns the Fire Event Parameter. */
	FORCEINLINE bool GetFireEvent() const { return m_FireEvent; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Called by the Ability Editor to allow special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif

private:
	/* Helper method to copy the results of our query into the Ability Context. */
	void CopyResultsToContext(const TArray<FHitResult>& InResults, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

protected:
	/* The length of the Ray. */
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "Length", AbleBindableProperty))
	float m_Length;

	UPROPERTY()
	FGetAbleFloat m_LengthDelegate;

	/* If true, only return the blocking hit. Otherwise return all hits, including the blocking hit.*/
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "Only Return Blocking Hit", AbleBindableProperty))
	bool m_OnlyReturnBlockingHit;

	UPROPERTY()
	FGetAbleBool m_OnlyReturnBlockingHitDelegate;

	/* Whether or not to return the Face Index of the hit poly. */
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "Return Face Index", AbleBindableProperty))
	bool m_ReturnFaceIndex;

	UPROPERTY()
	FGetAbleBool m_ReturnFaceIndexDelegate;

	/* Whether or not to return the Physics Material for the hit poly. */
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "Return Physical Material", AbleBindableProperty))
	bool m_ReturnPhysicalMaterial;

	UPROPERTY()
	FGetAbleBool m_ReturnPhysicalMaterialDelegate;

	/* Filters to run on results from the Raycast. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Raycast|Filter", meta = (DisplayName = "Filters"))
	TArray<UAbleCollisionFilter*> m_Filters;

	/* Where to start the Raycast. */
	UPROPERTY(EditAnywhere, Category="Raycast", meta =(DisplayName = "Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_QueryLocation;

	UPROPERTY()
	FGetAbleTargetLocation m_QueryLocationDelegate;

	/* If true, the End Location value is used instead of the Length value to determine the end of the Raycast. */
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "Use End Location"))
	bool m_UseEndLocation;

	/* Where to end the Raycast can be used instead of using the Length Parameter. */
	UPROPERTY(EditAnywhere, Category = "Raycast", meta = (DisplayName = "End Location", AbleBindableProperty, EditCondition = m_UseEndLocation))
	FAbleAbilityTargetTypeLocation m_QueryEndLocation;

	UPROPERTY()
	FGetAbleTargetLocation m_QueryEndLocationDelegate;
	
	UPROPERTY(EditInstanceOnly, Category = "Raycast", meta = (DisplayName = "Collision Channel Present"))
	TEnumAsByte<EAbleChannelPresent> m_ChannelPresent;

    UPROPERTY(EditInstanceOnly, Category = "Raycast", meta = (DisplayName = "Collision Channels"))
    TArray<TEnumAsByte<ESPAbleTraceType>> m_CollisionChannels;

	/* If true, we'll fire the OnRaycastEvent in the Ability Blueprint. */
	UPROPERTY(EditAnywhere, Category = "Raycast|Event", meta = (DisplayName = "Fire Event"))
	bool m_FireEvent;

	/* An (optional) Name for this query. Useful if you have multiple Raycasts in a single ability. */
	UPROPERTY(EditAnywhere, Category = "Raycast|Event", meta = (DisplayName = "Name", EditCondition = m_FireEvent))
	FName m_Name;

	/* If true, the results of the query will be added to the Target Actor Array in the Ability Context. Note this takes 1 full frame to complete.*/
	UPROPERTY(EditAnywhere, Category = "Raycast|Misc", meta = (DisplayName = "Copy to Context"))
	bool m_CopyResultsToContext;

	/* If true, we won't check for already existing items when copying results to the context.*/
	UPROPERTY(EditAnywhere, Category = "Raycast|Misc", meta = (DisplayName = "Allow Duplicates", EditCondition = m_CopyResultsToContext))
	bool m_AllowDuplicateEntries;

	/* If true, we'll clear the Target Actor list before copying our context targets in. */
	UPROPERTY(EditAnywhere, Category = "Raycast|Misc", meta = (DisplayName = "Clear Existing Targets", EditCondition = m_CopyResultsToContext))
	bool m_ClearExistingTargets;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta =(DisplayName ="Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;

	/* If true, the query is placed in the Async queue and the results are retrieved next frame. */
	UPROPERTY(EditAnywhere, Category = "Optimization", meta = (DisplayName = "Use Async Query"))
	bool m_UseAsyncQuery;
	
};

#undef LOCTEXT_NAMESPACE