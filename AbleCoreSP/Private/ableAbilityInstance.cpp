// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityInstance.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableAbilityUtilities.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/ScopeLock.h"

struct FAbleAbilityTaskIsDonePredicate
{
	FAbleAbilityTaskIsDonePredicate(float InCurrentTime)
		: CurrentTime(InCurrentTime)
	{

	}

	float CurrentTime;

	bool operator()(const UAbleAbilityTask* A) const
	{
		return CurrentTime >= A->GetEndTime();
	}
};

UAbleAbilityInstance::UAbleAbilityInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

UAbleAbilityInstance::~UAbleAbilityInstance()
{
}

void UAbleAbilityInstance::Initialize(UAbleAbilityContext& AbilityContext)
{
	// AbilityContext.SetLoopIteration(0);
	m_Ability = AbilityContext.GetAbility();
	m_Ability->PreExecutionInit();
	m_Context = &AbilityContext;

	ENetMode NetMode = NM_Standalone;
	if (AbilityContext.GetSelfActor())
	{
		NetMode = AbilityContext.GetSelfActor()->GetNetMode();

        AbilityContext.SetAbilityActorStartLocation(AbilityContext.GetSelfActor()->GetActorLocation());
	}

	// Sort our Tasks into Sync/Async queues.
	/*const TArray<UAbleAbilityTask*> & Tasks = m_Ability->GetTasks();
	for (UAbleAbilityTask* Task : Tasks)
	{
        if (!Task || Task->IsDisabled())
        {
            continue;
        }

		if (Task->IsValidForNetMode(NetMode))
		{
			if (Task->IsAsyncFriendly())
			{
				m_AsyncTasks.Add(Task);
			}
			else
			{
				m_SyncTasks.Add(Task);
			}
		}
	}

	m_FinishedAyncTasks.Reserve(m_AsyncTasks.Num());
	m_FinishedSyncTasks.Reserve(m_SyncTasks.Num());*/

	// Populate our map of Tasks we need to keep track of due to other Tasks being dependent on them.
	/*const TArray<const UAbleAbilityTask*>& TaskDependencies = m_Ability->GetAllTaskDependencies();
	for (const UAbleAbilityTask* TaskDependency : TaskDependencies)
	{
        if (TaskDependency == nullptr)
            continue;

		if (!m_TaskDependencyMap.Contains(TaskDependency))
		{
			m_TaskDependencyMap.Add(TaskDependency, false);
		}
	}*/
	
	// Set our initial stacks.
	SetStackCount(FMath::Max(m_Ability->GetInitialStacks(m_Context), 0));

	// Tell our C++ Delegate
	if (m_Context->GetSelfAbilityComponent())
	{
		m_Context->GetSelfAbilityComponent()->GetOnAbilityStart().Broadcast(*m_Context);
	}
	
	m_ActiveSegmentIndex = -1;

	uint8 StartIndex = AbilityContext.IsActiveSegmentValid() ? AbilityContext.GetActiveSegmentIndex() : m_Ability->GetEntryIndex();
	if (StartIndex >= m_Ability->GetSegmentNumber()) StartIndex = m_Ability->GetEntryIndex();
	SetActiveSegmentIndex(StartIndex);
	SetCurrentTime(m_Context->GetCurrentTime());
	AbilityContext.SetSegmentLoopIteration(0, StartIndex);

	// Call our OnAbilityStart
	m_Ability->OnAbilityStartBP(m_Context);
	m_PendingPassed = false;
}

bool UAbleAbilityInstance::PreUpdate()
{
	// Copy over any targets that should be added to our context.
	check(m_Context != nullptr);

	if (m_ClearTargets)
	{
		m_Context->GetMutableTargetActors().Empty();
		m_ClearTargets = false;
	}

	if (m_AdditionalTargets.Num())
	{
		m_Context->GetMutableTargetActors().Append(m_AdditionalTargets);
		m_AdditionalTargets.Empty();
	}

	if (m_RequestedInstigator.IsValid())
	{
		m_Context->SetInstigator(m_RequestedInstigator.Get());
		m_RequestedInstigator.Reset();
	}

	if (m_RequestedOwner.IsValid())
	{
		m_Context->SetOwner(m_RequestedOwner.Get());
		m_RequestedOwner.Reset();
	}

	if (m_RequestedTargetLocation.SizeSquared() > 0.0f)
	{
		m_Context->SetTargetLocation(m_RequestedTargetLocation);
		m_RequestedTargetLocation = FVector::ZeroVector;
	}

	if (m_Context->GetSelfAbilityComponent() && !m_Context->GetSelfAbilityComponent()->IsAuthoritative() && 
	    m_Context->GetSelfAbilityComponent()->IsOwnerLocallyControlled())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(m_Context->GetSelfActor()) )
		{
			UCharacterMovementComponent* OwnerCharacterMovement = Cast<UCharacterMovementComponent>(OwnerCharacter->GetMovementComponent());
			if (OwnerCharacterMovement)
			{
				OwnerCharacterMovement->FlushServerMoves();
			}
		}
	}

	return true;
}

uint32 UAbleAbilityInstance::GetAbilityNameHash() const
{
	check(m_Ability != nullptr);
	return m_Ability->GetAbilityNameHash();
}

void UAbleAbilityInstance::ResetTargetingForIteration(const bool Called) const
{
	if (m_Context && m_Context->GetAbility() != nullptr)
	{
		const UAbleTargetingBase* m_Targeting = m_Context->GetAbility()->GetTargeting();
		if (m_Targeting && (m_Targeting->IsResetForIteration() || Called))
		{
			UE_LOG(LogAbleSP, Log, TEXT("[SPAbility] ability [%d] segment [%d] ResetTargetingForIteration"), m_Context->GetAbilityId(), GetActiveSegmentIndex())
			m_Context->ClearTargetActors();
			m_Targeting->FindTargets(*m_Context);
		}
	}
}

void UAbleAbilityInstance::ResetForNextIteration()
{
	ResetTargetingForIteration();
	
	for (int32 i = 0; i < m_ActiveSyncTasks.Num(); )
	{
		if (m_ActiveSyncTasks[i]->GetResetForIterations())
		{
			m_ActiveSyncTasks[i]->OnTaskEnd(m_Context, EAbleAbilityTaskResult::Successful);
			UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::ResetForNextIteration Index = [%d] TaskName [%s]"), i, *m_ActiveSyncTasks[i]->GetName());
			m_ActiveSyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	// m_Context->SetLoopIteration(m_Context->GetCurrentLoopIteration() + 1);
	m_Context->SetSegmentLoopIteration(m_Context->GetCurrentSegmentLoopIteration() + 1, m_ActiveSegmentIndex);
	if (m_Ability->IsSegmentLooping(m_ActiveSegmentIndex))
	{
		// This is the start, end time of the Loop. X = Start, Y = End
		const FVector2D& LoopRange = m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex);
		if (m_Context->GetCurrentTime() > LoopRange.Y)
		{
			// Reset our time to the start of the loop range, and we'll keep going.
			// m_Context->SetCurrentTime(LoopRange.X);
			// ecoliwu: modify the loop iteration start time
			m_Context->SetCurrentTime(LoopRange.X > m_Context->GetCurrentTime() - LoopRange.Y ? LoopRange.X : m_Context->GetCurrentTime() - LoopRange.Y);

			m_FinishedSyncTasks.RemoveAll([&](const UAbleAbilityTask* Task)
			{
				return Task->GetStartTime() >= LoopRange.X && Task->GetEndTime() <= LoopRange.Y;
			});

			// Sort our Tasks into Sync/Async queues (again, people apparently want to dynamically turn on/off tasks).
			m_SyncTasks.Empty(m_SyncTasks.Num());

			const TArray<UAbleAbilityTask*>& Tasks = m_Ability->GetTasks(m_ActiveSegmentIndex);
			for (UAbleAbilityTask* Task : Tasks)
			{
				if (!Task || Task->IsDisabled())
				{
					continue;
				}

				ENetMode NetMode = NM_Standalone;
				if (m_Context->GetSelfActor())
				{
					NetMode = m_Context->GetSelfActor()->GetNetMode();
				}

				if (Task->IsValidForNetMode(NetMode, m_Context->GetSelfActor(), m_Context))
				{
					if (!m_ActiveSyncTasks.Contains(Task))
					{
						m_SyncTasks.Add(Task);
					}
				}
			}
		}
	}
	else
	{
		m_Context->SetCurrentTime(0.0f);
	}

	if (GetStackCount() != 0
		&& m_Ability->GetDecrementAndRestartOnEnd())
	{
		int32 stackDecrement = m_Ability->GetStackDecrement(m_Context);
		int32 newStackValue = FMath::Max(GetStackCount() - stackDecrement, 0);

		m_Context->GetSelfAbilityComponent()->SetPassiveStackCount(m_Context->GetAbility(), newStackValue, true, EAbleAbilityTaskResult::Successful);
	}

	m_FinishedSyncTasks.Empty();
	IterationAbility();
}

bool UAbleAbilityInstance::IsIterationDone() const
{
	if (!IsValid()) return true;

	if (GetCurrentTime() > m_Ability->GetLength(m_ActiveSegmentIndex))
	{
		if (m_Ability->SegmentMustFinishAllTasks(m_ActiveSegmentIndex))
		{
			return m_ActiveSyncTasks.Num() == 0 && m_SyncTasks.Num() <= 0;
		}

		return true;
	}
	else if (m_Ability->SegmentMustFinishAllTasks(m_ActiveSegmentIndex))
	{
		return m_ActiveSyncTasks.Num() == 0 && m_SyncTasks.Num() <= 0;
	}
	else if (m_Ability->IsSegmentLooping(m_ActiveSegmentIndex) && m_Context->GetCurrentSegmentLoopIteration() != 0)
	{
		if (GetCurrentTime() > m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex).Y) // If we're looping, check our range.
		{
			if (m_Ability->SegmentMustFinishAllTasks(m_ActiveSegmentIndex))
			{
				return m_ActiveSyncTasks.Num() == 0;
			}

			return true;
		}
	}

	return false;
}

bool UAbleAbilityInstance::IsActiveSegmentDone() const
{
	check(m_Ability != nullptr);
	
	const FAbilitySegmentDefineData* ActiveSegment = m_Ability->FindSegmentDefineDataByIndex(m_ActiveSegmentIndex);
	check(ActiveSegment != nullptr);
	
	if (ActiveSegment->m_Loop)
	{
		const uint32 MaxIterations = ActiveSegment->m_LoopMaxIterations;
		// Zero = infinite.
		return MaxIterations != 0 && (uint32)m_Context->GetCurrentSegmentLoopIteration() >= MaxIterations;
	}

	return true;
}

bool UAbleAbilityInstance::IsDone() const
{
	check(m_Ability != nullptr);

	const FAbilitySegmentDefineData* DefineData = m_Ability->FindSegmentDefineDataByIndex(m_ActiveSegmentIndex);
	if (DefineData != nullptr && DefineData->BranchData.Num() > 0)
	{
		return false;
	}
	
	if (m_Ability->IsSegmentLooping(m_ActiveSegmentIndex))
	{
		return IsActiveSegmentDone();
	}

	if (m_Ability->GetDecrementAndRestartOnEnd() && m_Ability->GetMaxStacksBP(m_Context) != 0)
	{
		return GetStackCount() == 0;
	}

	return true;
}

bool UAbleAbilityInstance::IsChanneled() const
{
	return m_Ability != nullptr && m_Ability->IsChanneled();
}

int UAbleAbilityInstance::CheckForDecay(UAbleAbilityComponent* AbilityComponent)
{
    if (m_Ability != nullptr)
    {
        float decayStackTime = m_Ability->GetDecayStackTime(&GetContext());
        if (decayStackTime > 0.0f)
        {
            int cancelStackCount = UKismetMathLibrary::FMod(m_DecayTime, decayStackTime, m_DecayTime);
            if (cancelStackCount > 0)
            {
                int newStackCount = FMath::Max(GetStackCount() - cancelStackCount, 0);
                AbilityComponent->SetPassiveStackCount(&GetAbility(), newStackCount, false, EAbleAbilityTaskResult::Decayed);
                return true;
            }
        }
    }
    return false;
}

EAbleConditionResults UAbleAbilityInstance::CheckChannelConditions() const
{
	check(m_Ability != nullptr);

#if WITH_EDITOR
    if (UWorld* World = m_Context->GetWorld())
    {
        if (World->WorldType == EWorldType::Type::EditorPreview)
            return EAbleConditionResults::ACR_Passed;
    }
#endif
	EAbleConditionResults ConditionResult = EAbleConditionResults::ACR_Passed;
	for (const UAbleChannelingBase* Condition : m_Ability->GetChannelConditions())
	{
		ConditionResult = Condition->GetConditionResult(*m_Context);

		if (ConditionResult == EAbleConditionResults::ACR_Passed && !m_Ability->MustPassAllChannelConditions())
		{
			// One condition passed.
			return EAbleConditionResults::ACR_Passed;
		}
		else if (ConditionResult == EAbleConditionResults::ACR_Failed && m_Ability->MustPassAllChannelConditions())
		{
			return EAbleConditionResults::ACR_Failed;
		}
	}

	return ConditionResult;
}

bool UAbleAbilityInstance::RequiresServerNotificationOfChannelFailure() const
{
	for (const UAbleChannelingBase* Condition : m_Ability->GetChannelConditions())
	{
		if (Condition && Condition->RequiresServerNotificationOfFailure())
		{
			return true;
		}
	}

	return false;
}

EAbleAbilityTaskResult UAbleAbilityInstance::GetChannelFailureResult() const
{
	check(m_Ability != nullptr);
	return m_Ability->GetChannelFailureResult();
}

void UAbleAbilityInstance::ResetTime(bool ToLoopStart /*= false*/)
{
	check(m_Ability != nullptr);
	if (ToLoopStart && m_Ability->IsSegmentLooping(m_ActiveSegmentIndex))
	{
		// Reset to the start of our loop.
		m_Context->SetCurrentTime(m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex).X);
	}
	else
	{
		m_Context->SetCurrentTime(0.0f);
	}
}

void UAbleAbilityInstance::SyncUpdate(float DeltaTime)
{
	const float CurrentTime = m_Context->GetCurrentTime();
	const float AdjustedTime = CurrentTime + DeltaTime;

	InternalUpdateTasks(m_SyncTasks, m_ActiveSyncTasks, m_FinishedSyncTasks, CurrentTime, DeltaTime);

	m_Context->UpdateTime(DeltaTime);

    m_DecayTime += DeltaTime;
}

void UAbleAbilityInstance::SetStackCount(int32 TotalStacks)
{
	check(m_Context != nullptr);
	m_Context->SetStackCount(TotalStacks);
}

int32 UAbleAbilityInstance::GetStackCount() const
{
	check(m_Context != nullptr);
	return m_Context->GetCurrentStackCount();
}

void UAbleAbilityInstance::AddAdditionalTargets(const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates /*= false*/, bool ClearTargets /*= false*/)
{
	check(m_Context != nullptr);

	const TArray<TWeakObjectPtr<AActor>>& CurrentTargets = m_Context->GetTargetActorsWeakPtr();

	if (AllowDuplicates)
	{
		FScopeLock Lock(&m_AddTargetCS);
		// Simple Append.
		m_AdditionalTargets.Append(AdditionalTargets);
	}
	else
	{
		//Worst Case is O(n^2) which sucks, but hopefully our list is small.
		TArray<TWeakObjectPtr<AActor>> UniqueEntries;

		for (const TWeakObjectPtr<AActor>& Target : AdditionalTargets)
		{
			if (!CurrentTargets.Contains(Target) || ClearTargets)
			{
				UniqueEntries.AddUnique(Target);
			}
		}

		FScopeLock Lock(&m_AddTargetCS);
		m_AdditionalTargets.Append(UniqueEntries);
	}

	m_ClearTargets = ClearTargets;
}

void UAbleAbilityInstance::ModifyContext(AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets /*= false*/)
{
	FScopeLock Lock(&m_AddTargetCS); // Just re-use this.
	m_AdditionalTargets.Append(AdditionalTargets);
	m_RequestedInstigator = Instigator;
	m_RequestedOwner = Owner;
	m_RequestedTargetLocation = TargetLocation;
	m_ClearTargets = ClearTargets;
}

void UAbleAbilityInstance::StopAbility()
{
	InternalStopRunningTasks(EAbleAbilityTaskResult::Successful);

	InternalStopAcrossTasks(EAbleAbilityTaskResult::Successful);
	
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAbleAbilityTaskResult::Successful);

		// BP
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAbleAbilityTaskResult::Successful);
	}

	// Call our Ability method
	m_Ability->OnAbilityEndBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void UAbleAbilityInstance::FinishAbility()
{
	if (!IsValid()) return;
	
	InternalStopRunningTasks(EAbleAbilityTaskResult::Successful);

	InternalStopAcrossTasks(EAbleAbilityTaskResult::Successful);
	
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAbleAbilityTaskResult::Successful);

		// BP
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAbleAbilityTaskResult::Successful);
	}

	if (!m_Ability || !m_Context) return;
	
	// Call our Ability method.
	m_Ability->OnAbilityEndBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void UAbleAbilityInstance::InterruptAbility()
{
	InternalStopRunningTasks(EAbleAbilityTaskResult::Interrupted);

	InternalStopAcrossTasks(EAbleAbilityTaskResult::Interrupted);
	
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityInterrupt().Broadcast(*m_Context);
        AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAbleAbilityTaskResult::Interrupted);

		// BP
		AbilityComponent->AbilityInterruptBPDelegate.Broadcast(m_Context);
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAbleAbilityTaskResult::Interrupted);
	}

	// Call our Ability method.
	m_Ability->OnAbilityInterruptBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void UAbleAbilityInstance::BranchAbility()
{
	InternalStopRunningTasks(EAbleAbilityTaskResult::Branched);

	InternalStopAcrossTasks(EAbleAbilityTaskResult::Branched);
	
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++ 
		AbilityComponent->GetOnAbilityBranched().Broadcast(*m_Context);
		AbilityComponent->GetOnAbilityEnd().Broadcast(*m_Context, EAbleAbilityTaskResult::Branched);

		// BP
		AbilityComponent->AbilityBranchBPDelegate.Broadcast(m_Context);
		AbilityComponent->AbilityEndBPDelegate.Broadcast(m_Context, EAbleAbilityTaskResult::Branched);
	}

	// Call our Ability method.
	m_Ability->OnAbilityBranchBP(m_Context);

	// Release Scratchpads
	m_Context->ReleaseScratchPads();

	// Free up our Context.
	m_Context->Reset();
	m_Context = nullptr;
}

void UAbleAbilityInstance::IterationAbility() const
{
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++
		AbilityComponent->GetOnAbilityIteration().Broadcast(*m_Context);

		// BP
		AbilityComponent->AbilityIterationBPDelegate.Broadcast(m_Context);
	}

	// Call our Ability method.
	if (m_Ability)
	{
		m_Ability->OnAbilityIterationBP(m_Context);
	}
}

bool UAbleAbilityInstance::BranchSegment(int SegmentIndex)
{
	if (m_ActiveSegmentIndex == SegmentIndex) return true;
	if (!GetAbility().IsSegmentIndexValid(SegmentIndex)) return false;

#if !(UE_BUILD_SHIPPING)
	const USPAbleSettings* m_Settings = GetDefault<USPAbleSettings>(USPAbleSettings::StaticClass());
	if (m_Settings->GetLogVerbose())
	{
		UE_LOG(LogAbleSP, Log, TEXT("BrachSegment [%s] to [%d] At Time"),
			*GetAbility().GetAbilityName(),
			SegmentIndex)
	}
#endif

	InternalStopRunningTasks(EAbleAbilityTaskResult::BranchSegment, false);
	//m_ClientNotifyTasks.Empty();

	m_Context->SetActiveSegmentIndex(SegmentIndex);
	SetActiveSegmentIndex(SegmentIndex);
	SetCurrentTime(0);

	// if (GetAbility().GetEntryIndex() != SegmentIndex) 这里不需要判断是因为在技能刚开始播放时并不会执行BranchSegment,而再次进入EntryIndex时候同样需要该功能,因此不需要判断
	{
		const FAbilitySegmentDefineData* SegmentDefineData = GetAbility().FindSegmentDefineDataByIndex(SegmentIndex);
		if (SegmentDefineData && SegmentDefineData->m_ReTargeting)
		{
			ResetTargetingForIteration(true);
		}
	}
	
	// Tell our Delegates
	if (UAbleAbilityComponent* AbilityComponent = m_Context->GetSelfAbilityComponent())
	{
		// C++ 
		AbilityComponent->GetOnAbilitySegmentBranched().Broadcast(*m_Context);

		// BP
		AbilityComponent->AbilitySegmentBranchBPDelegate.Broadcast(m_Context);
	}
	
	// Call our Ability method.
	m_Ability->OnAbilitySegmentBranchedBP(m_Context);
	
	return true;
}

UAbleAbilityContext& UAbleAbilityInstance::GetMutableContext()
{
	verify(m_Context != nullptr)
	return *m_Context;
}

const UAbleAbilityContext& UAbleAbilityInstance::GetContext() const
{
	verify(m_Context != nullptr);
	return *m_Context;
}

const UAbleAbility& UAbleAbilityInstance::GetAbility() const
{
	verify(m_Ability != nullptr);
	return *m_Ability;
}

float UAbleAbilityInstance::GetPlayRate() const
{
	return GetAbility().GetPlayRate(m_Context);
}

void UAbleAbilityInstance::SetCurrentTime(const float NewTime, const bool FromPreview)
{
	if (m_ActiveSegmentIndex < 0 || m_ActiveSegmentIndex >= m_Ability->GetSegmentNumber()) return;
	m_Context->SetCurrentTime(NewTime);

	TArray<UAbleAbilityTask*> NewTasks;
	TArray<UAbleAbilityTask*> CurrentTasks;

	const TArray<UAbleAbilityTask*> & Tasks = m_Ability->GetTasks(m_ActiveSegmentIndex);
	for (UAbleAbilityTask* Task : Tasks)
	{
		if (Task && Task->CanStart(m_Context, NewTime, 0.0f ) && !Task->IsDisabled() && !Task->IsAcrossSegment())
		{
			if (m_ActiveSyncTasks.Contains(Task))
			{
				CurrentTasks.Add(Task);
			}
			else
			{
				NewTasks.Add(Task);
			}
		}
	}

	// Now go through and Terminate any tasks that should no longer be running.
	for (int i = 0; i < m_ActiveSyncTasks.Num();)
	{
		if (!CurrentTasks.Contains(m_ActiveSyncTasks[i]))
		{
			m_ActiveSyncTasks[i]->OnTaskEnd(m_Context, Successful);
			UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::SetCurrentTime Index = [%d] TaskName [%s]"), i, *m_ActiveSyncTasks[i]->GetName());
			m_ActiveSyncTasks.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}
	
	// Start our New Tasks.
	/*for (UAbleAbilityTask* Task : NewTasks)
	{
		Task->OnTaskStart(m_Context);
		m_ActiveSyncTasks.Add(Task);
	}*/

	// Tell our Current Tasks to go ahead and do any logic they need to if the time was modified out from under them.
	for (UAbleAbilityTask* Task : CurrentTasks)
	{
		Task->OnAbilityTimeSet(m_Context);
	}
}

#if WITH_EDITOR
void UAbleAbilityInstance::SetPreviewTime(float NewTime)
{
	int Index = m_ActiveSegmentIndex;
	m_ActiveSegmentIndex = -1;
	SetActiveSegmentIndex(Index);
	SetCurrentTime(NewTime, true);
	for (const UAbleAbilityTask* Task : m_ActiveSyncTasks)
	{
		Task->OnAbilityPreviewTimeSet(m_Context);
	}
}
#endif

void UAbleAbilityInstance::SetActiveSegmentIndex(int Index)
{
	if (m_ActiveSegmentIndex == Index) return;

	m_ActiveSegmentIndex = Index;

#if WITH_EDITOR
	m_Ability->K2_OnSegmentChanged(Index);
#endif

	ENetMode NetMode = NM_Standalone;
	if (m_Context->GetSelfActor())
	{
		NetMode = m_Context->GetSelfActor()->GetNetMode();
	}
	// Sort our Tasks into Sync/Async queues.
	m_SyncTasks.Empty();
	m_SyncTaskAdditionInfo.Empty();

	const TArray<UAbleAbilityTask*> & Tasks = m_Ability->GetTasks(m_ActiveSegmentIndex);
	TArray<UAbleAbilityTask*> AcrossSegments;
	for (UAbleAbilityTask* Task : Tasks)
	{
		if (!Task || Task->IsDisabled())
		{
			continue;
		}
		
		/*if (Task->m_FeatureCondition && !Task->m_FeatureCondition->Check(m_Context))
		{
			continue;
		}*/

		if (const bool IsValidForNetwork = Task->IsValidForNetMode(NetMode, m_Context->GetSelfActor(), m_Context))
		{
			if (Task->IsAcrossSegment())
			{
				m_AcrossTasks.AddUnique(Task);
			}
			else
			{
				m_SyncTasks.Add(Task);
				m_SyncTaskAdditionInfo.Add(FSyncTaskAdditionInfo{ IsValidForNetwork });
			}
		}
	}
	
	{
		m_TaskDependencyMap.Empty();
		m_TaskIterationMap.Empty();
	}
	const TArray<const UAbleAbilityTask*>& TaskDependencies = m_Ability->GetAllTaskDependencies(m_ActiveSegmentIndex);
	for (const UAbleAbilityTask* TaskDependency : TaskDependencies)
	{
		if (TaskDependency == nullptr)
			continue;

		check(!m_TaskDependencyMap.Contains(TaskDependency));
		m_TaskDependencyMap.Add(TaskDependency);
	}

#if WITH_EDITOR
	m_SyncTasks.StableSort([](const UAbleAbilityTask& A, const UAbleAbilityTask& B)
	{
		return A.GetStartTime() < B.GetStartTime();
	});
	for (int i = 0; i < m_SyncTasks.Num(); i++)
	{
		m_SyncTaskAdditionInfo[i].IsValidForNetwork = m_SyncTasks[i]->IsValidForNetMode(NetMode, m_Context->GetSelfActor(), m_Context);
	}
#endif

	m_FinishedSyncTasks.Empty();
	m_FinishedSyncTasks.Reserve(m_SyncTasks.Num());
}

void UAbleAbilityInstance::SetCurrentIteration(uint32 NewIteration)
{
	// m_Context->SetLoopIteration(NewIteration);
	m_Context->SetSegmentLoopIteration(NewIteration, m_ActiveSegmentIndex);
}

void UAbleAbilityInstance::Reset()
{
	// Defaulting to false on the shrink for the arrays. These array entry values are so small, that having them constantly go in/out of allocation could be pretty gross for fragmentation.
	// Just keep the memory for now, and if it becomes an issue (not sure why it would), then just remove the false and let the memory get released.
	m_DecayTime = 0.0f;
	m_SyncTasks.Empty(false);
	m_ActiveSyncTasks.Empty(false);
	m_FinishedSyncTasks.Empty(false);
	m_Ability = nullptr;
	m_Context = nullptr;
	m_ClearTargets = false;
	m_AdditionalTargets.Empty(false);
	m_TaskDependencyMap.Empty(false);
	m_RequestedInstigator.Reset();
	m_RequestedOwner.Reset();
	m_RequestedTargetLocation = FVector::ZeroVector;
	m_ActiveSegmentIndex = -1;
	m_PreRemoveTask.Empty(false);
	m_AcrossTasks.Empty(false);
	m_ActiveAcrossTasks.Empty(false);
	m_TaskIterationMap.Empty();
	m_PendingPassed = false;
}

void UAbleAbilityInstance::StopAcrossTask(const TArray<const UAbleAbilityTask*>& Tasks)
{
	UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::StopAcrossTask 1 [%d]"), Tasks.Num());
	
	TArray<UAbleAbilityTask*> RemoveTasks;
	for (const UAbleAbilityTask* Task : Tasks)
	{
		if (UKismetSystemLibrary::IsValid(Task))
		{
			for (UAbleAbilityTask* AcrossTask : m_ActiveAcrossTasks)
			{
				if (UKismetSystemLibrary::IsValid(AcrossTask) && Task->GetTaskNameHash() == AcrossTask->GetTaskNameHash())
				{
					UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::StopAcrossTask 2 [%s]"), *AcrossTask->GetName());
					RemoveTasks.Add(AcrossTask);
					break;
				}
			}
			for (UAbleAbilityTask* AcrossTask : m_AcrossTasks)
			{
				if (UKismetSystemLibrary::IsValid(AcrossTask) && Task->GetTaskNameHash() == AcrossTask->GetTaskNameHash())
				{
					m_AcrossTasks.Remove(AcrossTask);
					break;
				}
			}
		}
	}

	for (UAbleAbilityTask* Task :RemoveTasks)
	{
		m_ActiveAcrossTasks.Remove(Task);
		Task->OnTaskEnd(m_Context, EAbleAbilityTaskResult::StopAcross);
	}
}

bool UAbleAbilityInstance::CheckForceDependency()
{
	if (!IsValid()) return false;

	if (!IsPendingPassed())
	{
		// check for force animation dependency
		if (GetAbility().IsForceDependencyAnimation())
		{
			if (GetContext().GetOwner())
			{
				TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(GetContext().GetOwner());
				for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
				{
					if (!SkeletalMeshComponent) continue;
				
					if (!SkeletalMeshComponent->SkeletalMesh || !SkeletalMeshComponent->GetAnimInstance())
					{
						UE_LOG(LogAbleSP, Warning, TEXT("UAbleAbilityInstance::CheckForceDependency for [%s] failed, wait until pass..."), *GetAbility().GetAbilityName())
						return false;
					}
				}
			}
		}
	}
	
	m_PendingPassed = true;
	return true;
}

void UAbleAbilityInstance::InternalUpdateTasks(TArray<UAbleAbilityTask*>& InTasks, TArray<const UAbleAbilityTask*>& InActiveTasks, TArray<const UAbleAbilityTask*>& InFinishedTasks, float CurrentTime, float DeltaTime)
{
	const float AdjustedTime = CurrentTime + DeltaTime;
	const bool IsLooping = m_Ability->IsSegmentLooping(m_ActiveSegmentIndex) || m_Ability->GetDecrementAndRestartOnEnd();
	const FVector2D LoopTimeRange = m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex);

	// Just for readability...
	bool IsStarting = false;

	// if we need to clean up any active tasks.
	bool NeedArrayCleanUp = false;

	TArray<const UAbleAbilityTask*> NewlyStartedTasks;
	TArray<const UAbleAbilityTask*> FinishedDependentTasks;

	TArray<UAbleAbilityTask*> StartAcrossTasks;
	for (UAbleAbilityTask* AcrossTask : m_AcrossTasks)
	{
		if (!UKismetSystemLibrary::IsValid(AcrossTask)) continue;
		IsStarting = AcrossTask->CanStart(m_Context, CurrentTime, DeltaTime) && !AcrossTask->IsDisabled();
		
		if (!IsStarting) continue;

		for (UAbleAbilityTask* ActiveAcrossTask : m_ActiveAcrossTasks)
		{
			if (AcrossTask->GetTaskNameHash() == ActiveAcrossTask->GetTaskNameHash())
			{
				IsStarting = false;
				break;
			}
		}

		if (IsStarting)
		{
			StartAcrossTasks.Add(AcrossTask);
		}
	}

	for (UAbleAbilityTask* AcrossTask : StartAcrossTasks)
	{
		AcrossTask->SetActualStartTime(m_Context->GetCurrentTime());
		// New Task to start.
		AcrossTask->OnTaskStart(m_Context);
		
		m_ActiveAcrossTasks.Add(AcrossTask);
		m_AcrossTasks.Remove(AcrossTask);
	}
	
	// First go through our Tasks and see if they need to be started.
	for (int i = 0; i < InTasks.Num(); )
	{
		UAbleAbilityTask* Task = InTasks[i];
		if (!Task)
		{
			++i;
			continue;
		}
		
		IsStarting = !Task->IsDisabled() && Task->CanStart(m_Context, CurrentTime, DeltaTime) && CanStartTask(Task);

		if (IsLooping)
		{
			// If we're looping (And thus not cleaning up tasks as we complete them), make sure we haven't already finished this task.
			IsStarting = IsStarting && !InFinishedTasks.Contains(Task);
		}

		// Check any dependencies.
		if (IsStarting && Task->HasDependencies())
		{
			for (const UAbleAbilityTask* DependantTask : Task->GetTaskDependencies())
			{
				if (!DependantTask)
				{
					continue;
				}

				// This *should* always be true, but in the editor sometimes the list isn't properly updated.
				// This is by far the assert I hit the most during iteration, so there's obviously a case I'm missing but I'd rather make this code a bit
				// more adaptive as it was written before I added the finished lists for both processes.
				if (!m_TaskDependencyMap.Contains(DependantTask))
				{
					FScopeLock DependencyMapLock(&m_DependencyMapCS);

					m_TaskDependencyMap.Add(DependantTask);
					m_TaskDependencyMap[DependantTask] = m_FinishedSyncTasks.Contains(DependantTask);
				}

				if (!m_TaskDependencyMap[DependantTask])
				{
					// A dependent Task is still executing, we can't start this frame.
					// UE_LOG(LogTemp, Warning, TEXT("Task [%s] Want to Execute but DependantTask [%s] did not complete yet !"), *GetNameSafe(Task), *GetNameSafe(DependantTask));
					IsStarting = false;
					break;
				}
				else
				{
					// Our dependency is done, but we have some context targets that are pending. These could be needed, so delay for a frame.
					IsStarting = m_AdditionalTargets.Num() == 0;
				}
			}
		}

		if (!Task)
		{
			UE_LOG(LogAbleSP, Warning, TEXT("Task NULL [%s]!"), *GetNameSafe(&GetAbility()));
		}

		if (IsStarting && !InActiveTasks.Contains(Task))
		{
			FScopeCycleCounter TaskScope(Task->GetStatId());

			Task->SetActualStartTime(m_Context->GetCurrentTime());
			// New Task to start.
			Task->OnTaskStart(m_Context);
			// Save into cache map
			if (m_TaskIterationMap.Contains(Task))
			{
				m_TaskIterationMap[Task]++;
			}
			else
			{
				m_TaskIterationMap.Add(Task, 1);
			}
			
			if (Task->IsSingleFrame())
			{
				if (!Task->IsAcrossSegment())
				{
					// We can go ahead and end this task and forget about it.
					Task->OnTaskEnd(m_Context, EAbleAbilityTaskResult::Successful);
				}

				if (m_TaskDependencyMap.Contains(Task))
				{
					FScopeLock DependencyMapLock(&m_DependencyMapCS);
					m_TaskDependencyMap[Task] = true;
				}

				InFinishedTasks.Add(Task);
			}
			else
			{
				NewlyStartedTasks.Add(Task);
			}
			
			if (!IsLooping || Task->GetStartTime() < LoopTimeRange.X)
			{
				// If we aren't looping, or our task starts before our loop range, we can remove items from this list as we start tasks to cut down on future iterations.
				InTasks.RemoveAt(i, 1, false);
				continue;
			}

		}
		else if (!Task || AdjustedTime < Task->GetStartTime())
		{
			// Since our array is sorted by time. It's safe to early out.
			break;
		}

		++i;
	}

	InTasks.Shrink();

	// Update our actives.
	bool TaskCompleted = false;
	const UAbleAbilityTask* ActiveTask = nullptr;
	for (int i = 0; i < InActiveTasks.Num(); )
	{
		ActiveTask = InActiveTasks[i];
		if (!ActiveTask) continue;
		
		FScopeCycleCounter ActiveTaskScope(ActiveTask->GetStatId());

		TaskCompleted = ActiveTask->IsDone(m_Context);
		// UE_LOG(LogTemp, Warning, TEXT("After IsDone [%s] TaskCompleted = %d !"), *GetNameSafe(ActiveTask), TaskCompleted);
		if (!TaskCompleted && ActiveTask->NeedsTick())
		{
			ActiveTask->OnTaskTick(m_Context, DeltaTime);
		}
		else if (TaskCompleted)
		{
			if (!ActiveTask->IsAcrossSegment())
			{
				ActiveTask->OnTaskEnd(m_Context, EAbleAbilityTaskResult::Successful);
			}

			if (m_TaskDependencyMap.Contains(ActiveTask))
			{
				FScopeLock DependencyMapLock(&m_DependencyMapCS);
				m_TaskDependencyMap[ActiveTask] = true;
				// UE_LOG(LogTemp, Warning, TEXT("m_TaskDependencyMap set Dependant %s true !"), *GetNameSafe(ActiveTask));
			}
			else
			{
				// UE_LOG(LogTemp, Warning, TEXT("m_TaskDependencyMap dont contains task [%s]!"), *GetNameSafe(ActiveTask));
			}

			if (IsLooping)
			{
				InFinishedTasks.Add(ActiveTask);
			}
			// UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::InternalUpdateTasks Index = [%d] TaskName [%s]"), i, *ActiveTask->GetName());
			InActiveTasks.RemoveAt(i, 1, false);
			continue;
		}

		++i;
	}

	for (UAbleAbilityTask* AcrossTask : m_ActiveAcrossTasks)
	{
		if (AcrossTask->NeedsTick())
		{
			AcrossTask->OnTaskTick(m_Context, DeltaTime);
		}
	}
	
	// Move our newly started tasks over.
	InActiveTasks.Append(NewlyStartedTasks);
	InActiveTasks.Shrink();
}

void UAbleAbilityInstance::InternalStopRunningTasks(EAbleAbilityTaskResult Reason, bool ResetForLoop)
{
	if (ResetForLoop)
	{
		// We only want to remove Tasks that fall within our Loop time range.
		const FVector2D LoopRange = m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex);

		const UAbleAbilityTask* CurrentTask = nullptr;
		TArray<int32> IndicesToRemove;

		for (int32 i = 0; i < m_ActiveSyncTasks.Num(); ++i)
		{
			CurrentTask = m_ActiveSyncTasks[i];
			if (!UKismetSystemLibrary::IsValid(CurrentTask)) continue;
			
			if (CurrentTask->GetStartTime() >= LoopRange.X && CurrentTask->GetEndTime() <= LoopRange.Y)
			{
				if (!CurrentTask->IsAcrossSegment())
				{
					CurrentTask->OnTaskEnd(m_Context, Reason);
				}
				
				// UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::InternalStopRunningTasks Index = [%d] TaskName [%s]"), i, *CurrentTask->GetName());
				IndicesToRemove.Add(i);
				// m_ActiveSyncTasks.RemoveAt(i, 1, false);
				continue;
			}
		}
		
		// 倒序删除元素
		for (int32 i = IndicesToRemove.Num() - 1; i >= 0; --i)
		{
			m_ActiveSyncTasks.RemoveAt(IndicesToRemove[i], 1, false);
		}

		IndicesToRemove.Empty();
		m_ActiveSyncTasks.Shrink();

	}
	else
	{
		for (const UAbleAbilityTask* Task : m_ActiveSyncTasks)
		{
			if (!UKismetSystemLibrary::IsValid(Task) || Task->IsAcrossSegment()) continue;
			Task->OnTaskEnd(m_Context, Reason);
		}
		m_ActiveSyncTasks.Empty();
	}

}

void UAbleAbilityInstance::InternalStopAcrossTasks(EAbleAbilityTaskResult Reason)
{
	// UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityInstance::InternalStopAcrossTasks [%d] [%d]"), Reason, m_ActiveAcrossTasks.Num());
	
	for (UAbleAbilityTask* AcrossTask : m_ActiveAcrossTasks)
	{
		AcrossTask->OnTaskEnd(m_Context, Reason);
	}

	m_AcrossTasks.Empty(false);
	m_ActiveAcrossTasks.Empty(false);
}

void UAbleAbilityInstance::ResetTaskDependencyStatus()
{
	TMap<const UAbleAbilityTask*, bool>::TIterator ResetIt = m_TaskDependencyMap.CreateIterator();
	const UAbleAbilityTask* Task;
	for (; ResetIt; ++ResetIt)
	{
		Task = ResetIt.Key();
		if (!m_Ability->IsSegmentLooping(m_ActiveSegmentIndex))
		{
			ResetIt.Value() = false;
		}
		else
		{
			// Only reset if our task falls within our Loop range.
			if (Task->GetStartTime() > m_Ability->GetSegmentLoopRange(m_ActiveSegmentIndex).X)
			{
				ResetIt.Value() = false;
			}
		}
	}
}

bool UAbleAbilityInstance::CanStartTask(const UAbleAbilityTask* Task) const
{
	if (!Task) return false;

	if (Task->GetOnceDuringIteration())
	{
		return !m_TaskIterationMap.Contains(Task);
	}

	return true;
}
