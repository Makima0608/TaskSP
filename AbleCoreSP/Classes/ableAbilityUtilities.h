// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityInstance.h"
#include "ableAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/EngineTypes.h"

class FAbleAbilityUtilities
{
public:
	static const TArray<struct FKey> GetKeysForInputAction(const FName& InputAction);
	static UBlackboardComponent* GetBlackboard(AActor* Target);
};

// Various one off helper classes.
// These are mostly Predicates, or Async Tasks structs.
class FAsyncAbilityInstanceUpdaterTask
{
public:
	// We make a copy of the Current time since it is written to during the sync update.
	FAsyncAbilityInstanceUpdaterTask(UAbleAbilityInstance* InAbilityInstance, float InCurrentTime, float InDeltaTime)
		: m_AbilityInstance(InAbilityInstance),
		m_CurrentTime(InCurrentTime),
		m_DeltaTime(InDeltaTime)
	{ }

	/* Returns the mode of this Task. */
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::FireAndForget; }
	
	/* Returns the desired thread of this Task. */
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }

	/* Returns the Profiler Stat ID of this Task. */
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncAbilityInstanceUpdaterTask, STATGROUP_TaskGraphTasks);
	}

private:
	/* Ability Instance to Update. */
	UAbleAbilityInstance* m_AbilityInstance;
	
	/* Current Ability Time. */
	float m_CurrentTime;

	/* Delta Time*/
	float m_DeltaTime;
};

class FAsyncAbilityCooldownUpdaterTask
{
public:
	// We make a copy of the Current time since it is written to during the sync update.
	FAsyncAbilityCooldownUpdaterTask(UAbleAbilityComponent* InComponent, float InDeltaTime)
		: m_AbilityComponent(InComponent),
		m_DeltaTime(InDeltaTime)
	{ }

	/* Update all Cooldowns. */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (m_AbilityComponent.IsValid())
		{
			m_AbilityComponent->UpdateCooldowns(m_DeltaTime);
		}
	}

	/* Returns the mode of this Task. */
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::FireAndForget; }
	
	/* Returns the desired thread of this Task. */
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
	
	/* Returns the Profiler Stat ID for this Task. */
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncAbilityInstanceUpdaterTask, STATGROUP_TaskGraphTasks);
	}

private:
	/* The Ability Component to update Cooldowns on. */
	TWeakObjectPtr<UAbleAbilityComponent> m_AbilityComponent;

	/* The Delta Time to apply. */
	float m_DeltaTime;
};

struct FAbleFindAbilityInstanceByHash
{
	FAbleFindAbilityInstanceByHash(uint32 InAbilityHash)
		: AbilityHash(InAbilityHash)
	{

	}

	/* The Hash to find. */
	uint32 AbilityHash;

	bool operator()(const TWeakObjectPtr<UAbleAbilityInstance>& A) const
	{
		check(A.IsValid());
		return A->GetAbility().GetAbilityNameHash() == AbilityHash;
	}
};

struct FAbleFindAbilityNetworkContextByHash
{
	FAbleFindAbilityNetworkContextByHash(uint32 InAbilityHash)
		: AbilityHash(InAbilityHash)
	{

	}

	/* The Hash to find. */
	uint32 AbilityHash;

	bool operator()(const FAbleAbilityNetworkContext& A) const
	{
		return A.GetAbility()->GetAbilityNameHash() == AbilityHash;
	}
};

struct FAbleAbilityInstanceWhiteList
{
	FAbleAbilityInstanceWhiteList(const TArray<uint32>& InAbilityHashWhiteList, bool InCallFinish = true)
		: AbilityHashWhiteList(InAbilityHashWhiteList), CallFinish(InCallFinish)
	{

	}

	/* The Hashs of all Abilities on our Whitelist. */
	TArray<uint32> AbilityHashWhiteList;

	/* True if Finish Ability should be called on all entities not in the whitelist before removing them.*/
	bool CallFinish;

	bool operator()(const TWeakObjectPtr<UAbleAbilityInstance>& A) const
	{
		check(A.IsValid());
		bool Remove = !AbilityHashWhiteList.Contains(A->GetAbility().GetAbilityNameHash());
		if (Remove && CallFinish)
		{
			A->FinishAbility();
		}
		return Remove;
	}
};

struct FAbleAbilityNetworkContextWhiteList
{
	FAbleAbilityNetworkContextWhiteList(const TArray<uint32>& InAbilityHashWhiteList)
		: AbilityHashWhiteList(InAbilityHashWhiteList)
	{

	}

	/* Ability Hash Whitelist*/
	TArray<uint32> AbilityHashWhiteList;

	bool operator()(const FAbleAbilityNetworkContext& A) const
	{
		return !AbilityHashWhiteList.Contains(A.GetAbility()->GetAbilityNameHash());
	}
};

struct FAbleAbilityResultSortByDistance
{
	FAbleAbilityResultSortByDistance(const FVector& InSourceLocation, bool InUse2DDistance, bool InSortAscending)
		: SourceLocation(InSourceLocation),
		Use2DDistance(InUse2DDistance),
		SortAscending(InSortAscending)
	{

	}

	/* The Location to use in our Sort comparision. */
	FVector SourceLocation;

	/* True if we should use 2D Distance instead of 3D distance. */
	bool Use2DDistance;

	/* Whether to sort the results in ascending or descending. */
	bool SortAscending;

	bool operator()(const FAbleQueryResult& A, const FAbleQueryResult& B) const
	{
		float DistanceA = Use2DDistance ? FVector::DistSquaredXY(SourceLocation, A.GetLocation()) : FVector::DistSquared(SourceLocation, A.GetLocation());
		float DistanceB = Use2DDistance ? FVector::DistSquaredXY(SourceLocation, B.GetLocation()) : FVector::DistSquared(SourceLocation, B.GetLocation());

		return SortAscending ? DistanceA < DistanceB : DistanceA > DistanceB;
	}
};

struct FAbleSPLogHelper
{
	/* Returns the provided Result as a human readable string. */
	ABLECORESP_API static const FString GetResultEnumAsString(EAbleAbilityStartResult Result);
	ABLECORESP_API static const FString GetTaskResultEnumAsString(EAbleAbilityTaskResult Result);
	ABLECORESP_API static const FString GetTargetTargetEnumAsString(EAbleAbilityTargetType Result);
	ABLECORESP_API static const FString GetCollisionResponseEnumAsString(ECollisionResponse Response);
	ABLECORESP_API static const FString GetCollisionChannelEnumAsString(ECollisionChannel Channel);
    ABLECORESP_API static const FString GetWorldName(const UWorld* World);
};