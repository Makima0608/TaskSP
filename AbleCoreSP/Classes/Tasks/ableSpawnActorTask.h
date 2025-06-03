// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableAbility.h"
#include "Engine/EngineTypes.h"
#include "Targeting/ableTargetingBase.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableSpawnActorTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for this task. */
UCLASS(Transient)
class UAbleSpawnActorTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleSpawnActorTaskScratchPad();
	virtual ~UAbleSpawnActorTaskScratchPad();

	/* The Actor(s) we've spawned. */
	UPROPERTY()
	TArray<AActor*> SpawnedActors;
};

UCLASS()
class ABLECORESP_API UAbleSpawnActorTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleSpawnActorTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSpawnActorTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns the realm our Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAbleAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleSpawnActorTaskCategory", "Spawn"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleSpawnActorTask", "Spawn Actor"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleSpawnActorTaskDesc", "Spawns an actor at the given location."); }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(0.0f, 0.0f, 220.0f / 255.0f); }
	
	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const override { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_SPAWN_ACTOR; }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* The class of the actor we want to dynamically spawn. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Actor Class", AbleBindableProperty))
	TSubclassOf<AActor> m_ActorClass;

	UPROPERTY()
	FGetAbleActor m_ActorClassDelegate;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Amount to Spawn", AbleBindableProperty, UIMin=1, ClampMin=1))
	int m_AmountToSpawn;

	UPROPERTY()
	FGetAbleInt m_AmountToSpawnDelegate;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Spawn Collision"))
	ESpawnActorCollisionHandlingMethod m_SpawnCollision;

	/* The socket within our mesh component to attach the actor to, or use as an initial transform when we spawn the Actor */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_SpawnLocation;

	UPROPERTY()
	FGetAbleTargetLocation m_SpawnLocationDelegate;

	/* The initial linear velocity for the spawned object. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Initial Velocity"))
	FVector m_InitialVelocity;

	/* If true, Set the owner of the new actor. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Set Owner"))
	bool m_SetOwner;

	/* The parent of the Actor. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Owner", EditCondition = "m_SetOwner"))
	TEnumAsByte<EAbleAbilityTargetType> m_OwnerTargetType;

	/* If true, the object will be attached to the socket. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Attach To Owner Socket", EditCondition = "m_SetOwner"))
	bool m_AttachToOwnerSocket;

	/* The Attachment Rule to use if attached to the owner socket. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Attach To Owner Socket", EditCondition = "m_AttachToOwnerSocket"))
	EAttachmentRule m_AttachmentRule;

	/* The socket within our mesh component to attach the actor to, or use as an initial transform when we spawn the Actor */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Socket", EditCondition = "m_SetOwner && m_AttachToOwnerSocket"))
	FName m_SocketName;

	/* If true, spawn with our parent's linear velocity. This is added before the initial velocity. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Inherit Velocity", EditCondition = "m_SetOwner"))
	bool m_InheritOwnerLinearVelocity;

	/* If true, marks the Actor as transient, so it won't be saved between game sessions. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Transient"))
	bool m_MarkAsTransient;

	/* Whether or not we destroy the actor at the end of the task. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Destroy On End"))
	bool m_DestroyAtEnd;

	/* If true, we'll call the OnSpawnedActorEvent in the Ability Blueprint. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Event", meta = (DisplayName = "Fire Event"))
	bool m_FireEvent;

	/* A String identifier you can use to identify this specific task in the Ability blueprint. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Event", meta = (DisplayName = "Name", EditCondition = m_FireEvent))
	FName m_Name;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;

};

#undef LOCTEXT_NAMESPACE
