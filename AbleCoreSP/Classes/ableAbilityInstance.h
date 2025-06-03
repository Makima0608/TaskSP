// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityTypes.h"
#include "Tasks/IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableAbilityInstance.generated.h"

class UAbleAbility;
class UAbleAbilityComponent;

struct FSyncTaskAdditionInfo
{
	bool IsValidForNetwork = true;
};

/* This class stores/controls all the variables needed during the execution of an Ability. 
 * It's not networked since the Context is the publicly exposed class and any variables that need to be kept
 * in sync with the Server/Queried by the user should be done there. This class is meant to be Fire & Forget. */
UCLASS(Transient)
class ABLECORESP_API UAbleAbilityInstance : public UObject
{
	GENERATED_BODY()
public:
	UAbleAbilityInstance(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleAbilityInstance();

	/* Initializes this instance, allocates any scratch pads, sorts our tasks into Async/Sync arrays. */
	void Initialize(UAbleAbilityContext& AbilityContext);

	/* Called just before we begin processing an update. There is no PostUpdate since we have no idea when our Async tasks have finished. */
	bool PreUpdate();

	/* Returns our Ability's Name hash. */
	uint32 GetAbilityNameHash() const;

	void ResetTargetingForIteration(const bool Called = false) const;
	
	/* Resets the instance for the next run. */
	void ResetForNextIteration();

	/* True/False if we've finished this iteration (gone past Ability length, tasks done, etc).*/
	bool IsIterationDone() const;
	
	bool IsActiveSegmentDone() const;

	/* True/False if we've gone past our Ability Length. Any remaining tasks will be told to finish. */
	bool IsDone() const;

	/* True/False if our Ability is channeled or not. */
	bool IsChanneled() const;

    /* Checks if the ability should decay by a stack. */
    int CheckForDecay(UAbleAbilityComponent* AbilityComponent);
	
	/* Runs all the Ability Channel conditions and returns the result. */
	EAbleConditionResults CheckChannelConditions() const;

	/* Returns if the client needs to tell the server about this failure. */
	bool RequiresServerNotificationOfChannelFailure() const;

	/* Returns how the user wants us to handle a failed channel. */
	EAbleAbilityTaskResult GetChannelFailureResult() const;

	/* Resets our Time (optionally to the start of our loop range - if it exists). */
	void ResetTime(bool ToLoopStart = false);
	
	/* Synchronous update entry point. */
	void SyncUpdate(float DeltaTime);

	/* Sets this Ability's current stack count. */
	void SetStackCount(int32 TotalStacks);

	/* Returns this Ability's current stack count. */
	int32 GetStackCount() const;

	/* Adds Actors to be appended to the Target Actors in the Context before the next update. */
	void AddAdditionalTargets(const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates = false, bool ClearTargets = false);

	/* Queues up a modification to the Context. */
	void ModifyContext(AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets = false);

	// Ability State Modifiers.
	
	/* Stops the Ability and all currently running tasks are ended as successful before calling the Ability's OnAbilityFinished method. 
	 * No difference between Stop/FinishAbility yet. Stop is called via Blueprints or through the code API, while Finish is entirely internal. */
	void StopAbility();

	/* Stops the Ability and all currently running tasks are ended as successful before calling the Ability's OnAbilityFinished method. */
	void FinishAbility();

	/* Stops the Ability and all currently running tasks are ended as interrupted before calling the Ability's OnAbilityInterupt method. */
	void InterruptAbility();

	/* Stops the Ability and all currently running tasks are ended as branched before calling the Ability's OnAbilityBranch method. */
	void BranchAbility();

	void IterationAbility() const;

	bool BranchSegment(int SegmentIndex);

	// Accessors

	/* Returns a Mutable version of our Ability Context. */
	UAbleAbilityContext& GetMutableContext();

	/* Returns a Non-mutable version of our Ability Context. */
	const UAbleAbilityContext& GetContext() const;

	/* Returns our Ability. */
	const UAbleAbility& GetAbility() const;

	/* Returns the current time of this Context. */
	FORCEINLINE float GetCurrentTime() const { return m_Context->GetCurrentTime(); }

	/* Returns the current time ratio of this Context. */
	FORCEINLINE float GetCurrentTimeRatio() const { return m_Context->GetCurrentTimeRatio(); }

	/* Returns the play rate of the Ability. */
	float GetPlayRate() const;

	/* Sets the current time of the Ability. */
	void SetCurrentTime(const float NewTime, const bool FromPreview = false);
	
#if WITH_EDITOR
	void SetPreviewTime(float NewTime);
#endif
	
	int GetActiveSegmentIndex() const { return m_ActiveSegmentIndex; }
	void SetActiveSegmentIndex(int Index);

	/* Sets the iteration (loop) counter of the Ability. */
	void SetCurrentIteration(uint32 NewIteration);

	/* Reset our structure to it's default state. */
	void Reset();

	/* Check if we're valid or not. */
	bool IsValid() const { return m_Context != nullptr && m_Ability != nullptr; }

	void StopAcrossTask(const TArray<const UAbleAbilityTask*>& Tasks);

	bool CheckForceDependency();

	UFUNCTION(BlueprintCallable)
	bool IsPendingPassed() const { return m_PendingPassed; }
	
protected:
	/* Shared code used by both Update versions (Async/Sync). */
	void InternalUpdateTasks(TArray<UAbleAbilityTask*>& InTasks, TArray<const UAbleAbilityTask*>& InActiveTasks, TArray<const UAbleAbilityTask*>& InFinishedTasks, float CurrentTime, float DeltaTime);
	
	/* Safely calls the appropriate OnTaskEnd for all running tasks. */
	void InternalStopRunningTasks(EAbleAbilityTaskResult Reason, bool ResetForLoop = false);

	/* Safely calls the appropriate OnTaskEnd for all across tasks. */
	void InternalStopAcrossTasks(EAbleAbilityTaskResult Reason);
	
	/* Sets all our Tasks we are keeping track of as being non-executed. */
	void ResetTaskDependencyStatus();

	bool CanStartTask(const UAbleAbilityTask* Task) const;
	
    /* Our stack decay time, if any. */
    UPROPERTY(Transient)
    float m_DecayTime = 0.0f;

	/* Note we only store pointers to our tasks. The tasks themselves are stateless/purely functional (State is stored inside Scratch Pads as needed). */

	/* Array of Asynchronous Tasks that have been completed, used when looping. */
	//UPROPERTY(Transient)
	//TArray<const UAbleAbilityTask*> m_FinishedAyncTasks;
	
	/* Array of all Synchronous Tasks in this Ability. */
	UPROPERTY(Transient)
	TArray<UAbleAbilityTask*> m_SyncTasks;

	UPROPERTY(Transient)
	TArray<UAbleAbilityTask*> m_AcrossTasks;
	
	UPROPERTY(Transient)
    TArray<UAbleAbilityTask*> m_ActiveAcrossTasks;

	/* Array of any Synchronous Tasks currently being executed. */
	UPROPERTY(Transient)
	TArray<const UAbleAbilityTask*> m_ActiveSyncTasks;

	/* Array of Synchronous Tasks that have been completed, used when looping. */
	UPROPERTY(Transient)
	TArray<const UAbleAbilityTask*> m_FinishedSyncTasks;
	
	//some task will not execute,but need notify server, so we use empty tasks to calculate time, but no execute
	TArray<FSyncTaskAdditionInfo> m_SyncTaskAdditionInfo;

	/* The Ability. */
	UPROPERTY(Transient)
	const UAbleAbility* m_Ability = nullptr;

	/* The Ability Context. */
	UPROPERTY(Transient)
	UAbleAbilityContext* m_Context = nullptr;

	UPROPERTY(Transient)
	bool m_ClearTargets = false;

	/* Targets to be added to our Context at the start of the next frame. */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> m_AdditionalTargets;

	/* Tasks we need to keep track of, for dependency purposes. */
	UPROPERTY(Transient)
	TMap<const UAbleAbilityTask*, bool> m_TaskDependencyMap;

	UPROPERTY(Transient)
	TMap<const UAbleAbilityTask*, uint32> m_TaskIterationMap;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> m_RequestedInstigator;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> m_RequestedOwner;

	UPROPERTY(Transient)
	FVector m_RequestedTargetLocation = FVector::ZeroVector;

	/* Critical Section for AddAdditionalTargets. */
	FCriticalSection m_AddTargetCS;

	/* Critical Section for Task Dependency Map. */
	FCriticalSection m_DependencyMapCS;
	
	/* Note we only store pointers to our tasks. The tasks themselves are stateless/purely functional (State is stored inside Scratch Pads as needed). */
	int m_ActiveSegmentIndex = 0;

	UPROPERTY(Transient)
	TSet<const UAbleAbilityTask*> m_PreRemoveTask;

	bool m_PendingPassed = false;
};

template<>
struct TStructOpsTypeTraits< UAbleAbilityInstance > : public TStructOpsTypeTraitsBase2< UAbleAbilityInstance >
{
	enum
	{
		WithCopy = false
	};
};
