// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityComponent.h"

#include "ableAbility.h"
#include "ableAbilityInstance.h"
#include "ableAbilityUtilities.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "ableAbilityUtilities.h"
#include "Animation/AnimNode_SPAbilityAnimPlayer.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeLock.h"

#if WITH_EDITOR
#include "FXSystem.h"
#endif

#include "ableAbilityBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AbleCore"

FAbleAbilityCooldown::FAbleAbilityCooldown()
	: Ability(nullptr),
	Context(nullptr),
	CurrentTime(0.0f),
	CooldownTime(1.0f)
{

}
FAbleAbilityCooldown::FAbleAbilityCooldown(const UAbleAbility& InAbility, const UAbleAbilityContext& InContext)
	: Ability(nullptr),
	Context(nullptr),
	CurrentTime(0.0f),
	CooldownTime(1.0f)
{
	Ability = &InAbility;
	Context = &InContext;
	CooldownTime = Ability->GetCooldown(Context);
}

void FAbleAbilityCooldown::Update(float DeltaTime)
{
	if (Ability && Context && !Ability->CanCacheCooldown())
	{
		CooldownTime = Ability->GetCooldown(Context);
	}

	CurrentTime += DeltaTime;
}

UAbleAbilityComponent::UAbleAbilityComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ActiveAbilityInstance(nullptr),
    m_PassivesDirty(false),
	m_ClientPredictionKey(0),
	m_AbilityAnimationNode(nullptr)
{
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	SetIsReplicatedByDefault(true);

	m_Settings = GetDefault<USPAbleSettings>(USPAbleSettings::StaticClass());

	m_LocallyPredictedAbilities.Reserve(ABLE_ABILITY_PREDICTION_RING_SIZE);
	m_LocallyPredictedAbilities.AddDefaulted(ABLE_ABILITY_PREDICTION_RING_SIZE);
}

void UAbleAbilityComponent::BeginPlay()
{
	m_TagContainer.AppendTags(m_AutoApplyTags);

	Super::BeginPlay();
}

void UAbleAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (m_ActiveAbilityInstance)
	{
		m_ActiveAbilityInstance->StopAbility();
		HandleInstanceCleanUp(m_ActiveAbilityInstance->GetAbility());
	    m_ActiveAbilityInstance->Reset();
	}
	m_ActiveAbilityInstance = nullptr;
	m_ActiveAbilityResult = EAbleAbilityTaskResult::Successful;
	
	for (UAbleAbilityInstance* PassiveInstance : m_PassiveAbilityInstances)
	{
		if (PassiveInstance && PassiveInstance->IsValid())
		{
			PassiveInstance->StopAbility();
			HandleInstanceCleanUp(PassiveInstance->GetAbility());
		}
	}
	m_PassiveAbilityInstances.Empty();

	Super::EndPlay(EndPlayReason);
}

void UAbleAbilityComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::TickComponent"), STAT_AbleAbilityComponent_TickComponent, STATGROUP_Able);

	// Do our cooldowns first (if we're Async this will give it a bit of time to go ahead and start running).
	if (m_ActiveCooldowns.Num() > 0)
	{
		// Go ahead and fire off our cooldown update first.
		if (USPAbleSettings::IsAsyncEnabled() && m_Settings->GetAllowAsyncCooldownUpdate())
		{
			TGraphTask<FAsyncAbilityCooldownUpdaterTask>::CreateTask().ConstructAndDispatchWhenReady(this, DeltaTime);
		}
		else
		{
			UpdateCooldowns(DeltaTime);
		}
	}

	bool ActiveChanged = false;
	bool PassivesChanged = false;

	// Check the status of our Active, we only do this on our authoritative client, or if we're locally controlled for local simulation purposes.
	if (m_ActiveAbilityInstance && 
		(IsAuthoritative() || IsOwnerLocallyControlled()))
	{
		// Have to turn this on to prevent Users from apparently canceling the Ability that is finishing... (Seems like an extreme case, but whatever).
		m_IsProcessingUpdate = true;
		
		if (m_ActiveAbilityInstance->GetActiveSegmentIndex() != m_ActiveAbilityInstance->GetMutableContext().GetActiveSegmentIndex())
		{
			m_ActiveAbilityInstance->BranchSegment(m_ActiveAbilityInstance->GetMutableContext().GetActiveSegmentIndex());
		}
		
		// Check for done...
		if (m_ActiveAbilityInstance->IsIterationDone())
        {
			if (m_ActiveAbilityInstance->IsDone())
			{
				if (m_Settings->GetLogVerbose())
				{
					UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] IsDone, Queueing cancel."),
						*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*m_ActiveAbilityInstance->GetAbility().GetAbilityName());
				}
				
				m_PendingCancels.Add(FAblePendingCancelContext(m_ActiveAbilityInstance->GetAbilityNameHash(), EAbleAbilityTaskResult::Successful));

				ActiveChanged = true;
			}
			else if(m_ActiveAbilityInstance->IsActiveSegmentDone())
			{
				if (m_Settings->GetLogVerbose())
				{
					/* UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] IsActiveSegmentDone [%d], Queueing Branch Segment."),
						*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*m_ActiveAbilityInstance->GetAbility().GetDisplayName(),
						m_ActiveAbilityInstance->GetActiveSegmentIndex()); */
				}

				// 获取技能
				const UAbleAbility& Ability = m_ActiveAbilityInstance->GetAbility();
				const int ActiveIndex = m_ActiveAbilityInstance->GetActiveSegmentIndex();
				const FAbilitySegmentDefineData* DefineData = Ability.FindSegmentDefineDataByIndex(ActiveIndex);
				if (DefineData != nullptr)
				{
					int NextIndex = -1;
					bool bWait = false;
					EAbleAbilityTaskRealm Realm = EAbleAbilityTaskRealm::ATR_TotalRealms;
					UAbleAbilityContext* Context = &m_ActiveAbilityInstance->GetMutableContext();
					for (auto& BranchData : DefineData->BranchData)
					{
						int BranchIndex = Ability.FindSegmentIndexByFName(BranchData.NextName);
						if (BranchIndex < 0)
							continue;
						if (!UAbleAbilityBlueprintLibrary::IsValidForNetwork( Context->GetSelfActor(), BranchData.Realm))
						{
							bWait = true;
							break;
						}
						if (UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(Context, BranchData))
						{
							NextIndex = BranchIndex;
							Realm = BranchData.Realm;
							break;
						}
					}
					if (NextIndex >= 0 && !bWait)
					{
						m_ActiveAbilityInstance->BranchSegment(NextIndex);
						
						if (Realm == EAbleAbilityTaskRealm::ATR_Client)
						{
							ServerBranchSegment(NextIndex, FJumpSegmentSetting());
						}
					}
				}
			}
			else
			{
				m_ActiveAbilityInstance->ResetForNextIteration();
			}
		}
		
		// Check for Channeling...
		if (!ActiveChanged && m_ActiveAbilityInstance->IsChanneled())
		{
			if (m_ActiveAbilityInstance->CheckChannelConditions() == EAbleConditionResults::ACR_Failed)
			{
				m_ActiveAbilityResult = m_ActiveAbilityInstance->GetChannelFailureResult();

                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] Channel Condition Failure [%s], Queueing cancel."),
                        *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(m_ActiveAbilityInstance->GetAbility().GetAbilityName()),
                        *FAbleSPLogHelper::GetTaskResultEnumAsString(m_ActiveAbilityResult));
                }

				if (!IsAuthoritative() && m_ActiveAbilityInstance->RequiresServerNotificationOfChannelFailure())
				{
					// ServerCancelAbility(m_ActiveAbilityInstance->GetAbilityNameHash(), m_ActiveAbilityInstance->GetChannelFailureResult());
				}

				m_PendingCancels.Add(FAblePendingCancelContext(m_ActiveAbilityInstance->GetAbilityNameHash(), m_ActiveAbilityInstance->GetChannelFailureResult()));

				ActiveChanged = true;
			}
		}

		m_IsProcessingUpdate = false;
	}
	int32 PrePendingPassiveCount = m_PassiveAbilityInstances.Num();
	UAbleAbilityInstance* PrePendingActiveInstance = m_ActiveAbilityInstance;

	// Handle any Pending cancels.
	HandlePendingCancels();

#if WITH_EDITOR
	if (ActiveChanged && m_ActiveAbilityInstance)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Killed Active Ability manually after it failed to cancel."));
		m_ActiveAbilityInstance->Reset();
		m_ActiveAbilityInstance = nullptr;
	}
#endif

	// Handle any Pending abilities.
	HandlePendingContexts();

	ActiveChanged |= PrePendingActiveInstance != m_ActiveAbilityInstance;
	PassivesChanged |= m_PassivesDirty;

	// Try and process our Async targeting queue..
	TArray<UAbleAbilityContext*> RemovalList;
	for (TArray<UAbleAbilityContext*>::TIterator ItAsync = m_AsyncContexts.CreateIterator(); ItAsync; ++ItAsync)
	{
		EAbleAbilityStartResult StartResult = InternalStartAbility((*ItAsync));
		if (StartResult != EAbleAbilityStartResult::AsyncProcessing)
		{
			// We got some result (either pass or fail), remove this from further processing.
			RemovalList.Add(*ItAsync);

			if ((*ItAsync)->GetAbility()->IsPassive())
			{
				PassivesChanged = true;
			}
			else
			{
				ActiveChanged = true;
			}
		}
	}

	for (UAbleAbilityContext* ToRemove : RemovalList)
	{
		m_AsyncContexts.Remove(ToRemove);
	}

	m_IsProcessingUpdate = true;

	// Update our Active
	if (m_ActiveAbilityInstance)
	{
		// Process update (or launch a task to do it).
		InternalUpdateAbility(m_ActiveAbilityInstance, DeltaTime * m_ActiveAbilityInstance->GetPlayRate());
	}

	// Update Passives
	UAbleAbilityInstance* Passive = nullptr;
	for (int i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
	{
		Passive = m_PassiveAbilityInstances[i];
		if (Passive && Passive->IsValid())
		{
            bool PassiveFinished = false;

			if (Passive->IsChanneled() && (IsAuthoritative() || IsOwnerLocallyControlled()))
			{
				if (Passive->CheckChannelConditions() == EAbleConditionResults::ACR_Failed)
				{
					UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] Channel Condition Failure [%s]"),
						*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*(Passive->GetAbility().GetAbilityName()),
						*FAbleSPLogHelper::GetTaskResultEnumAsString(Passive->GetChannelFailureResult()));

					CancelAbility(&Passive->GetAbility(), Passive->GetChannelFailureResult());

					continue;
				}
			}

			if (Passive->IsIterationDone() && (IsAuthoritative() || IsOwnerLocallyControlled()))
			{
				if (Passive->IsDone())
				{
					if (m_Settings->GetLogVerbose())
					{
						UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] Passive IsDone"),
							*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
							*Passive->GetAbility().GetAbilityName());
					}

					PassiveFinished = true;
					Passive->FinishAbility();
				}
				else if(Passive->IsActiveSegmentDone())
				{
					// 获取技能
					const UAbleAbility& Ability = Passive->GetAbility();
					const int ActiveIndex = Passive->GetActiveSegmentIndex();
					const FAbilitySegmentDefineData* DefineData = Ability.FindSegmentDefineDataByIndex(ActiveIndex);
					if (DefineData != nullptr)
					{
						int NextIndex = -1;
						bool bWait = false;
						UAbleAbilityContext* Context = &m_ActiveAbilityInstance->GetMutableContext();
						EAbleAbilityTaskRealm Realm = EAbleAbilityTaskRealm::ATR_TotalRealms;
						for (auto& BranchData : DefineData->BranchData)
						{
							int BranchIndex = Ability.FindSegmentIndexByFName(BranchData.NextName);
							if (BranchIndex < 0)
								continue;
							if (!UAbleAbilityBlueprintLibrary::IsValidForNetwork( Context->GetSelfActor(), BranchData.Realm))
							{
								bWait = true;
								break;
							}
							if (UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(Context, BranchData))
							{
								NextIndex = BranchIndex;
								Realm = BranchData.Realm;
								break;
							}
						}
						if (NextIndex >= 0 && !bWait)
						{
							Passive->BranchSegment(NextIndex);
							if (Realm == EAbleAbilityTaskRealm::ATR_Client)
							{
								ServerBranchSegment(NextIndex, FJumpSegmentSetting());
							}
						}
					}
				}
				else
				{
					Passive->ResetForNextIteration();
				}

			}
			else
			{
				InternalUpdateAbility(Passive, DeltaTime * Passive->GetPlayRate());

				if (Passive->IsIterationDone())
				{
					if (Passive->IsDone())
					{
						if (m_Settings->GetLogVerbose())
						{
							UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] Passive Update->IsDone"),
								*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
								*Passive->GetAbility().GetAbilityName());
						}

						PassiveFinished = true;
						Passive->FinishAbility();
					}
					else if(Passive->IsActiveSegmentDone())
					{
						// 获取技能
						const UAbleAbility& Ability = Passive->GetAbility();
						const int ActiveIndex = Passive->GetActiveSegmentIndex();
						const FAbilitySegmentDefineData* DefineData = Ability.FindSegmentDefineDataByIndex(ActiveIndex);
						if (DefineData != nullptr)
						{
							int NextIndex = -1;
							bool bWait = false;
							UAbleAbilityContext* Context = &Passive->GetMutableContext();
							EAbleAbilityTaskRealm Realm = EAbleAbilityTaskRealm::ATR_TotalRealms;
							for (auto& BranchData : DefineData->BranchData)
							{
								int BranchIndex = Ability.FindSegmentIndexByFName(BranchData.NextName);
								if (BranchIndex < 0)
									continue;
 								if (!UAbleAbilityBlueprintLibrary::IsValidForNetwork( Context->GetSelfActor(), BranchData.Realm))
								{
									bWait = true;
									break;
								}
								if (UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(Context, BranchData))
								{
									NextIndex = BranchIndex;
									Realm = BranchData.Realm;
									break;
								}
							}
							if (NextIndex >= 0 && !bWait)
							{
								Passive->BranchSegment(NextIndex);
								if (Realm == EAbleAbilityTaskRealm::ATR_Client)
								{
									ServerBranchSegment(NextIndex, FJumpSegmentSetting());
								}
							}
						}
					}
					else
					{
						Passive->ResetForNextIteration();
					}
				}

				if (Passive && Passive->IsValid() && Passive->GetActiveSegmentIndex() != Passive->GetMutableContext().GetActiveSegmentIndex())
				{
					Passive->BranchSegment(Passive->GetMutableContext().GetActiveSegmentIndex());
				}
			}

            // Check for stack decay
            if (!PassiveFinished && (IsAuthoritative() || IsOwnerLocallyControlled()))
            {
                int32 decayedStacks = Passive->CheckForDecay(this);
                if (decayedStacks > 0)
                {
                    if (m_Settings->GetLogVerbose())
                    {
                        UE_LOG(LogAbleSP, Warning, TEXT("[%s] TickComponent [%s] Passive Ability Decayed [%d] stacks"),
                            *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                            *(Passive->GetAbility().GetAbilityName()),
                            decayedStacks);
                    }
                }
            }
		}
	}

	// Clean up finished passives.
	int32 Removed = 0;
	for (UAbleAbilityInstance* Instance : m_PassiveAbilityInstances)
	{
		if (!Instance || !Instance->IsValid() || (Instance->IsIterationDone() && Instance->IsDone()))
		{
			Removed++;
			m_PendingCancels.Add(FAblePendingCancelContext(Instance->GetAbilityNameHash(), EAbleAbilityTaskResult::Successful));
		}
	}
	// Handle any Pending cancels.
	HandlePendingCancels();
	
    PassivesChanged |= Removed > 0;

	if (IsNetworked() && IsAuthoritative())
	{
		// Make sure we keep our client watched fields in sync.
		if (ActiveChanged)
		{
			UpdateServerActiveAbility();
		}

		if (PassivesChanged)
		{
			UpdateServerPassiveAbilities();
            m_PassivesDirty = false;
		}
	}

	CheckNeedsTick();

	// We've finished our update, validate things for remote clients - any Abilities that need to restart will begin next frame.
	ValidateRemoteRunningAbilities();

	m_IsProcessingUpdate = false;
}

void UAbleAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These two fields are replicated and watched by the client.
	// DOREPLIFETIME(UAbleAbilityComponent, m_ServerActive);
	// DOREPLIFETIME(UAbleAbilityComponent, m_ServerPassiveAbilities);
	/*DOREPLIFETIME(UAbleAbilityComponent, m_ServerPredictionKey);*/
}

void UAbleAbilityComponent::BeginDestroy()
{
	Super::BeginDestroy();
}

bool UAbleAbilityComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	/*for (UAbleAbility* Ability : m_CreatedAbilityInstances)
	{
		if (Ability && !Ability->IsPendingKill())
		{
			WroteSomething |= Channel->ReplicateSubobject(Ability, *Bunch, *RepFlags);
		}
	}*/

	return WroteSomething;
}

bool UAbleAbilityComponent::IsNetworked() const
{
	return GetNetMode() != ENetMode::NM_Standalone && GetOwnerRole() != ROLE_SimulatedProxy;
}

EAbleAbilityStartResult UAbleAbilityComponent::CanActivateAbility(UAbleAbilityContext* Context) const
{
	if (!Context)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Null Context passed CanActivateAbility method."));
		return EAbleAbilityStartResult::InternalSystemsError;
	}
	
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::CanActivateAbility"), STAT_AbleAbilityComponent_CanActivateAbility, STATGROUP_Able);
	
	if (const UAbleAbility* Ability = Context->GetAbility())
	{
		if (Ability->IsPassive())
		{
			return CanActivatePassiveAbility(Context);
		}

		if (IsAbilityOnCooldown(Ability))
		{
			return EAbleAbilityStartResult::CooldownNotExpired;
		}

		if (m_ActiveAbilityInstance && !IsAbilityInstancePendingCancel(m_ActiveAbilityInstance))
		{
			// We're already playing an active ability, check if interrupt is allowed.
			if (!Ability->CanInterruptAbilityBP(Context, &m_ActiveAbilityInstance->GetAbility()))
			{
				if (m_Settings->GetLogAbilityFailures())
				{
					UE_LOG(LogAbleSP, Warning, TEXT("[%s] CanActivateAbility Ability [%s] not allowed to interrupt ability [%s]"),
						*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
						*GetNameSafe(Ability),
						*GetNameSafe(&m_ActiveAbilityInstance->GetAbility()));
				}
				
				return EAbleAbilityStartResult::CannotInterruptCurrentAbility;
			}

			if (m_Settings->GetLogVerbose())
			{
				UE_LOG(LogAbleSP, Warning, TEXT("[%s] CanActivateAbility Ability [%s] allowed to interrupt ability [%s]"),
					*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
					*(Context->GetAbility()->GetAbilityName()),
					*m_ActiveAbilityInstance->GetAbility().GetAbilityName());
			}
		}

		return Ability->CanAbilityExecute(*Context);
	}

	// Our Ability wasn't valid. 
	return EAbleAbilityStartResult::InternalSystemsError;
}

bool UAbleAbilityComponent::BranchSegment(const UAbleAbilityContext* Context, int SegmentIndex,
	const FJumpSegmentSetting& JumpSetting)
{
	#if !(UE_BUILD_SHIPPING)
	if (m_Settings->GetLogVerbose())
	{
		if(m_ActiveAbilityInstance)
			UE_LOG(LogAbleSP, Log, TEXT("BranchSegment, active abl name %s in hash %i, %f "), *m_ActiveAbilityInstance->GetAbility().GetAbilityName(), m_ActiveAbilityInstance->GetAbilityNameHash(), GetWorld()->GetTimeSeconds());
	}
#endif

	/*if (m_IsEndPlay)
	{
#if !(UE_BUILD_SHIPPING)
		if (m_Settings->GetLogVerbose())
		{
			UE_LOG(LogAbleSP, Log, TEXT("BranchSegment, failed for skill %i, segment %i, %f, reason: component endplay "), Context->GetSkillID(), SegmentIndex, GetWorld()->GetTimeSeconds());
		}
#endif
		return false;
	}*/
	
	if (!Context)
	{
		return false;
	}

	const UAbleAbility* Ability = Context->GetAbility();
	if (!Ability)
	{
		return false;
	}
	
	/*if (Context->m_IsPassiveAbility)
	{
#if !(UE_BUILD_SHIPPING)
		if (m_Settings->GetLogVerbose())
		{
			UE_LOG(LogAble, Log, TEXT("BranchSegment, failed for skill %i, segment %i, %f, reason: passive "), Context->GetSkillID(), SegmentIndex, GetWorld()->GetTimeSeconds());
		}
#endif
		return false;
	}*/

	if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
	{
		//we must execute branch segment in updating scope
		//because some task will be executed in brach segment,if the task is cancel ability, the task loop will be failed
		//it will cause task index is invalid
		if (IsProcessingUpdate() || true)
		{
			m_ActiveAbilityInstance->GetMutableContext().SetActiveSegmentIndex(SegmentIndex);
			m_ActiveAbilityInstance->GetMutableContext().SetJumpSegmentSetting(JumpSetting);

#if !(UE_BUILD_SHIPPING)
			if (m_Settings->GetLogVerbose())
			{
				UE_LOG(LogAbleSP, Log, TEXT("BranchSegment, success for ability %i, segment %i, %f "), Context->GetAbilityId(), SegmentIndex, GetWorld()->GetTimeSeconds());
			}
#endif
			return true;
		}
		/*else
		{
			if (IsOwnerLocallyControlled())
			{
				FAblAbilityNetworkContext NewContext = FAblAbilityNetworkContext(*Context);
				NewContext.SetStartSegmentIndex(SegmentIndex);
				ServerBranchSegment(NewContext);
			}
			bool Ret = m_ActiveAbilityInstance->BranchSegment(SegmentIndex);
			if (IsAuthoritative()) m_ServerActive.UpdateWithContext(&m_ActiveAbilityInstance->GetContext());
			return Ret;
		}*/
	}
	else
	{
		for (UAbleAbilityInstance* PassiveAbilityInstance : m_PassiveAbilityInstances)
		{
			if (PassiveAbilityInstance && PassiveAbilityInstance->IsValid() && PassiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
			{
				if (IsProcessingUpdate() || true)
				{
					PassiveAbilityInstance->GetMutableContext().SetActiveSegmentIndex(SegmentIndex);
					PassiveAbilityInstance->GetMutableContext().SetJumpSegmentSetting(JumpSetting);

#if !(UE_BUILD_SHIPPING)
					if (m_Settings->GetLogVerbose())
					{
						UE_LOG(LogAbleSP, Log, TEXT("BranchSegment, success for ability %i, segment %i, %f "), Context->GetAbilityId(), SegmentIndex, GetWorld()->GetTimeSeconds());
					}
#endif
					return true;
				}
			}
		}
	}

#if !(UE_BUILD_SHIPPING)
	if (m_Settings->GetLogVerbose())
	{
		UE_LOG(LogAbleSP, Log, TEXT("BranchSegment, failed for skill %i, segment %i, %f, reason: Invalid/mismatch active ability "), Context->GetAbilityId(), SegmentIndex, GetWorld()->GetTimeSeconds());
	}
#endif
	return false;
}

bool UAbleAbilityComponent::BranchSegmentWithName(const UAbleAbilityContext* Context, FName SegmentName)
{
	int SegmentIndex = FindSegment(Context, SegmentName);
	if (SegmentIndex >= 0)
	{
		return BranchSegment(Context, SegmentIndex, FJumpSegmentSetting());
	}
	return false;
}

bool UAbleAbilityComponent::JumpSegment(const UAbleAbilityContext* Context, const FJumpSegmentSetting& JumpSetting)
{
	const UAbleAbility* Ability = Context->GetAbility();

	if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
	{
		//we must execute branch segment in updating scope
		//because some task will be executed in brach segment,if the task is cancel ability, the task loop will be failed
		//it will cause task index is invalid
		if (IsProcessingUpdate() || true)
		{
			// 获取技能
			const int ActiveIndex = m_ActiveAbilityInstance->GetActiveSegmentIndex();
			const FAbilitySegmentDefineData* DefineData = Ability->FindSegmentDefineDataByIndex(ActiveIndex);
			if (DefineData != nullptr)
			{
				int NextIndex = -1;
				EAbleAbilityTaskRealm Realm = EAbleAbilityTaskRealm::ATR_TotalRealms;
				for (auto& BranchData : DefineData->BranchData)
				{
					int BranchIndex = Ability->FindSegmentIndexByFName(BranchData.NextName);
					if (BranchIndex < 0)
						continue;
					if (UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(&m_ActiveAbilityInstance->GetMutableContext(), BranchData))
					{
						NextIndex = BranchIndex;
						Realm = BranchData.Realm;
						break;
					}
				}
				if (NextIndex >= 0)
				{
					m_ActiveAbilityInstance->GetMutableContext().SetActiveSegmentIndex(NextIndex);
					m_ActiveAbilityInstance->GetMutableContext().SetJumpSegmentSetting(JumpSetting);
					if (Realm == EAbleAbilityTaskRealm::ATR_Client)
					{
						ServerBranchSegment(NextIndex, FJumpSegmentSetting());
					}
					return true;
				}
			}
		}
	}
	
	return false;
}

int32 UAbleAbilityComponent::FindSegment(const UAbleAbilityContext* Context, const FName& SegmentName)
{
	const UAbleAbility* Ability = Context->GetAbility();
	if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
	{
		return Ability->FindSegmentIndexByFName(SegmentName);
	}

	for (UAbleAbilityInstance* PassiveAbilityInstance : m_PassiveAbilityInstances)
	{
		if (PassiveAbilityInstance && PassiveAbilityInstance->IsValid() && PassiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			return Ability->FindSegmentIndexByFName(SegmentName);
		}
	}

	return -1;
}

void UAbleAbilityComponent::StopAcrossTask(const UAbleAbilityContext* Context,
	const TArray<const UAbleAbilityTask*>& Tasks)
{
	UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityComponent::StopAcrossTask 1"));
	const UAbleAbility* Ability = Context->GetAbility();
	if (IsValid(Ability) && m_ActiveAbilityInstance)
	{
		UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityComponent::StopAcrossTask 2 [%d:%d]"), m_ActiveAbilityInstance->GetAbilityNameHash(), Ability->GetAbilityNameHash());
		if (m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			m_ActiveAbilityInstance->StopAcrossTask(Tasks);
		}
		else
		{
			UE_LOG(LogAbleSP, Warning, TEXT("UAbleAbilityComponent::StopAcrossTask Ability is not match"));
		}
	}
}

void UAbleAbilityComponent::BranchSegmentFromClient(const UAbleAbilityContext* Context, int SegmentIndex, const FJumpSegmentSetting& JumpSetting)
{
	BranchSegment(Context, SegmentIndex, JumpSetting);

	ServerBranchSegment(SegmentIndex, JumpSetting);
}

EAbleAbilityStartResult UAbleAbilityComponent::ActivateAbility(UAbleAbilityContext* Context, bool ServerActivatedAbility)
{
	EAbleAbilityStartResult Result = EAbleAbilityStartResult::InternalSystemsError;
	/** modify SP bladeyuan
	 * 由于在SP项目中Able原生提供的机制不足以支撑项目需求，例如Consume、Event等机制，但其实直接Hack进行机制扩充也是可行的，
	 * 但是Hack的成本其实与自行开发一套的成本相差不大，且对于网络决策方面自行维护的可控性更高，Able中近期踩的坑略多，
	 * 故综合考虑决定在Able上自行封装一层技能系统，Able只充当简单的timeline工具，其中的Network、Cooldown、Passive等机制全部替换为外部自行实现来进行控制
	 * 这里直接注释掉所有网络通信相关的代码，原因如下三：
	 * 1.所有网络相关的流程需要统一，防止其他模块自行调用Able进行使用后进行网络消息的隐匿通信，给技能管线带来不必要的调试和查错成本，所有需要涉及到需要同步的需求一律收束到上层的技能框架中
	 * 2.由于上层技能框架已经实现了类似的网络同步且网络决策可控，这里的ActivateAbility在执行时可以全部认为是LocalClient或Server环境，即已经在目标环境中，所以可以直接InternalStart而无需关心通信相关逻辑
	 * 3.对于外界模块需要使用Able呈现表现时，需要自行决策和关心该逻辑的执行端，我们并不希望外界模块对“表现”的期望是自带网络通信
	**/
	
	/*if (IsNetworked())
	{
		if (!IsAuthoritative())
		{
			// Pass it to the server to validate/execute.
			FAbleAbilityNetworkContext AbilityNetworkContext(*Context);
			if (m_Settings->GetAlwaysForwardToServerFirst())
			{
				ServerActivateAbility(AbilityNetworkContext);
			}

			// If we're a locally controlled player...
			if (IsOwnerLocallyControlled())
			{
				if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
				{
					// Track it for prediction.
					AddLocallyPredictedAbility(AbilityNetworkContext);

					QueueContext(Context, EAbleAbilityTaskResult::Successful);

					Result = EAbleAbilityStartResult::Success;
				}
				else // if (Context->GetAbility()->GetInstancePolicy() != EAbleInstancePolicy::NewInstanceReplicated)
				{
					// Go ahead and attempt to play the ability locally, the server is authoritative still. Worst case you end up playing an ability but
					// the server rejects it for whatever reason. That's still far better than having a delay to all actions.
					Result = InternalStartAbility(Context);

					if (!m_Settings->GetAlwaysForwardToServerFirst())
					{
						if (Result == EAbleAbilityStartResult::Success)
						{
							ServerActivateAbility(AbilityNetworkContext);
							// Track it for prediction.
							AddLocallyPredictedAbility(AbilityNetworkContext);
						}
					}
					else
					{
						// Track it for prediction.
						AddLocallyPredictedAbility(AbilityNetworkContext);
					}
				}

			}
			else
			{
				Result = EAbleAbilityStartResult::ForwardedToServer;
			}
		}
		else
		{
			if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
			{
				QueueContext(Context, EAbleAbilityTaskResult::Successful);

				Result = EAbleAbilityStartResult::Success;
			}
			else
			{
				// We're authoritative. Start the Ability.
				Result = InternalStartAbility(Context);
			}
		}
	}
	else
	{
		if (IsProcessingUpdate()) // We're in the middle of an update, set it as the pending context so our Ability has a chance to finish.
		{
			QueueContext(Context, EAbleAbilityTaskResult::Successful);

			Result = EAbleAbilityStartResult::Success;
		}
		else
		{
			// Local game, just pass it along.
			Result = InternalStartAbility(Context);
		}
	}*/

	Result = InternalStartAbility(Context, ServerActivatedAbility);
	CheckNeedsTick();

	return Result;
}

bool UAbleAbilityComponent::HasAbilityAnimation() const
{
	return m_AbilityAnimationNode && m_AbilityAnimationNode->HasAnimationToPlay();
}

void UAbleAbilityComponent::CancelActiveAbility(EAbleAbilityTaskResult ResultToUse)
{
	if (m_ActiveAbilityInstance)
	{
		// Network forwarding should already be handled by this point by CancelAbility.
		bool bIsPawnLocallyControlled = false;
		APawn* PawnOwner = Cast<APawn>(GetOwner());
		if(PawnOwner)
		{
			AController* CurController = PawnOwner->GetController();
			ENetMode NetMode = GetNetMode();
			ENetRole PawnRole = PawnOwner->GetLocalRole();
			if(CurController == nullptr)
			{
				if (NetMode == NM_Client && PawnRole == ROLE_AutonomousProxy)
				{
					bIsPawnLocallyControlled = true;
				}
			}
		}
		
		if (IsAuthoritative() || IsOwnerLocallyControlled() || bIsPawnLocallyControlled)
		{
			if (IsProcessingUpdate())
			{
				// UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityComponent::CancelActiveAbility IsProcessingUpdate true"));
				m_PendingCancels.Add(FAblePendingCancelContext(m_ActiveAbilityInstance->GetAbilityNameHash(), ResultToUse));
				return;
			}

			m_IsProcessingUpdate = true;
			
			// UE_LOG(LogAbleSP, Log, TEXT("UAbleAbilityComponent::CancelActiveAbility Not IsProcessingUpdate"));
			switch (ResultToUse)
			{
			case EAbleAbilityTaskResult::Branched:
				m_ActiveAbilityInstance->BranchAbility();
				break;
			case EAbleAbilityTaskResult::Interrupted:
				m_ActiveAbilityInstance->InterruptAbility();
				break;
			default:
			case EAbleAbilityTaskResult::Successful:
				m_ActiveAbilityInstance->FinishAbility();
				break;
			}
			
			m_IsProcessingUpdate = false;
		}

        if (m_Settings->GetLogVerbose())
        {
            UE_LOG(LogAbleSP, Warning, TEXT("[%s] CancelActiveAbility( %s ) Result [%s]"),
                *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *m_ActiveAbilityInstance->GetAbility().GetAbilityName(),
                *FAbleSPLogHelper::GetTaskResultEnumAsString(ResultToUse));
        }
		
		HandleInstanceCleanUp(m_ActiveAbilityInstance->GetAbility());
		m_ActiveAbilityInstance->Reset();
		m_ActiveAbilityInstance = nullptr;
		m_ActiveAbilityResult = ResultToUse;
	}

	if (IsNetworked() && IsAuthoritative())
	{
		// Tell the client to reset as well.
		UpdateServerActiveAbility();
	}
}

const UAbleAbility* UAbleAbilityComponent::GetConstActiveAbility() const
{
	if (m_ActiveAbilityInstance)
	{
		const UAbleAbility& CurrentAbility = m_ActiveAbilityInstance->GetAbility();
		return &CurrentAbility;
	}

	return nullptr;
}

UAbleAbility* UAbleAbilityComponent::GetActiveAbility() const
{
	return const_cast<UAbleAbility*>(GetConstActiveAbility());
}

void UAbleAbilityComponent::ExecuteCustomEventForActiveAbility(const UAbleAbility* Ability, const FName& EventName)
{
    if (m_ActiveAbilityInstance)
    {
        const UAbleAbilityContext& CurrentAbilityContext = m_ActiveAbilityInstance->GetContext();
        const UAbleAbility& CurrentAbility = m_ActiveAbilityInstance->GetAbility();
        if (Ability == &CurrentAbility)
        {
            return CurrentAbility.OnCustomEvent(&CurrentAbilityContext, EventName);
        }
    }
}

EAbleAbilityStartResult UAbleAbilityComponent::CanActivatePassiveAbility(UAbleAbilityContext* Context) const
{
	if (!Context)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Null Context passed to CanActivatePassiveAbility method."));
		return EAbleAbilityStartResult::InternalSystemsError;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::CanActivatePassiveAbility"), STAT_AbleAbilityComponent_CanActivatePassiveAbility, STATGROUP_Able);

	EAbleAbilityStartResult Result = EAbleAbilityStartResult::InternalSystemsError;

	if (const UAbleAbility* Ability = Context->GetAbility())
	{
		/*if (IsAbilityOnCooldown(Ability))
		{
			return EAbleAbilityStartResult::CooldownNotExpired;
		}*/

		if (!Ability->IsPassive())
		{
			return EAbleAbilityStartResult::NotAllowedAsPassive;
		}

		// Do our stack count first, since it should be pretty cheap.
		/*int32 CurrentStackCount = GetCurrentStackCountForPassiveAbility(Ability);
		if (CurrentStackCount == 0 || CurrentStackCount < Ability->GetMaxStacks(Context))
		{*/
			return Ability->CanAbilityExecute(*Context);
		/*}
		else
		{
			return EAbleAbilityStartResult::PassiveMaxStacksReached;
		}*/
	}

	return Result;
}

EAbleAbilityStartResult UAbleAbilityComponent::ActivatePassiveAbility(UAbleAbilityContext* Context, bool ServerActivatedAbility)
{
	if (!Context)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Null Context passed to ActivatePassiveAbility method."));
		return EAbleAbilityStartResult::InternalSystemsError;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::ActivatePassiveAbility"), STAT_AbleAbilityComponent_ActivatePassiveAbility, STATGROUP_Able);

	EAbleAbilityStartResult Result = ServerActivatedAbility ? EAbleAbilityStartResult::Success : CanActivatePassiveAbility(Context);

	const UAbleAbility* Ability = Context->GetAbility();

	if (Result == EAbleAbilityStartResult::AsyncProcessing)
	{
        if (m_Settings->GetLogVerbose())
        {
            UE_LOG(LogAbleSP, Warning, TEXT("[%s] ActivatePassiveAbility [%s] queued for async"),
                *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetAbilityName()));
        }

		// Save and process it later once the Async query is done.
		m_AsyncContexts.AddUnique(Context);
		return Result;
	}
	else if (Result == EAbleAbilityStartResult::PassiveMaxStacksReached)
	{
		if (Ability->AlwaysRefreshDuration())
		{
			UAbleAbilityInstance** FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				(*FoundPassive)->ResetTime(Ability->RefreshLoopTimeOnly());

				if (Ability->ResetLoopCountOnRefresh())
				{
					(*FoundPassive)->SetCurrentIteration(0);
				}

                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAbleSP, Warning, TEXT("[%s] ActivatePassiveAbility [%s] reset time"),
                        *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetAbilityName()));
                }

				return EAbleAbilityStartResult::Success;
			}
		}
        else
        {
            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] ActivatePassiveAbility [%s] max stacks reached"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetAbilityName()));
            }
        }
	}
	else if (Result == EAbleAbilityStartResult::Success)
	{
		UAbleAbilityInstance** FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
		if (FoundPassive && Ability->CanStack())
		{
			UAbleAbilityContext& MutableContext = (*FoundPassive)->GetMutableContext();
			int32 StackIncrement = FMath::Max(Ability->GetStackIncrement(Context), 1);
			MutableContext.SetStackCount(MutableContext.GetCurrentStackCount() + StackIncrement);
			Ability->OnAbilityStackAddedBP(&MutableContext);

			if (Ability->RefreshDurationOnNewStack())
			{
				(*FoundPassive)->ResetTime(Ability->RefreshLoopTimeOnly());
			}

			if (Ability->ResetLoopCountOnRefresh())
			{
				(*FoundPassive)->SetCurrentIteration(0);
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] ActivatePassiveAbility [%s] stack added stack count %d (Increment %d)"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetAbilityName()),
                    MutableContext.GetCurrentStackCount(),
					StackIncrement);
            }
			// We created a new ability object , so we need to clean up the useless one.
			if (const UAbleAbility* DefaultAbility = Cast<UAbleAbility>(Ability->GetClass()->GetDefaultObject()))
			{
				Context->SetAbility(DefaultAbility);
				HandleInstanceCleanUp(*Ability);
			}
		}
		else
		{
			// New Instance
			UAbleAbilityInstance* NewInstance = NewObject<UAbleAbilityInstance>(this);
			NewInstance->Initialize(*Context);
			
			// We've passed all our checks, go ahead and allocate our Task scratch pads.
			Context->AllocateScratchPads();

			// make sure the ability knows a stack was added and *don't* use the OnStart to duplicate the stack added behavior
			Ability->OnAbilityStackAddedBP(Context);

			// Go ahead and start our cooldown.
			AddCooldownForAbility(*Ability, *Context);

            m_PassivesDirty |= true;

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] ActivatePassiveAbility [%s] Started"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetAbilityName()));
            }
			
			m_PassiveAbilityInstances.Add(NewInstance);
		}
	}
	else
	{
		if (m_Settings->GetLogAbilityFailures())
		{
			UE_LOG(LogAbleSP, Error, TEXT("[%s] Failed to play Passive Ability [%s] due to reason [%s]."), 
                *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetAbilityName()), 
                *FAbleSPLogHelper::GetResultEnumAsString(Result));
		}

		return Result;
	}

	CheckNeedsTick();

	return EAbleAbilityStartResult::Success;
}


void UAbleAbilityComponent::CancelAbility(const UAbleAbility* Ability, EAbleAbilityTaskResult ResultToUse)
{
	if (!Ability)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Null Ability passed to CancelAbility method."));
		return;
	}

	/*if (!IsAuthoritative())
	{
		ServerCancelAbility(Ability->GetAbilityNameHash(), ResultToUse);

		// Fall through and Locally simulate the cancel if we're player controlled.
		if (!IsOwnerLocallyControlled())
		{
			return;
		}
	}*/

	InternalCancelAbility(Ability, ResultToUse);
}

int32 UAbleAbilityComponent::GetCurrentStackCountForPassiveAbility(const UAbleAbility* Ability) const
{
	if (!Ability)
	{
		return 0;
	}

	UAbleAbilityInstance* const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
	if (FoundPassive)
	{
		return (*FoundPassive)->GetStackCount();
	}

	return 0;
}

void UAbleAbilityComponent::GetCurrentPassiveAbilities(TArray<UAbleAbility*>& OutPassives) const
{
	OutPassives.Empty();
	for (const UAbleAbilityInstance* ActivePassive : m_PassiveAbilityInstances)
	{
		const UAbleAbility& PassiveAbility = ActivePassive->GetAbility();
		// This is the devil, but BP doesn't like const pointers.
		OutPassives.Add(const_cast<UAbleAbility*>(&PassiveAbility));
	}
}

void UAbleAbilityComponent::SetPassiveStackCount(const UAbleAbility* Ability, int32 NewStackCount, bool ResetDuration, EAbleAbilityTaskResult ResultToUseOnCancel)
{
	if (!Ability)
	{
		return;
	}

	UAbleAbilityInstance* const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
	if (FoundPassive)
	{
		if (NewStackCount == 0)
		{
			// make sure to notify of "stack removal" first
			(*FoundPassive)->GetAbility().OnAbilityStackRemovedBP(&(*FoundPassive)->GetContext());

			// Just cancel the Passive->
			CancelAbility(Ability, ResultToUseOnCancel);
		}
		else
		{
			int32 current = (*FoundPassive)->GetStackCount();
			(*FoundPassive)->SetStackCount(NewStackCount);

			if (current > NewStackCount)
			{
				(*FoundPassive)->GetAbility().OnAbilityStackRemovedBP(&(*FoundPassive)->GetContext());
			}
			else
			{
				(*FoundPassive)->GetAbility().OnAbilityStackAddedBP(&(*FoundPassive)->GetContext());
			}

			if (ResetDuration)
			{
				(*FoundPassive)->ResetTime(Ability->RefreshLoopTimeOnly());
			}

			if (Ability->ResetLoopCountOnRefresh())
			{
				(*FoundPassive)->SetCurrentIteration(0);
			}
		}
	}

}

float UAbleAbilityComponent::GetAbilityCooldownRatio(const UAbleAbility* Ability) const
{
	if (Ability)
	{
		if (const FAbleAbilityCooldown* FoundCooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			return FoundCooldown->getTimeRatio();
		}
	}

	return 0.0f;
}

float UAbleAbilityComponent::GetAbilityCooldownTotal(const UAbleAbility* Ability) const
{
	if (Ability)
	{
		if (const FAbleAbilityCooldown* FoundCooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			return FoundCooldown->GetCooldownTime();
		}
	}

	return 0.0f;
}

void UAbleAbilityComponent::RemoveCooldown(const UAbleAbility* Ability)
{
	if (Ability)
	{
		m_ActiveCooldowns.Remove(Ability->GetAbilityNameHash());
	}
}

void UAbleAbilityComponent::SetCooldown(const UAbleAbility* Ability, UAbleAbilityContext* Context, float time)
{
	if (Ability)
	{
		if (time <= 0.0f)
		{
			RemoveCooldown(Ability);
		}

		if (FAbleAbilityCooldown* cooldown = m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()))
		{
			cooldown->SetCooldownTime(time);
		}
		else if (Context)
		{
			FAbleAbilityCooldown newCooldown(*Ability, *Context);
			m_ActiveCooldowns.Add(Ability->GetAbilityNameHash(), newCooldown);
		}
		else
		{
			UE_LOG(LogAbleSP, Warning, TEXT("Unable to find Cooldown for Ability [%s] and no Context was provided so one could not be added."), *Ability->GetAbilityName());
		}
	}
}

float UAbleAbilityComponent::GetAbilityCurrentTime(const UAbleAbility* Ability) const
{
	if (Ability)
	{
		if (Ability->IsPassive())
		{
			UAbleAbilityInstance* const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				return (*FoundPassive)->GetCurrentTime();
			}
		}
		else if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			return m_ActiveAbilityInstance->GetCurrentTime();
		}
	}

	return 0.0f;
}

float UAbleAbilityComponent::GetAbilityCurrentTimeRatio(const UAbleAbility* Ability) const
{
	if (Ability)
	{
		if (Ability->IsPassive())
		{
			UAbleAbilityInstance* const * FoundPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash()));
			if (FoundPassive)
			{
				return (*FoundPassive)->GetCurrentTimeRatio();
			}
		}
		else if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == Ability->GetAbilityNameHash())
		{
			return m_ActiveAbilityInstance->GetCurrentTimeRatio();
		}
	}

	return 0.0f;
}

void UAbleAbilityComponent::QueueContext(UAbleAbilityContext* Context, EAbleAbilityTaskResult ResultToUse)
{
	if (IsAuthoritative() || IsOwnerLocallyControlled())
	{
		m_PendingContext.Add(Context); 
		m_PendingResult.Add(ResultToUse);

		CheckNeedsTick();
	}
}

void UAbleAbilityComponent::AddAdditionTargetsToContext(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool AllowDuplicates /*= false*/, bool ClearTargets/* = false*/)
{
	check(Context.IsValid());
	const uint32 AbilityNameHash = Context->GetAbility()->GetAbilityNameHash();
	if (!Context->GetAbility()->IsPassive())
	{
		if (m_ActiveAbilityInstance)
		{
			if (m_ActiveAbilityInstance->GetAbilityNameHash() == AbilityNameHash)
			{
				m_ActiveAbilityInstance->AddAdditionalTargets(AdditionalTargets, AllowDuplicates, ClearTargets);
			}
		}
	}
	else
	{
		for (UAbleAbilityInstance* Passive : m_PassiveAbilityInstances)
		{
			if (Passive && Passive->IsValid())
			{
				if (Passive->GetAbilityNameHash() == AbilityNameHash)
				{
					Passive->AddAdditionalTargets(AdditionalTargets, AllowDuplicates, ClearTargets);
					break;
				}
			}
		}
	}
}

void UAbleAbilityComponent::ModifyContext(const TWeakObjectPtr<const UAbleAbilityContext>& Context, AActor* Instigator, AActor* Owner, const FVector& TargetLocation, const TArray<TWeakObjectPtr<AActor>>& AdditionalTargets, bool ClearTargets /*= false*/)
{
	check(Context.IsValid());
	const uint32 AbilityNameHash = Context->GetAbility()->GetAbilityNameHash();
	if (!Context->GetAbility()->IsPassive())
	{
		if (m_ActiveAbilityInstance)
		{
			if (m_ActiveAbilityInstance->GetAbilityNameHash() == AbilityNameHash)
			{
				m_ActiveAbilityInstance->ModifyContext(Instigator, Owner, TargetLocation, AdditionalTargets, ClearTargets);
			}
		}
	}
	else
	{
		for (UAbleAbilityInstance* Passive : m_PassiveAbilityInstances)
		{
			if (Passive && Passive->IsValid())
			{
				if (Passive->GetAbilityNameHash() == AbilityNameHash)
				{
					Passive->ModifyContext(Instigator, Owner, TargetLocation, AdditionalTargets, ClearTargets);
					break;
				}
			}
		}
	}
}

void UAbleAbilityComponent::CheckNeedsTick()
{
	// We need to tick if we...
	bool NeedsTick = m_ActiveAbilityInstance || // Have an active ability...
        m_PassivesDirty || // Have pending dirty passives...
        m_PassiveAbilityInstances.Num() || // Have any passive abilities...
		m_ActiveCooldowns.Num() || // Have active cooldowns...
		m_AsyncContexts.Num() || // Have Async targeting to process...
		m_PendingContext.Num() || // We have a pending context...
		m_PendingCancels.Num();  // We have a pending cancel...

	PrimaryComponentTick.SetTickFunctionEnable(NeedsTick);
}

EAbleAbilityStartResult UAbleAbilityComponent::InternalStartAbility(UAbleAbilityContext* Context, bool ServerActivatedAbility)
{
	if (!Context)
	{
		return EAbleAbilityStartResult::InvalidParameter;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::InternalStartAbility"), STAT_AbleAbilityComponent_InternalStartAbility, STATGROUP_Able);
	
	EAbleAbilityStartResult Result = EAbleAbilityStartResult::InvalidParameter;
	if (Context->GetAbility())
	{
		HandleInstanceLogic(Context);

		// Hand off for Passive processing.
		if (Context->GetAbility()->IsPassive())
		{
			return ActivatePassiveAbility(Context, ServerActivatedAbility);
		}

		Result = ServerActivatedAbility ? EAbleAbilityStartResult::Success : CanActivateAbility(Context);

		if (Result == EAbleAbilityStartResult::CannotInterruptCurrentAbility && (m_ActiveAbilityInstance->IsIterationDone() && m_ActiveAbilityInstance->IsDone()))
		{
			CancelActiveAbility(EAbleAbilityTaskResult::Successful);
			Result = CanActivateAbility(Context); // Try again.
		}

		if (Result == EAbleAbilityStartResult::AsyncProcessing)
		{
			// Save and process it later once the Async query is done.
			m_AsyncContexts.AddUnique(Context);

			return Result;
		}
	}

	if (Result != EAbleAbilityStartResult::Success)
	{
		if (m_Settings->GetLogAbilityFailures())
		{
			UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility Failed to play Ability [%s] due to reason [%s]"),
                *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                *(Context->GetAbility()->GetAbilityName()), 
                *FAbleSPLogHelper::GetResultEnumAsString(Result));
		}
		HandleInstanceCleanUp(*Context->GetAbility());
		return Result;
	}

	// Interrupt any current abilities.
	if (m_ActiveAbilityInstance)
	{
		if (!m_ActiveAbilityInstance->IsIterationDone())
		{
			if (IsProcessingUpdate())
			{
                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility [%s] queued to interrupt [%s]"),
                        *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetAbilityName()),
                        *m_ActiveAbilityInstance->GetAbility().GetAbilityName());
                }

				// Delay processing till next frame.
				m_PendingCancels.Add(FAblePendingCancelContext(m_ActiveAbilityInstance->GetAbilityNameHash(), EAbleAbilityTaskResult::Interrupted));
				// Delay processing till next frame.
				QueueContext(Context, EAbleAbilityTaskResult::Successful);
				return Result;
			}

            if (m_Settings->GetLogVerbose() && Context && Context->GetAbility())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility [%s] interrupted [%s]"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetAbilityName()),
                    *m_ActiveAbilityInstance->GetAbility().GetAbilityName());
            }

			m_IsProcessingUpdate = true;
			m_ActiveAbilityInstance->InterruptAbility();
			m_IsProcessingUpdate = false;
		}
		else
		{
			if (IsProcessingUpdate())
			{
                if (m_Settings->GetLogVerbose() && Context && Context->GetAbility())
                {
                    UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility [%s] queued for next frame"),
                        *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *(Context->GetAbility()->GetAbilityName()));
                }

				// Delay processing till next frame.
				QueueContext(Context, EAbleAbilityTaskResult::Successful);
				return Result;
			}

            if (m_Settings->GetLogVerbose() && Context && Context->GetAbility())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility [%s] Ending Current Ability [%s]"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *(Context->GetAbility()->GetAbilityName()),
                    *m_ActiveAbilityInstance->GetAbility().GetAbilityName());
            }

			m_IsProcessingUpdate = true;
			m_ActiveAbilityInstance->FinishAbility();
			m_IsProcessingUpdate = false;
		}
		HandleInstanceCleanUp(m_ActiveAbilityInstance->GetAbility());
		m_ActiveAbilityInstance->Reset();
		m_ActiveAbilityInstance = nullptr;
	}

    if (m_Settings->GetLogVerbose())
    {
        UE_LOG(LogAbleSP, Warning, TEXT("[%s] InternalStartAbility [%s] Started"),
            *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
            *(GetNameSafe(Context->GetAbility())));
    }
	
	UAbleAbilityInstance* NewInstance = NewObject<UAbleAbilityInstance>(this);
	NewInstance->Initialize(*Context);

	// We've passed all our checks, go ahead and allocate our Task scratch pads.
	Context->AllocateScratchPads();

	m_ActiveAbilityInstance = NewInstance;

	// Go ahead and start our cooldown.
	AddCooldownForAbility(*(Context->GetAbility()), *Context);

	if (IsNetworked() && IsAuthoritative())
	{
		// Propagate changes to client.
		UpdateServerActiveAbility();
	}

	CheckNeedsTick();

	return Result;
}

void UAbleAbilityComponent::InternalCancelAbility(const UAbleAbility* Ability, EAbleAbilityTaskResult ResultToUse)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::InternalCancelAbility"), STAT_AbleAbilityComponent_InternalCancelAbility, STATGROUP_Able);

	if (Ability && IsProcessingUpdate())
	{
		// Mid update, let's defer this.
		m_PendingCancels.Add(FAblePendingCancelContext(Ability->GetAbilityNameHash(), ResultToUse));
		return;
	}

	if (GetActiveAbility() && Ability->GetAbilityNameHash() == GetActiveAbility()->GetAbilityNameHash())
	{
		CancelActiveAbility(ResultToUse);
	}
	else if (Ability && Ability->IsPassive())
	{
		for (int i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
		{
			if (m_PassiveAbilityInstances[i] && m_PassiveAbilityInstances[i]->IsValid() && m_PassiveAbilityInstances[i]->GetAbilityNameHash() == Ability->GetAbilityNameHash())
			{
                if (m_Settings->GetLogVerbose())
                {
                    UE_LOG(LogAbleSP, Warning, TEXT("[%s] CancelPassiveAbility( %s ) Result [%s]"),
                        *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                        *m_PassiveAbilityInstances[i]->GetAbility().GetAbilityName(),
                        *FAbleSPLogHelper::GetTaskResultEnumAsString(ResultToUse));
                }

				m_PassiveAbilityInstances[i]->FinishAbility();
				HandleInstanceCleanUp(m_PassiveAbilityInstances[i]->GetAbility());
				m_PassiveAbilityInstances.RemoveAt(i);

                m_PassivesDirty |= true;
				break;
			}
		}
	}
}

void UAbleAbilityComponent::AddCooldownForAbility(const UAbleAbility& Ability, const UAbleAbilityContext& Context)
{
	if (Ability.GetCooldown(&Context) > 0.0f)
	{
		m_ActiveCooldowns.Add(Ability.GetAbilityNameHash(), FAbleAbilityCooldown(Ability, Context));
	}
}

bool UAbleAbilityComponent::IsAbilityOnCooldown(const UAbleAbility* Ability) const
{
	return Ability ? m_ActiveCooldowns.Find(Ability->GetAbilityNameHash()) != nullptr : false;
}

bool UAbleAbilityComponent::IsPassiveActive(const UAbleAbility* Ability) const
{
	if (Ability && Ability->IsPassive())
	{
		return m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(Ability->GetAbilityNameHash())) != nullptr;
	}

	return false;
}

void UAbleAbilityComponent::InternalUpdateAbility(UAbleAbilityInstance* AbilityInstance, float DeltaTime)
{
	if (AbilityInstance)
	{
		if (!AbilityInstance->PreUpdate())
		{
			InternalCancelAbility(&AbilityInstance->GetAbility(), EAbleAbilityTaskResult::Successful);
			return;
		}

		if (!AbilityInstance->CheckForceDependency())
		{
			return;
		}

		AbilityInstance->SyncUpdate(DeltaTime);
	}
}

void UAbleAbilityComponent::UpdateCooldowns(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AbleAbilityComponent::UpdateCooldowns"), STAT_AbleAbilityComponent_UpdateCooldowns, STATGROUP_Able);
	for (auto ItUpdate = m_ActiveCooldowns.CreateIterator(); ItUpdate; ++ItUpdate)
	{
		FAbleAbilityCooldown& AbilityCooldown = ItUpdate->Value;

		AbilityCooldown.Update(DeltaTime);
		if (AbilityCooldown.IsComplete())
		{
			ItUpdate.RemoveCurrent();
		}
	}
}

void UAbleAbilityComponent::HandlePendingContexts()
{
	check(m_PendingContext.Num() == m_PendingResult.Num());

	for (int i = 0; i < m_PendingContext.Num(); ++i)
	{
		if (const UAbleAbility* PendingAbility = m_PendingContext[i]->GetAbility())
		{
			if (!PendingAbility->IsPassive())
			{
				EAbleAbilityStartResult PendingStart = CanActivateAbility(m_PendingContext[i]);
				// Only process the pending context if we're going to succeed, or it's a branch and we know the target and cooldown are okay.
				bool validPendingContext = (PendingStart == EAbleAbilityStartResult::Success);
				validPendingContext |= (m_PendingResult[i].GetValue() == EAbleAbilityTaskResult::Branched &&
										(PendingStart != EAbleAbilityStartResult::CooldownNotExpired && PendingStart != EAbleAbilityStartResult::InvalidTarget));

				if (!validPendingContext)
				{
					continue;
				}

				// If we're about to start an Active ability, we need to first cancel any current active.
				if (m_ActiveAbilityInstance)
				{
					switch (m_PendingResult[i].GetValue())
					{
					case EAbleAbilityTaskResult::Interrupted:
						m_ActiveAbilityInstance->InterruptAbility();
						break;
					case EAbleAbilityTaskResult::Successful:
						m_ActiveAbilityInstance->FinishAbility();
						break;
					case EAbleAbilityTaskResult::Branched:
						m_ActiveAbilityInstance->BranchAbility();
						break;
					}

                    if (m_Settings->GetLogVerbose())
                    {
                        UE_LOG(LogAbleSP, Warning, TEXT("[%s] HandlePendingContexts( %s ) Result [%s]"),
                            *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                            *FAbleSPLogHelper::GetTaskResultEnumAsString(m_PendingResult[i].GetValue()),
                            *m_ActiveAbilityInstance->GetAbility().GetAbilityName());
                    }

					HandleInstanceCleanUp(m_ActiveAbilityInstance->GetAbility());

					m_ActiveAbilityInstance->Reset();
					m_ActiveAbilityInstance = nullptr;
					m_ActiveAbilityResult = m_PendingResult[i].GetValue();
				}
			}

			InternalStartAbility(m_PendingContext[i]);
		}
	}

	m_PendingContext.Empty();
	m_PendingResult.Empty();
}

void UAbleAbilityComponent::HandlePendingCancels()
{
	if (m_PendingCancels.Num() == 0) return;
	
	m_PendingCancelHandles = TArray<FAblePendingCancelContext>(m_PendingCancels);
	m_IsProcessingUpdate = true;
	m_PendingCancelNameHashes = TArray<uint32>();
	
	for (FAblePendingCancelContext& CancelContext : m_PendingCancelHandles)
	{
		if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == CancelContext.GetNameHash())
		{
			switch (CancelContext.GetResult())
			{
			case EAbleAbilityTaskResult::Interrupted:
				m_ActiveAbilityInstance->InterruptAbility();
				break;
			case EAbleAbilityTaskResult::Successful:
				m_ActiveAbilityInstance->FinishAbility();
				break;
			case EAbleAbilityTaskResult::Branched:
				m_ActiveAbilityInstance->BranchAbility();
				break;
			}

            if (m_Settings->GetLogVerbose())
            {
                UE_LOG(LogAbleSP, Warning, TEXT("[%s] HandlePendingCancels( %s ) Result [%s]"),
                    *FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
                    *FAbleSPLogHelper::GetTaskResultEnumAsString(CancelContext.GetResult()),
                    *m_ActiveAbilityInstance->GetAbility().GetAbilityName());
            }
			
			HandleInstanceCleanUp(m_ActiveAbilityInstance->GetAbility());

			m_ActiveAbilityInstance->Reset();
			m_ActiveAbilityInstance = nullptr;
			m_ActiveAbilityResult = CancelContext.GetResult();
			m_PendingCancelNameHashes.Add(CancelContext.GetNameHash());
		}
		else
		{
			for (int32 i = 0; i < m_PassiveAbilityInstances.Num(); ++i)
			{
				if (m_PassiveAbilityInstances[i] && m_PassiveAbilityInstances[i]->GetAbilityNameHash() == CancelContext.GetNameHash())
				{
					if (m_PassiveAbilityInstances[i]->IsValid())
					{
						m_PassiveAbilityInstances[i]->FinishAbility();
					}

					HandleInstanceCleanUp(m_PassiveAbilityInstances[i]->GetAbility());
					m_PassiveAbilityInstances.RemoveAt(i);
					m_PassivesDirty |= true;
					m_PendingCancelNameHashes.Add(CancelContext.GetNameHash());
					break;
				}
			}
		}
	}
	
	m_IsProcessingUpdate = false;
	
	m_PendingCancels.RemoveAll([&](const FAblePendingCancelContext& Context){
		return m_PendingCancelNameHashes.Contains(Context.GetNameHash());
	});
	m_PendingCancelNameHashes.Empty();
	m_PendingCancelHandles.Empty();
}

void UAbleAbilityComponent::HandleInstanceLogic(UAbleAbilityContext* Context)
{
	check(Context->GetAbility());
	UAbleAbility* NewInstance = NewAbilityObject(Context);
	check(NewInstance);
	
#if WITH_EDITOR
	// BlueprintGeneratedClass.cpp has a bug when the array size of the CDO doesn't match the array size of the Template - this can only happen in Editor builds. So, fix that here.
	NewInstance->ValidateDataAgainstTemplate(Context->GetAbility());
#endif
	m_CreatedAbilityInstances.Add(NewInstance);
	Context->SetAbility(NewInstance);
	
	/*switch (Context->GetAbility()->GetInstancePolicy())
	{
		case EAbleInstancePolicy::NewInstance:
		case EAbleInstancePolicy::NewInstanceReplicated:
		{
			check(Context->GetAbility()->GetInstancePolicy() == EAbleInstancePolicy::NewInstance || IsAuthoritative());
			UAbleAbility* NewInstance = NewObject<UAbleAbility>(GetOwner(), Context->GetAbility()->GetClass(), NAME_None, RF_Transient, Cast<UObject>(Context->GetAbilityBP()));
			check(NewInstance);

#if WITH_EDITOR
			// BlueprintGeneratedClass.cpp has a bug when the array size of the CDO doesn't match the array size of the Template - this can only happen in Editor builds. So, fix that here.
			NewInstance->ValidateDataAgainstTemplate(Context->GetAbility());
#endif
			m_CreatedAbilityInstances.Add(NewInstance);
			Context->SetAbility(NewInstance);
		}
		break;
		case EAbleInstancePolicy::Default:
		default:
			break;
	} */
}

UAbleAbility* UAbleAbilityComponent::NewAbilityObject(const UAbleAbilityContext* Context) const
{
	// UAbleAbility* NewInstance = NewObject<UAbleAbility>(GetOwner(), Context->GetAbility()->GetClass(), NAME_None, RF_Transient, Cast<UObject>(Context->GetAbilityBP()));
	UAbleAbility* NewInstance = NewObject<UAbleAbility>(GetOwner(), Context->GetAbility()->GetClass(), NAME_None, RF_Transient, Cast<UObject>(Context->GetAbilityBP()));
	NewInstance->Process(Context->GetAbilityBP());

	return NewInstance;
}

void UAbleAbilityComponent::HandleInstanceCleanUp(const UAbleAbility& Ability)
{
	const UAbleAbility* LHS = &Ability;
	// The things I do to avoid a const_cast...
	UAbleAbility** HeldAbility = m_CreatedAbilityInstances.FindByPredicate([&](UAbleAbility* RHS)
	{
		return LHS == RHS;
	});

	if (HeldAbility)
	{
		UAbleAbility* AbilityToRemove = *HeldAbility;
		m_CreatedAbilityInstances.Remove(AbilityToRemove);
		AbilityToRemove->CleanUp();
		AbilityToRemove->MarkPendingKill();
	}
	
	/*switch (Ability.GetInstancePolicy())
	{
	case EAbleInstancePolicy::NewInstance:
	case EAbleInstancePolicy::NewInstanceReplicated:
	{
		const UAbleAbility* LHS = &Ability;

		// Only Server can destroy Replicated objects.
		if (LHS->GetInstancePolicy() == EAbleInstancePolicy::NewInstanceReplicated && !IsAuthoritative())
		{
			break;
		}

		// The things I do to avoid a const_cast...
		UAbleAbility** HeldAbility = m_CreatedAbilityInstances.FindByPredicate([&](UAbleAbility* RHS)
		{
			return LHS == RHS;
		});

		if (HeldAbility)
		{
			UAbleAbility* AbilityToRemove = *HeldAbility;
			m_CreatedAbilityInstances.Remove(AbilityToRemove);
			AbilityToRemove->MarkPendingKill();
		}
	}
	break;
	case EAbleInstancePolicy::Default:
	default:
		break;
	}*/
}

void UAbleAbilityComponent::ValidateRemoteRunningAbilities()
{
	if (IsOwnerLocallyControlled() || IsAuthoritative())
	{
		return;
	}

	// Validate the Active Ability.
	if (m_ServerActive.IsValid())
	{
		if (!m_ActiveAbilityInstance || m_ServerActive.GetAbility()->GetAbilityNameHash() != m_ActiveAbilityInstance->GetAbilityNameHash())
		{
			// Server has an Active, but we aren't playing anything (or we're playing the wrong thing some how).
			InternalStartAbility(UAbleAbilityContext::MakeContext(m_ServerActive), true);
		}
	}

	// Validate Passive Abilities.
	for (const FAbleAbilityNetworkContext& PassiveContext : m_ServerPassiveAbilities)
	{
		if (PassiveContext.IsValid())
		{
			if (!IsPassiveActive(PassiveContext.GetAbility().Get()))
			{
				// Server has a Passive, but we aren't playing it. Fix that.
				InternalStartAbility(UAbleAbilityContext::MakeContext(PassiveContext), true);
			}
		}
	}
}

bool UAbleAbilityComponent::IsOwnerLocallyControlled() const
{
	//if (GetNetMode() == ENetMode::NM_ListenServer)
	//{
	//	return true;
	//}

	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		return PawnOwner->IsLocallyControlled();
	}

	return false;
}

bool UAbleAbilityComponent::IsAuthoritative() const
{
	return !IsNetworked() || (IsNetworked() && GetOwnerRole() == ROLE_Authority);
}

bool UAbleAbilityComponent::AbilityClientPolicyAllowsExecution(const UAbleAbility* Ability, bool IsPrediction) const
{
	if (!Ability)
	{
		return false;
	}

	switch (Ability->GetClientPolicy())
	{
		default:
		case EAbleClientExecutionPolicy::Default:
		{
			// Remote, Client, or Authoritative
			if (IsPrediction)
			{
				return IsOwnerLocallyControlled() || IsAuthoritative();
			}

			return true;
		}
		break;
		case EAbleClientExecutionPolicy::LocalAndAuthoritativeOnly:
		{
			return IsAuthoritative() || IsOwnerLocallyControlled();
		}
		break;
	}

	return false;
}

EAbleAbilityStartResult UAbleAbilityComponent::BranchAbility(UAbleAbilityContext* Context)
{
	EAbleAbilityStartResult Result = EAbleAbilityStartResult::InternalSystemsError;
	if (!IsAuthoritative())
	{
		FAbleAbilityNetworkContext AbilityNetworkContext(*Context);
		ServerBranchAbility(AbilityNetworkContext);

		if (IsOwnerLocallyControlled())
		{
			// Track for prediction.
			AddLocallyPredictedAbility(AbilityNetworkContext);

			QueueContext(Context, EAbleAbilityTaskResult::Branched);
			Result = EAbleAbilityStartResult::Success;
		}
		else
		{
			Result = EAbleAbilityStartResult::ForwardedToServer;
		}
	}
	else
	{
		QueueContext(Context, EAbleAbilityTaskResult::Branched);
		Result = EAbleAbilityStartResult::Success;
	}

	CheckNeedsTick();

	return Result;
}

bool UAbleAbilityComponent::IsAbilityRealmAllowed(const UAbleAbility& Ability) const
{
	if (!IsNetworked())
	{
		return true;
	}

	switch (Ability.GetAbilityRealm())
	{
		case EAbleAbilityTaskRealm::ATR_Client:
		{
			return !IsAuthoritative() || IsOwnerLocallyControlled() || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer;
		}
		break;
		case EAbleAbilityTaskRealm::ATR_Server:
		{
			return IsAuthoritative();
		}
		break;
		case EAbleAbilityTaskRealm::ATR_ClientAndServer:
		{
			return true;
		}
		break;
		default:
			break;
	}

	return false;
}

void UAbleAbilityComponent::UpdateServerPassiveAbilities()
{
	check(IsAuthoritative()); // Should only be called on the server.

	m_ServerPassiveAbilities.RemoveAll([&](const FAbleAbilityNetworkContext& Passive)
	{
		return !Passive.GetAbility().IsValid();
	});

	TArray<uint32> Whitelist;
	for (const UAbleAbilityInstance* PassiveInstance : m_PassiveAbilityInstances)
	{
		if (PassiveInstance && PassiveInstance->IsValid())
		{
			Whitelist.Add(PassiveInstance->GetAbilityNameHash());
			if (FAbleAbilityNetworkContext* ExistingNetworkContext = m_ServerPassiveAbilities.FindByPredicate(FAbleFindAbilityNetworkContextByHash(PassiveInstance->GetAbilityNameHash())))
			{
				ExistingNetworkContext->SetCurrentStacks(PassiveInstance->GetStackCount());
			}
			else
			{
				// New Passive->
				m_ServerPassiveAbilities.Add(FAbleAbilityNetworkContext(PassiveInstance->GetContext()));
			}
		}
	}

	m_ServerPassiveAbilities.RemoveAll(FAbleAbilityNetworkContextWhiteList(Whitelist));
}

void UAbleAbilityComponent::UpdateServerActiveAbility()
{
	check(IsAuthoritative()); // Should only be called on the server.
	
	if (!m_ActiveAbilityInstance)
	{
		m_ServerActive.Reset();
	}
	else if(!m_ServerActive.IsValid() || m_ServerActive.GetAbility()->GetAbilityNameHash() != m_ActiveAbilityInstance->GetAbilityNameHash())
	{
		m_ServerActive = FAbleAbilityNetworkContext(m_ActiveAbilityInstance->GetContext(), m_ActiveAbilityResult.GetValue());
	}
	else if (m_ServerActive.IsValid() && m_ServerActive.GetAbility()->GetAbilityNameHash() == m_ActiveAbilityInstance->GetAbilityNameHash())
	{
		FAbleAbilityNetworkContext forcedContext(m_ActiveAbilityInstance->GetContext());
		// ClientForceAbility(forcedContext);
	}
}

void UAbleAbilityComponent::ServerActivateAbility_Implementation(const FAbleAbilityNetworkContext& Context)
{
	UAbleAbilityContext* LocalContext = UAbleAbilityContext::MakeContext(Context);
	if (!LocalContext) return;
	
	if (InternalStartAbility(LocalContext) == EAbleAbilityStartResult::Success)
	{
		if (!Context.GetAbility()->IsPassive())
		{
			m_ServerActive = FAbleAbilityNetworkContext::UpdateNetworkContext(*LocalContext, Context);
		}
		else
		{
			int32 StackCount = GetCurrentStackCountForPassiveAbility(Context.GetAbility().Get());
			if (FAbleAbilityNetworkContext* ExistingContext = m_ServerPassiveAbilities.FindByPredicate(FAbleFindAbilityNetworkContextByHash(Context.GetAbility()->GetAbilityNameHash())))
			{
				ExistingContext->SetCurrentStacks(StackCount);
			}
			else
			{
				FAbleAbilityNetworkContext NewNetworkContext(*LocalContext);
				NewNetworkContext.SetCurrentStacks(StackCount);
				m_ServerPassiveAbilities.Add(NewNetworkContext);
			}
		}
	}
}

bool UAbleAbilityComponent::ServerActivateAbility_Validate(const FAbleAbilityNetworkContext& Context)
{
	// Nothing really to check here since the server isn't trusting the Client at all.
	return Context.IsValid();
}

void UAbleAbilityComponent::ServerCancelAbility_Implementation(uint32 AbilityNameHash, EAbleAbilityTaskResult ResultToUse)
{
	const UAbleAbility* Ability = nullptr;
	const UAbleAbilityContext* Context = nullptr;
	if (m_ActiveAbilityInstance && m_ActiveAbilityInstance->GetAbilityNameHash() == AbilityNameHash)
	{
		Ability = &m_ActiveAbilityInstance->GetAbility();
		Context = &m_ActiveAbilityInstance->GetContext();
	}
	else
	{
		if (UAbleAbilityInstance** PassiveInstance = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(AbilityNameHash)))
		{
			Ability = &(*PassiveInstance)->GetAbility();
			Context = &(*PassiveInstance)->GetContext();
		}
	}

	if (Ability && Context)
	{
		// We have a cancel request, but our Server hasn't canceled the Ability. Verify the client is allowed to cancel it.
		if (Ability->CanClientCancelAbilityBP(Context))
		{
			// Just pass it along.
			InternalCancelAbility(Ability, ResultToUse);
		}
		else if(m_Settings->GetLogVerbose())
		{
			UE_LOG(LogAbleSP, Warning, TEXT("[%s] ServerCancelAbility ignored because Ability [%s] CanClientCancelAbility returned false."),
				*FAbleSPLogHelper::GetWorldName(GetOwner()->GetWorld()),
				*Ability->GetAbilityName());
		}
	}

}

void UAbleAbilityComponent::ServerBranchAbility_Implementation(const FAbleAbilityNetworkContext& Context)
{
	check(IsAuthoritative());

	QueueContext(UAbleAbilityContext::MakeContext(Context), EAbleAbilityTaskResult::Branched);
}

bool UAbleAbilityComponent::ServerBranchAbility_Validate(const FAbleAbilityNetworkContext& Context)
{
	return Context.IsValid();
}

void UAbleAbilityComponent::ClientForceAbility_Implementation(const FAbleAbilityNetworkContext& Context)
{
	if (GetOwner() && GetOwner()->GetNetMode() == ENetMode::NM_Client)
	{
		InternalStartAbility(UAbleAbilityContext::MakeContext(Context), true);
	}
}

void UAbleAbilityComponent::ServerBranchSegment_Implementation(int32 SegmentIndex,
	const FJumpSegmentSetting& JumpSetting)
{
	check(IsAuthoritative());

	if (!m_ActiveAbilityInstance)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("ServerBranchSegment failed, m_ActiveAbilityInstance invalid !"));
		return;
	}
	
	BranchSegment(&m_ActiveAbilityInstance->GetContext(), SegmentIndex, JumpSetting);
}

void UAbleAbilityComponent::OnServerActiveAbilityChanged()
{
	if (m_ServerActive.IsValid() && 
		AbilityClientPolicyAllowsExecution(m_ServerActive.GetAbility().Get()))
	{
		if (WasLocallyPredicted(m_ServerActive))
		{
			// Same Ability, skip it since we were locally predicting it.
			return;
		}

		if (IsPlayingAbility())
		{
			// TODO: Should the client care about server based interrupt/branches?
			InternalCancelAbility(GetActiveAbility(), EAbleAbilityTaskResult::Successful);
		}

		InternalStartAbility(UAbleAbilityContext::MakeContext(m_ServerActive), true);
	}
	else
	{
		if (IsPlayingAbility())
		{
			InternalCancelAbility(GetActiveAbility(), m_ServerActive.GetResult());
		}
	}
}

void UAbleAbilityComponent::OnServerPassiveAbilitiesChanged()
{
	TArray<uint32> ValidAbilityNameHashes;

	m_PassiveAbilityInstances.RemoveAll([&](const UAbleAbilityInstance* Instance)
	{
		return !IsValid(&Instance->GetAbility());
	});
	
	for (const FAbleAbilityNetworkContext& ServerPassive : m_ServerPassiveAbilities)
	{
		if (ServerPassive.IsValid() && 
			ServerPassive.GetAbility().IsValid() && 
			AbilityClientPolicyAllowsExecution(ServerPassive.GetAbility().Get()))
		{
			if (UAbleAbilityInstance* const * CurrentPassive = m_PassiveAbilityInstances.FindByPredicate(FAbleFindAbilityInstanceByHash(ServerPassive.GetAbility()->GetAbilityNameHash())))
			{
				// Just make sure our stack count is accurate.
				(*CurrentPassive)->SetStackCount(ServerPassive.GetCurrentStack());
			}
			else // New Passive Ability
			{
				if (!WasLocallyPredicted(ServerPassive))
				{
					ActivatePassiveAbility(UAbleAbilityContext::MakeContext(ServerPassive));
				}
			}

			ValidAbilityNameHashes.Add(ServerPassive.GetAbility()->GetAbilityNameHash());
		}
	}

	m_PassiveAbilityInstances.RemoveAll(FAbleAbilityInstanceWhiteList(ValidAbilityNameHashes));
    m_PassivesDirty |= true;
}

/*void UAbleAbilityComponent::OnServerPredictiveKeyChanged()
{
	// Update our client key to use the latest key from the server.
	m_ClientPredictionKey = m_ServerPredictionKey;
}*/

#if WITH_EDITOR

void UAbleAbilityComponent::PlayAbilityFromEditor(const UAbleAbility* Ability, const FName EntrySegmentName, AActor* ForceTarget)
{
	// Interrupt any current abilities.
	if (m_ActiveAbilityInstance)
	{
		m_ActiveAbilityInstance->StopAbility();
		m_ActiveAbilityInstance->Reset();
		m_ActiveAbilityInstance = nullptr;
	}

	if (!Ability)
	{
		return;
	}

	UAbleAbilityContext* FakeContext = UAbleAbilityContext::MakeContext(Ability, this, GetOwner(), nullptr);
	FakeContext->SetActiveSegmentIndex(Ability->FindSegmentIndexByFName(EntrySegmentName));

	// Run Targeting so we can visually see it - we don't care about the result since the editor always succeeds.
	Ability->CanAbilityExecute(*FakeContext);

	// Force our Target, if we have one.
	if (ForceTarget)
	{
		FakeContext->GetMutableTargetActors().Add(ForceTarget);
	}

	UAbleAbilityInstance* NewInstance = NewObject<UAbleAbilityInstance>(this);
	NewInstance->Initialize(*FakeContext);

	// We've passed all our checks, go ahead and allocate our Task scratch pads.
	FakeContext->AllocateScratchPads();

	m_ActiveAbilityInstance = NewInstance;

	CheckNeedsTick();
}

float UAbleAbilityComponent::GetCurrentAbilityTime() const
{
	if (m_ActiveAbilityInstance)
	{
		return m_ActiveAbilityInstance->GetCurrentTime();
	}

	return 0.0f;
}

void UAbleAbilityComponent::SetAbilityPreviewTime(float NewTime)
{
	if (m_ActiveAbilityInstance)
	{
		m_ActiveAbilityInstance->SetPreviewTime(NewTime);
	}
}

void UAbleAbilityComponent::SetAbilityTime(float NewTime)
{
	if (m_ActiveAbilityInstance)
	{
		m_ActiveAbilityInstance->SetCurrentTime(NewTime);
	}
}

int UAbleAbilityComponent::GetCurrentSegmentIndex() const
{
	if (m_ActiveAbilityInstance)
	{
		return m_ActiveAbilityInstance->GetActiveSegmentIndex();
	}
	return 0;
}

#endif

void UAbleAbilityComponent::AddTag(const FGameplayTag Tag)
{
	m_TagContainer.AddTag(Tag);
}

void UAbleAbilityComponent::RemoveTag(const FGameplayTag Tag)
{
	m_TagContainer.RemoveTag(Tag);
}

bool UAbleAbilityComponent::HasTag(const FGameplayTag Tag, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasTag(Tag);
}

bool UAbleAbilityComponent::MatchesAnyTag(const FGameplayTagContainer Container, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasAny(Container);
}

bool UAbleAbilityComponent::MatchesAllTags(const FGameplayTagContainer Container, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    return CombinedTags.HasAll(Container);
}

bool UAbleAbilityComponent::CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);

    if (!ExcludesAny.IsEmpty())
    {
        if (CombinedTags.HasAny(ExcludesAny))
            return false;
    }

    if (!IncludesAny.IsEmpty())
    {
        if (!CombinedTags.HasAny(IncludesAny))
            return false;
    }

    if (!IncludesAll.IsEmpty())
    {
        if (!CombinedTags.HasAll(IncludesAll))
            return false;
    }
    return true;
}

void UAbleAbilityComponent::GetCombinedGameplayTags(FGameplayTagContainer& CombinedTags, bool includeExecutingAbilities) const
{
    CombinedTags = FGameplayTagContainer::EmptyContainer;
    CombinedTags.AppendTags(m_TagContainer);

    if (includeExecutingAbilities)
    {
        if (m_ActiveAbilityInstance)
        {
            CombinedTags.AppendTags(m_ActiveAbilityInstance->GetAbility().GetAbilityTagContainer());
        }

        for (const UAbleAbilityInstance* Passive : m_PassiveAbilityInstances)
        {
            if (Passive && Passive->IsValid())
            {
                CombinedTags.AppendTags(Passive->GetAbility().GetAbilityTagContainer());
            }
        }
    }
}

bool UAbleAbilityComponent::IsAbilityInstancePendingCancel(const UAbleAbilityInstance* AbilityInstance) const
{
	if (!AbilityInstance || !AbilityInstance->IsValid() || m_PendingCancels.Num() == 0) return false;
	if (&AbilityInstance->GetAbility() == nullptr) return false;
	
	return m_PendingCancels.ContainsByPredicate([&](FAblePendingCancelContext Context)
	{
		return Context.GetNameHash() == AbilityInstance->GetAbilityNameHash();
	});
}

bool UAbleAbilityComponent::MatchesQuery(const FGameplayTagQuery Query, bool includeExecutingAbilities) const
{
    FGameplayTagContainer CombinedTags;
    GetCombinedGameplayTags(CombinedTags, includeExecutingAbilities);
    return CombinedTags.MatchesQuery(Query);
}

void UAbleAbilityComponent::SetAbilityAnimationNode(const FAnimNode_SPAbilityAnimPlayer* Node)
{
	FScopeLock CS(&m_AbilityAnimNodeCS);

	m_AbilityAnimationNode = Node;
}

/*uint16 UAbleAbilityComponent::GetPredictionKey()
{
	if (IsAuthoritative())
	{
		m_ServerPredictionKey = FMath::Max<uint16>(++m_ServerPredictionKey, 1);
		return m_ServerPredictionKey;
	}

	m_ClientPredictionKey = FMath::Max<uint16>(++m_ClientPredictionKey, 1);
	return m_ClientPredictionKey;
}*/

void UAbleAbilityComponent::AddLocallyPredictedAbility(const FAbleAbilityNetworkContext& Context)
{
	m_LocallyPredictedAbilities[Context.GetPredictionKey() % ABLE_ABILITY_PREDICTION_RING_SIZE] = Context;
}

bool UAbleAbilityComponent::WasLocallyPredicted(const FAbleAbilityNetworkContext& Context)
{
	if (Context.GetPredictionKey() == 0 || m_LocallyPredictedAbilities.Num() == 0)
	{
		return false;
	}

	uint32 PredictionTolerance = m_Settings.IsValid() ? m_Settings->GetPredictionTolerance() : 0U;

	FAbleAbilityNetworkContext* FoundLocalContext = m_LocallyPredictedAbilities.FindByPredicate([&](const FAbleAbilityNetworkContext& LHS)
	{
		return LHS.GetAbility().IsValid() && LHS.GetAbility()->GetAbilityNameHash() == Context.GetAbility()->GetAbilityNameHash() && (uint32)FMath::Abs(LHS.GetPredictionKey() - Context.GetPredictionKey()) <= PredictionTolerance;
	});

	if (FoundLocalContext)
	{
		// Found a match entry, consume it.
		FoundLocalContext->Reset();
		return true;
	}

	return false;
}

ETraceTypeQuery UAbleAbilityComponent::GetQueryType(ESPAbleTraceType TraceType)
{
	return ETraceTypeQuery::TraceTypeQuery_MAX;
}

ECollisionChannel UAbleAbilityComponent::GetCollisionChannelFromTraceType(ESPAbleTraceType TraceType)
{
	return ECollisionChannel::ECC_MAX;
}

void UAbleAbilityComponent::FilterTargets(UAbleAbilityContext& Context)
{
}

#undef LOCTEXT_NAMESPACE
