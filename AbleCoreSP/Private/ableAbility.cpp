// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbility.h"
#include "AbleCoreSPPrivate.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Misc/Crc.h"
#include "Tasks/ablePlayAnimationTask.h"
#include "Tasks/IAbleAbilityTask.h"


#define LOCTEXT_NAMESPACE "AbleAbility"

UAbleAbility::UAbleAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_Length(1.0f),
	m_Cooldown(0.0f),
	m_CacheCooldown(true),
	m_FinishAllTasks(false),
	m_PlayRate(1.0f),
	m_IsPassive(false),
	m_InitialStackCount(1),
	m_StackIncrement(1),
	m_StackDecrement(1),
	m_MaxStacks(0),
	m_RefreshDurationOnNewStack(true),
	m_AlwaysRefreshDuration(false),
	m_OnlyRefreshLoopTime(false),
	m_ResetLoopCountOnRefresh(false),
	m_DecrementAndRestartOnEnd(false),
	m_Loop(false),
	m_LoopStart(0.0f),
	m_LoopEnd(1.0f),
	m_LoopMaxIterations(0U),
	m_RequiresTarget(false),
	m_Targeting(nullptr),
    m_DecayStackTime(0.0f),
	m_IsChanneled(false),
	m_ChannelConditions(),
	m_MustPassAllChannelConditions(true),
	m_FailedChannelResult(EAbleAbilityTaskResult::Interrupted),
	m_Tasks(),
	// m_InstancePolicy(EAbleInstancePolicy::Default),
	m_ClientPolicy(EAbleClientExecutionPolicy::Default),
	m_AbilityNameHash(0U),
	m_AbilityRealm(0),
	m_DependenciesDirty(true)
{
#if WITH_EDITORONLY_DATA
	ThumbnailImage = nullptr;
	ThumbnailInfo = nullptr;
	DrawTargetingShapes = false;
	IsCollapsed = false;
#endif
}

void UAbleAbility::PostInitProperties()
{
	Super::PostInitProperties();
}

void UAbleAbility::PreSave(const class ITargetPlatform* TargetPlatform)
{
#if WITH_EDITOR
	SanitizeTasks();
	SortTasks();
#endif
	Super::PreSave(TargetPlatform);
}

void UAbleAbility::PostLoad()
{
	Super::PostLoad();
	
	if (m_Segments.Num() <= 0)
	{
		FAbilitySegmentDefineData Default = FAbilitySegmentDefineData();
		Default.m_Tasks.Append(m_Tasks);
		Default.m_AllDependentTasks.Append(m_AllDependentTasks);
		Default.m_Length = m_Length;
		Default.m_LoopStart = m_LoopStart;
		Default.m_LoopEnd = m_LoopEnd;
		Default.m_LoopMaxIterations = m_LoopMaxIterations;
#if WITH_EDITORONLY_DATA
		Default.CompactData.Append(CompactData);
#endif
		Default.m_Loop = IsLooping();
		Default.m_FinishAllTasks = m_FinishAllTasks;
		m_Segments.Add(Default);
	}

    // remove any broken references
    const int32 numTasks = m_Tasks.Num();
    m_Tasks.RemoveAll([](const UAbleAbilityTask* Source) { return Source == nullptr; });
    if (numTasks < m_Tasks.Num())
    {
        UE_LOG(LogAbleSP, Warning, TEXT("UAbleAbility.PostLoad() %s had %d NULL Tasks."), *GetAbilityName(), m_Tasks.Num()-numTasks);
    }

	BindDynamicProperties();

#if WITH_EDITORONLY_DATA
	for (UAbleAbilityTask* ReferencedTask : m_Tasks)
	{
        // Listen for any property changes on our Tasks. 
        ReferencedTask->GetOnTaskPropertyModified().AddUObject(this, &UAbleAbility::OnReferencedTaskPropertyModified);
    }
#endif

	// Generate our Name hash.
	m_AbilityNameHash = FCrc::StrCrc32(*GetName());
}

bool UAbleAbility::IsSupportedForNetworking() const
{
	// return GetInstancePolicy() == EAbleInstancePolicy::NewInstanceReplicated ||
	return true; // GetOuter()->IsA(UPackage::StaticClass());
}

void UAbleAbility::PreExecutionInit() const
{
	for (int SegmentIndex = 0; SegmentIndex < m_Segments.Num(); SegmentIndex++)
	{
		BuildDependencyList(SegmentIndex);
	}
}

EAbleAbilityStartResult UAbleAbility::CanAbilityExecute(UAbleAbilityContext& Context) const
{
	const AActor* Owner = Context.GetOwner();
	if (IsValid(Owner))
	{
		Context.SetTargetActorRotationSnap(Owner->GetActorRotation());
	}
	
	// Check Targeting...
	if (m_Targeting != nullptr)
	{
		// If this is Async, it's safe to call it multiple times as it will poll for the results.
		m_Targeting->FindTargets(Context);
		
		if (Context.GetMutableTargetActors().Num())
		{
			const TWeakObjectPtr<AActor> TargetActor = Context.GetMutableTargetActors()[0];
			if (TargetActor.IsValid())
			{
				Context.SetTargetActorLocationSnap(TargetActor->GetActorLocation());
			}
		}

		if (m_Targeting->IsUsingAsync() && Context.HasValidAsyncHandle())
		{
			return EAbleAbilityStartResult::AsyncProcessing;
		}

		if (RequiresTarget() && !Context.HasAnyTargets())
		{
			return EAbleAbilityStartResult::InvalidTarget;
		}
	}

	// Check Custom BP logic
	if (!CustomCanAbilityExecuteBP(&Context))
	{
		return EAbleAbilityStartResult::FailedCustomCheck;
	}

	return EAbleAbilityStartResult::Success;
}

FString UAbleAbility::GetAbilityName() const
{
	FString Result;
	GetName(Result);
	int32 UnderscoreIndex;

	// Chop off the variant (_A/B/C/D) from the Left side.
	if (Result.FindLastChar('_', UnderscoreIndex))
	{
		int32 UnderscoreSize = Result.Len() - UnderscoreIndex;
	
		static FString DefaultString(TEXT("Default__"));
		if (Result.Find(DefaultString) >= 0)
		{
			int32 StringSize = Result.Len() - UnderscoreSize - DefaultString.Len();
			Result = Result.Mid(DefaultString.Len(), StringSize);
		}
		else
		{
			Result = Result.LeftChop(UnderscoreSize);
		}
	}

	return Result;
}

void UAbleAbility::OnAbilityIterationBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityIteration(Context);
}

EAbleCallbackResult UAbleAbility::OnCollisionEvent(const UAbleAbilityContext* Context, const FName& EventName, const TArray<struct FAbleQueryResult>& HitEntities) const
{
	return EAbleCallbackResult::Complete;
}

EAbleCallbackResult UAbleAbility::OnRaycastEvent(const UAbleAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const
{
	return EAbleCallbackResult::Complete;
}

float UAbleAbility::GetPlayRate(const UAbleAbilityContext* Context) const
{
	const float Rate = Context->GetCustomPlayRate() * ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, m_PlayRate);
	return Rate;
}

void UAbleAbility::SetPlayRate(const UAbleAbilityContext* Context, float NewPlayRate)
{
	if (Context)
	{
		m_PlayRate = NewPlayRate;
		const TArray<UAbleAbilityTask*>& Tasks = GetTasks(Context->GetActiveSegmentIndex());
		for (UAbleAbilityTask* Task : Tasks)
		{
			if (!Task)
			{
				continue;
			}
			Task->OnAbilityPlayRateChanged(Context, NewPlayRate);
		}
	}
}

FVector UAbleAbility::GetTargetLocation(const UAbleAbilityContext* Context) const
{
	if (Context && Context->GetSelfActor())
	{
		return Context->GetSelfActor()->GetActorLocation();
	}

	return FVector::ZeroVector;
}

bool UAbleAbility::CheckCustomConditionEventBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName) const
{
	return CheckCustomConditionEvent(Context, EventName);
}

void UAbleAbility::OnAbilityStackAdded(const UAbleAbilityContext* Context) const
{

}

void UAbleAbility::OnAbilityStackAddedBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityStackAdded(Context);
}

void UAbleAbility::OnAbilityStackRemoved(const UAbleAbilityContext* Context) const
{

}

void UAbleAbility::OnAbilityStackRemovedBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityStackRemoved(Context);
}

TSubclassOf<UAbleAbilityScratchPad> UAbleAbility::GetAbilityScratchPadClass(const UAbleAbilityContext* Context) const
{
	return nullptr;
}

TSubclassOf<UAbleAbilityScratchPad> UAbleAbility::GetAbilityScratchPadClassBP_Implementation(const UAbleAbilityContext* Context) const
{
	return GetAbilityScratchPadClass(Context);
}

void UAbleAbility::ResetAbilityScratchPad(UAbleAbilityScratchPad* ScratchPad) const
{
	ResetAbilityScratchPadBP(ScratchPad);
}

void UAbleAbility::ResetAbilityScratchPadBP_Implementation(UAbleAbilityScratchPad* ScratchPad) const
{
	// Do nothing as we don't know the contents of the Scratchpad.
}

bool UAbleAbility::CanClientCancelAbilityBP_Implementation(const UAbleAbilityContext* Context) const
{
	return CanClientCancelAbility(Context);
}

USkeletalMeshComponent* UAbleAbility::GetSkeletalMeshComponentForActor(const UAbleAbilityContext* Context, AActor* Actor, const FName& EventName) const
{
	return GetSkeletalMeshComponentForActorBP(Context, Actor, EventName);
}

USkeletalMeshComponent* UAbleAbility::GetSkeletalMeshComponentForActorBP_Implementation(const UAbleAbilityContext* Context, AActor* Actor, const FName& EventName) const
{
	return nullptr;
}

UWorld* UAbleAbility::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

int32 UAbleAbility::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UAbleAbility::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != nullptr);

	AActor* Owner = CastChecked<AActor>(GetOuter());

	bool bProcessed = false;

	FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
	if (Context != nullptr)
	{
		for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
		{
			if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(Owner, Function))
			{
				Driver.NetDriver->ProcessRemoteFunction(Owner, Function, Parameters, OutParms, Stack, this);
				bProcessed = true;
			}
		}
	}

	return bProcessed;
}

void UAbleAbility::CleanUp()
{
	if (m_Tasks.Num())
	{
		for (UAbleAbilityTask* Task : m_Tasks)
		{
			if (IsValid(Task))
			{
				Task->Clear();
				Task->MarkPendingKill();
			}
		}

		m_Tasks.Empty();
	}

	if (IsValid(m_Targeting))
	{
		m_Targeting->CleanUp();
		m_Targeting->MarkPendingKill();
		m_Targeting = nullptr;
	}

	for(UAbleChannelingBase* ChannelCondition : m_ChannelConditions)
	{
		if (IsValid(ChannelCondition))
		{
			ChannelCondition->MarkPendingKill();
		}
	}
}

FVector UAbleAbility::GetTargetLocationBP_Implementation(const UAbleAbilityContext* Context) const
{
	return GetTargetLocation(Context);
}

bool UAbleAbility::ShouldCancelAbilityBP_Implementation(const UAbleAbilityContext* Context, const UAbleAbility* Ability, const FName& EventName) const
{
	return ShouldCancelAbility(Context, Ability, EventName);
}

float UAbleAbility::CalculateCooldownBP_Implementation(const UAbleAbilityContext* Context) const
{
	return GetCooldown(Context);
}

bool UAbleAbility::CanInterruptAbilityBP_Implementation(const UAbleAbilityContext* Context, const UAbleAbility* CurrentAbility) const
{
	return CanInterruptAbility(Context, CurrentAbility);
}

bool UAbleAbility::CustomFilterConditionBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName, AActor* Actor) const
{
	return CustomFilterCondition(Context, EventName, Actor);
}

bool UAbleAbility::SegmentMustFinishAllTasks(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return false;

	return m_Segments[SegmentIndex].m_FinishAllTasks;
}

float UAbleAbility::CalculateCooldown(const UAbleAbilityContext* Context) const
{
	return GetBaseCooldown();
}

int32 UAbleAbility::GetMaxStacksBP_Implementation(const UAbleAbilityContext* Context) const
{
	return GetMaxStacks(Context);
}

bool UAbleAbility::CustomCanAbilityExecuteBP_Implementation(const UAbleAbilityContext* Context) const
{
	return CustomCanAbilityExecute(Context);
}

bool UAbleAbility::CustomCanLoopExecuteBP_Implementation(const UAbleAbilityContext* Context) const
{
	return CustomCanLoopExecute(Context);
}

EAbleConditionResults UAbleAbility::CheckCustomChannelConditionalBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName) const
{
	return CheckCustomChannelConditional(Context, EventName);
}

float UAbleAbility::CalculateDamageForActorBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName, float BaseDamage, AActor* Actor) const
{
	return CalculateDamageForActor(Context, EventName, BaseDamage, Actor);
}

bool UAbleAbility::CustomCanBranchToBP_Implementation(const UAbleAbilityContext* Context, const UAbleAbility* BranchAbility) const
{
	return CustomCanBranchTo(Context, BranchAbility);
}

void UAbleAbility::OnAbilityStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityStart(Context);
}

void UAbleAbility::OnAbilityEndBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityEnd(Context);
}

void UAbleAbility::OnAbilityInterruptBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityInterrupt(Context);
}

void UAbleAbility::OnAbilityBranchBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilityBranch(Context);
}

void UAbleAbility::OnAbilitySegmentBranchedBP_Implementation(const UAbleAbilityContext* Context) const
{
	OnAbilitySegmentBranched(Context);
}

EAbleCallbackResult UAbleAbility::OnCollisionEventBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName, const TArray<FAbleQueryResult>& HitEntities) const
{
	return OnCollisionEvent(Context, EventName, HitEntities);
}

EAbleCallbackResult UAbleAbility::OnRaycastEventBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName, const TArray<FHitResult>& HitResults) const
{
	return OnRaycastEvent(Context, EventName, HitResults);
}

void UAbleAbility::OnCustomEventBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName) const
{
	OnCustomEvent(Context, EventName);
}

void UAbleAbility::OnSpawnedActorEventBP_Implementation(const UAbleAbilityContext* Context, const FName& EventName, AActor* SpawnedActor, int SpawnIndex) const
{
	OnSpawnedActorEvent(Context, EventName, SpawnedActor, SpawnIndex);
}

void UAbleAbility::BuildDependencyList(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return;
	
	m_Segments[SegmentIndex].m_AllDependentTasks.Empty();
	m_AllDependentTasks.Empty();

	for (UAbleAbilityTask* Task : m_Tasks)
	{
		if (!Task)
		{
			// Should never occur, but to be safe.
			continue; 
		}

        if (Task->HasDependencies())
        {
			Task->GetMutableTaskDependencies().RemoveAll([](const UAbleAbilityTask* LHS){ return LHS == nullptr; });

			for (const UAbleAbilityTask* TaskDependency : Task->GetTaskDependencies())
			{
				if (!TaskDependency)
				{
					continue;
				}

                // Make sure our Tasks and Dependencies are in the same realms (or Client/Server so they'll always run) and that they aren't stale somehow.
                if ((TaskDependency->GetTaskRealm() == Task->GetTaskRealm() ||
                    TaskDependency->GetTaskRealm() == EAbleAbilityTaskRealm::ATR_ClientAndServer ||
                    Task->GetTaskRealm() == EAbleAbilityTaskRealm::ATR_ClientAndServer) &&
					m_Segments[SegmentIndex].m_Tasks.Contains(TaskDependency))
                    // m_Tasks.Contains(TaskDependency))
                {
					m_Segments[SegmentIndex].m_AllDependentTasks.AddUnique(TaskDependency);
                    m_AllDependentTasks.AddUnique(TaskDependency);
                }
            }
        }
	}

	m_Segments[SegmentIndex].m_DependenciesDirty = false;
}

FName UAbleAbility::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_") + PropertyName;
	return FName(*DelegateName);
}

void UAbleAbility::BindDynamicProperties()
{
	for (UAbleAbilityTask* Task : m_Tasks )
    {
		if (!Task) continue;
        Task->BindDynamicDelegates(this);
    }

	for (UAbleChannelingBase* Channel : m_ChannelConditions)
	{
		if (Channel)
		{
			Channel->BindDynamicDelegates(this);
		}
	}

	if (m_Targeting)
	{
		m_Targeting->BindDynamicDelegates(this);
	}

	// ABL_BIND_DYNAMIC_PROPERTY(this, m_Cooldown, TEXT("Cooldown"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_PlayRate, TEXT("Play Rate"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_InitialStackCount, TEXT("Initial Stacks"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_StackIncrement, TEXT("Stack Increment"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_StackDecrement, TEXT("Stack Decrement"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_MaxStacks, TEXT("Max Stacks"));
	// ABL_BIND_DYNAMIC_PROPERTY(this, m_LoopMaxIterations, TEXT("Max Iterations"));
	ABL_BIND_DYNAMIC_PROPERTY(this, m_IsPassive, TEXT("Is Passive"));
    ABL_BIND_DYNAMIC_PROPERTY(this, m_DecayStackTime, TEXT("Stack Decay Time"));    
}

bool UAbleAbility::HasTag(const FGameplayTag Tag) const
{
    return m_TagContainer.HasTag(Tag);
}

/* Returns true if we have any tags from the passed in container in our own container. */
bool UAbleAbility::MatchesAnyTag(const FGameplayTagContainer Container) const
{
    return m_TagContainer.HasAny(Container);
}

/* Returns true if we have all the tags from the passed in container in our own container. */
bool UAbleAbility::MatchesAllTags(const FGameplayTagContainer Container) const
{
    return m_TagContainer.HasAll(Container);
}

/* Returns true if the ability checks pass for a series of gameplay tags. */
bool UAbleAbility::CheckTags(const FGameplayTagContainer& IncludesAny, const FGameplayTagContainer& IncludesAll, const FGameplayTagContainer& ExcludesAny) const
{
    if (!ExcludesAny.IsEmpty())
    {
        if (m_TagContainer.HasAny(ExcludesAny))
            return false;
    }
    if (!IncludesAny.IsEmpty())
    {
        if (!m_TagContainer.HasAny(IncludesAny))
            return false;
    }
    if (!IncludesAll.IsEmpty())
    {
        if (!m_TagContainer.HasAll(IncludesAll))
            return false;
    }
    return false;
}

bool UAbleAbility::IsSegmentLooping(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return false;

	return m_Segments[SegmentIndex].m_Loop;
}

uint32 UAbleAbility::GetSegmentLoopMaxIterations(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return 0;

	return m_Segments[SegmentIndex].m_LoopMaxIterations;
}

FVector2D UAbleAbility::GetSegmentLoopRange(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return FVector2D();

	return FVector2D(m_Segments[SegmentIndex].m_LoopStart, m_Segments[SegmentIndex].m_LoopEnd);
}

const TArray<UAbleAbilityTask*>& UAbleAbility::GetTasks() const 
{
	return m_Tasks;
}

const TArray<UAbleAbilityTask*> UAbleAbility::GetTasks(const int SegmentIndex) const
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return {};
	
	return m_Segments[SegmentIndex].m_Tasks;
}

const int UAbleAbility::FindSegmentIndexByFName(FName name) const
{
	for (int i = 0; i < m_Segments.Num(); i++)
	{
		if (m_Segments[i].m_SegmentName == name)
			return i;
	}
	return -1;
}

int UAbleAbility::FindSpecSegmentIndexByFName(TArray<FAbilitySegmentDefineData>& m_SpecSegments, FName name) const
{
	for (int i = 0; i < m_SpecSegments.Num(); i++)
	{
		if (m_SpecSegments[i].m_SegmentName == name)
			return i;
	}
	
	return -1;
}

TArray<FName> UAbleAbility::FindSegments() const
{
	TArray<FName> Segments;

	for (int i = 0; i < m_Segments.Num(); ++i)
	{
		Segments.Add(m_Segments[i].m_SegmentName);
	}

	return Segments;
}

TArray<FAbilitySegmentDefineData> UAbleAbility::GetAllSegmentDefineData() const
{
	return m_Segments;
}

void UAbleAbility::SortSegments(const TArray<FName>& SortedSegmentNames)
{
	auto CompareFunction = [&](const FAbilitySegmentDefineData& A, const FAbilitySegmentDefineData& B)
	{
		const int32 Index1 = SortedSegmentNames.IndexOfByKey(A.m_SegmentName);
		const int32 Index2 = SortedSegmentNames.IndexOfByKey(B.m_SegmentName);
		return Index1 < Index2;
	};
	
	m_Segments.Sort(CompareFunction);
}

void UAbleAbility::SetNodeDatas(const TArray<FSegmentNodeData>& NodeDatas)
{
	auto CompareFunction = [&](const FAbilitySegmentBranchData& A, const FAbilitySegmentBranchData& B)
	{
		return A.Priority < B.Priority;
	};

	// 除重
	TSet<FName> Names;
	for (int32 i = m_Segments.Num() - 1; i >= 0; i--)
	{
		FName SegmentName = m_Segments[i].m_SegmentName;
		if (!Names.Contains(SegmentName))
		{
			Names.Add(SegmentName);
		}
		else
		{
			m_Segments.RemoveAt(i);
		}
	}
	
	for (int32 i = 0; i < m_Segments.Num(); i++)
	{
		auto& Segment = m_Segments[i];
		
		for (auto& Node : NodeDatas)
		{
			if (Segment.m_SegmentName == Node.Name)
			{
				Segment.BranchData.Empty();
				for (auto& Branch : Node.BranchData)
				{
					Segment.BranchData.Add(Branch);
				}
				Segment.BranchData.Sort(CompareFunction);
				break;
			}
		}

		for (auto& Task : Segment.m_Tasks)
		{
			if (IsValid(Task))
			{
				Task->SetSegment(i);
			}
		}
	}
}

FAbilitySegmentDefineData* UAbleAbility::FindSegmentDefineDataByIndex(const int Index)
{
	if (!m_Segments.IsValidIndex(Index)) return nullptr;
	return &m_Segments[Index];
}

const FAbilitySegmentDefineData* UAbleAbility::FindSegmentDefineDataByIndex(const int Index) const
{
	check(Index >= 0 && Index < m_Segments.Num());
	return &m_Segments[Index];
}

const TArray<FAbilitySegmentBranchData>& UAbleAbility::GetBranchData() const
{
	return BranchData;
}

void UAbleAbility::GetBeforeSegments(int Index, TArray<const FAbilitySegmentDefineData*>& SegmentArray)
{
	// const FAbilitySegmentDefineData* Target = FindSegmentDefineDataByIndex(Index);
	// if (Target != nullptr)
	// {
	// 	for (int i = 0; i < m_Segments.Num(); ++i)
	// 	{
	// 		if (i != Index)
	// 		{
	// 			const FAbilitySegmentDefineData* DefineData = &m_Segments[i];
	// 			for (auto& InBranchData : DefineData->BranchData)
	// 			{
	// 				if (InBranchData.NextName == Target->m_SegmentName)
	// 				{
	// 					SegmentArray.Add(DefineData);
	// 				}
	// 			}
	// 		}		
	// 	}
	// }
	for (auto& Segment : m_Segments)
	{
		SegmentArray.Add(&Segment);
	}
}

float UAbleAbility::GetLength(const int Index) const
{
	if (!m_Segments.IsValidIndex(Index)) return m_Length;

	return m_Segments[Index].m_Length;
}

float UAbleAbility::GetAbilityLength() const
{
	float Length = .0f;
	for (const FAbilitySegmentDefineData& Segment : m_Segments)
	{
		Length += Segment.m_Length;
	}

	return Length;
}

void UAbleAbility::Process(const UAbleAbility* Template)
{
	if (!Template) return;

	for (UAbleAbilityTask* Task : m_Tasks)
	{
		if (!Task)
		{
			// Should never occur, but to be safe.
			continue; 
		}

		if (Task->HasDependencies())
		{
			Task->GetMutableTaskDependencies().RemoveAll([](const UAbleAbilityTask* LHS){ return LHS == nullptr; });

			auto Dependencies = TArray<const UAbleAbilityTask*>();
			for (const UAbleAbilityTask* TaskDependency : Task->GetTaskDependencies())
			{
				if (!TaskDependency)
				{
					continue;
				}
				
				for (const UAbleAbilityTask* InnerTask : m_Tasks)
				{
					if (!InnerTask) continue;
					if (TaskDependency->GetTaskNameHash() == InnerTask->GetTaskNameHash())
					{
						Dependencies.Add(InnerTask);
					}
				}
			}
			Task->GetMutableTaskDependencies().Empty(Dependencies.Num());
			Task->GetMutableTaskDependencies() += Dependencies;
		}
	}

	if (Template->IsPassive())
	{
		SetPassive(Template->IsPassive());
	}
}

#if WITH_EDITOR

void UAbleAbility::RenameSegmentList(TArray<FName>& SegmentNameList, TArray<FName>& OldSegmentNameList,
	FName EntryNodeName)
{
	for (int Index = m_Segments.Num() - 1; Index >= 0; Index--)
	{
		FName Name = m_Segments[Index].m_SegmentName;
		if (!SegmentNameList.Contains(Name))
		{
			int FindIndex;
			if (OldSegmentNameList.Find(Name, FindIndex))
			{
				m_Segments[Index].m_SegmentName = SegmentNameList[FindIndex];
			}
			else
			{
				m_Segments.RemoveAt(Index);
			}
		}
	}
	
	TArray<FName> CurrentNames;
	for (FAbilitySegmentDefineData& SegmentData : m_Segments)
	{
		CurrentNames.Add(SegmentData.m_SegmentName);
	}
	
	for (FName Name : SegmentNameList)
	{
		if (!CurrentNames.Contains(Name))
		{
			m_Segments.AddDefaulted(1);
			m_Segments[m_Segments.Num() - 1].m_SegmentName = Name;
		}
	}
	
	int Index = FindSegmentIndexByFName(EntryNodeName);
	if (Index < 0) Index = 0;
	m_EntrySegmentIndex = Index;
}

void UAbleAbility::SanitizeTasks()
{
	bool hasInvalidTasks = m_Tasks.ContainsByPredicate([](const UAbleAbilityTask* LHS)
	{
		return LHS == nullptr;
	});

	if (hasInvalidTasks)
	{
		m_Tasks.RemoveAll([](const UAbleAbilityTask* LHS){ return LHS == nullptr; });
	}
}

void UAbleAbility::CopyInheritedTasks(const UAbleAbility& ParentObject)
{
	for (FAbilitySegmentDefineData& SegmentDefine : m_Segments)
	{
		SegmentDefine.m_Tasks.Empty();
	}

	for (int i = 0; i < m_Segments.Num(); i++)
	{
		if (i < ParentObject.m_Segments.Num() && m_Segments[i].m_SegmentName == ParentObject.m_Segments[i].m_SegmentName)
		{
			const TArray<UAbleAbilityTask*>& ParentTasks = ParentObject.GetTasks(i);
			for (UAbleAbilityTask* ParentTask : ParentTasks)
			{
				if (ParentTask->IsInheritable())
				{
					UAbleAbilityTask* taskCopy = NewObject<UAbleAbilityTask>(Cast<UObject>(this), ParentTask->GetClass(), NAME_None, RF_Transactional, Cast<UObject>(ParentTask));
					if (!taskCopy)
					{
						UE_LOG(LogAbleSP, Warning, TEXT("Failed to copy Parent Task [%s] from Parent[%s], it will not be available in the Child Ability [%s]"),
							*ParentTask->GetTaskName().ToString(),
							*ParentObject.GetAbilityName(),
							*GetAbilityName());

						continue;
					}
					
					m_Segments[i].m_Tasks.Add(taskCopy);
					MarkPackageDirty();
				}
			}
		}
	}
	
	/*m_Tasks.Empty();

	const TArray<UAbleAbilityTask*>& ParentTasks = ParentObject.GetTasks();
	for (UAbleAbilityTask* ParentTask : ParentTasks)
	{
		if (ParentTask && ParentTask->IsInheritable())
		{
			UAbleAbilityTask* taskCopy = NewObject<UAbleAbilityTask>(Cast<UObject>(this), ParentTask->GetClass(), NAME_None, RF_Public | RF_Transactional, Cast<UObject>(ParentTask));
			if (!taskCopy)
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Failed to copy Parent Task [%s] from Parent[%s], it will not be available in the Child Ability [%s]"),
					*ParentTask->GetTaskName().ToString(),
					*ParentObject.GetDisplayName(),
					*GetDisplayName());

				continue;
			}
			m_Tasks.Add(taskCopy);
			MarkPackageDirty();
		}
	}*/
}

bool UAbleAbility::FixUpObjectFlags()
{
	bool modified = false;

	for (UAbleChannelingBase* ChannelCond : m_ChannelConditions)
	{
		if (!ChannelCond)
		{
			continue;
		}

		modified |= ChannelCond->FixUpObjectFlags();
	}

	for (UAbleAbilityTask* Task : m_Tasks)
	{
		if (!Task)
		{
			continue;
		}

		modified |= Task->FixUpObjectFlags();
	}

	return modified;
}

void UAbleAbility::ClearBranchData()
{
	BranchData.Empty();
}

void UAbleAbility::ClearNullTasks()
{
	for (int i = 0; i < m_Segments.Num(); ++i)
    {
		for (int j = 0; j < m_Segments[i].m_Tasks.Num(); ++j)
        {
            if (!m_Segments[i].m_Tasks[j])
            {
                m_Segments[i].m_Tasks.RemoveAt(j);
                --j;
            }
        }
    }
	
	for (int i = 0; i < m_Tasks.Num(); ++i)
    {
        if (!m_Tasks[i])
        {
            m_Tasks.RemoveAt(i);
            --i;
        }
    }
}

void UAbleAbility::AddBranchData(const FAbilitySegmentBranchData& InBranchData)
{
	FAbilitySegmentBranchData Data;
	Data.Priority = InBranchData.Priority;
	Data.NextName = InBranchData.NextName;
	Data.bMustPassAllConditions = InBranchData.bMustPassAllConditions;
	Data.Realm = InBranchData.Realm;
	
	constexpr EObjectFlags TargetFlags = RF_Public|RF_Transactional|RF_ArchetypeObject|RF_WasLoaded|RF_LoadCompleted;
	for (auto& Condition : InBranchData.Conditions)
	{
		UAbleChannelingBase* Channeling = DuplicateObject<UAbleChannelingBase>(Condition, this);
		Condition->SetFlags(TargetFlags);
		Channeling->SetFlags(TargetFlags);
		Data.Conditions.Add(Channeling);
	}
	
	BranchData.Add(Data);
}

bool UAbleAbility::UpdateTasks()
{
	bool bRet = false;
	
	if (m_Segments.Num() > 0)
	{
		TArray<UAbleAbilityTask*> SegmentTasks;
		for (auto& Segment : m_Segments)
		{
			Segment.m_Tasks.RemoveAll([&bRet](const UAbleAbilityTask* SegmentTask)
			{
				if (!IsValid(SegmentTask))
				{
					bRet = true;
					return true;
				}
				return false;
			});
			SegmentTasks.Append(Segment.m_Tasks);
		}

		constexpr EObjectFlags TargetFlags = RF_Public|RF_Transactional|RF_ArchetypeObject|RF_WasLoaded|RF_LoadCompleted;
		
		for (auto& Task : SegmentTasks)
		{
			if (Task->GetFlags() != TargetFlags)
			{
				UE_LOG(LogTemp, Log, TEXT("%s Flags = %d"), *Task->GetName(), Task->GetFlags());
				bRet = true;
				Task->SetFlags(TargetFlags);
			}
		}
		
		if (SegmentTasks.Num() == m_Tasks.Num())
		{
			for (auto& Task : SegmentTasks)
			{
				if (!m_Tasks.Contains(Task))
				{
					bRet = true;
			
					m_Tasks.Empty();
					m_Tasks.Append(SegmentTasks);

					break;
				}
			}
		}
		else
		{
			bRet = true;
			
			m_Tasks.Empty();
			m_Tasks.Append(SegmentTasks);
		}
		
	}
	
	return bRet;
}

void UAbleAbility::AddTask(UAbleAbilityTask& Task, const int SegmentIndex)
{
	if (m_Segments.IsValidIndex(SegmentIndex))
	{
		m_Segments[SegmentIndex].m_Tasks.Add(&Task);
	}
	
	m_Tasks.Add(&Task);
}

void UAbleAbility::RemoveTask(UAbleAbilityTask& Task, const int SegmentIndex)
{
	if (m_Segments.IsValidIndex(SegmentIndex))
	{
		m_Segments[SegmentIndex].m_Tasks.Remove(&Task);
	}
	
	m_Tasks.Remove(&Task);
	ValidateDependencies(SegmentIndex);
}

void UAbleAbility::RemoveTask(const UAbleAbilityTask& Task, const int SegmentIndex)
{
	if (m_Segments.IsValidIndex(SegmentIndex))
	{
		for (int i = 0; i < m_Segments[SegmentIndex].m_Tasks.Num(); ++i)
		{
			if (m_Segments[SegmentIndex].m_Tasks[i] == &Task)
			{
				m_Segments[SegmentIndex].m_Tasks.RemoveAt(i);
				break;
			}
		}
	}
	
	for (int i = 0; i < m_Tasks.Num(); ++i)
	{
		if (m_Tasks[i] == &Task)
		{
			m_Tasks.RemoveAt(i);
			break;
		}
	}

	ValidateDependencies(SegmentIndex);
}

void UAbleAbility::RemoveTaskAtIndex(int32 Index)
{
	m_Tasks.RemoveAt(Index);

	ValidateDependencies(Index);
}

void UAbleAbility::RemoveTaskAtIndex(int32 Index, int SegmentIndex)
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return;

	if (m_Segments[SegmentIndex].m_Tasks[Index])
		m_Segments[SegmentIndex].m_Tasks.RemoveAt(Index);
	else
		RemoveTask(*m_Segments[SegmentIndex].m_Tasks[Index], SegmentIndex);
}

void UAbleAbility::GetSortedTasks(TArray<UAbleAbilityTask*>& OutTasks, int SegmentIndex)
{
	// 按照order value显示task，这里不做大的改变
	OutTasks.Empty();
	OutTasks.Append(GetTasks(SegmentIndex));
	OutTasks.Remove(nullptr);

	TSet<int> orderValueSet;
	for (int index = 0; index < OutTasks.Num(); ++index)
	{
		if (!OutTasks[index]) continue;
		int orderValue = OutTasks[index]->GetDisplayOrderValue();
		if (orderValue < 0 || orderValueSet.Contains(orderValue))
		{
			OutTasks[index]->SetDisplayOrderValue(GetMaxOrderValueOfAllTask(SegmentIndex) + 1);
		}

		orderValueSet.Add(orderValue);
	}

	OutTasks.Sort([](const UAbleAbilityTask& A, const UAbleAbilityTask& B)
	{
		return A.GetDisplayOrderValue() < B.GetDisplayOrderValue();
	});
}

bool UAbleAbility::SwitchTaskOrder(UAbleAbilityTask* Task, int32 OrderModified, int SegmentIndex)
{
	TArray<UAbleAbilityTask*> Tasks;
	GetSortedTasks(Tasks, SegmentIndex);
	int taskCount = Tasks.Num();

	int32 CurIndex = Tasks.Find(Task);
	int32 toOrderIndex = CurIndex + OrderModified;

	if (CurIndex < 0 || CurIndex > taskCount) return false;
	if (CurIndex == toOrderIndex) return false;
	if (toOrderIndex < 0) toOrderIndex = 0;
	if (toOrderIndex >= taskCount) toOrderIndex = taskCount - 1;

	const int32 toOrderValue = Tasks[toOrderIndex]->GetDisplayOrderValue();
	if (CurIndex < toOrderIndex)
	{
		for (int index = toOrderIndex; index > CurIndex; --index)
		{
			Tasks[index]->SetDisplayOrderValue(Tasks[index - 1]->GetDisplayOrderValue());
		}
	}
	else
	{
		for (int index = toOrderIndex; index < CurIndex; ++index)
		{
			Tasks[index]->SetDisplayOrderValue(Tasks[index + 1]->GetDisplayOrderValue());
		}
	}
	Tasks[CurIndex]->SetDisplayOrderValue(toOrderValue);

	return true;
}

int32 UAbleAbility::GetMaxOrderValueOfAllTask(int SegmentIndex)
{
	const TArray<UAbleAbilityTask*>& allTask = GetTasks(SegmentIndex);
	int32 RecordMax = 0;
	for (const auto& Task : allTask)
	{
		if (Task && Task->GetDisplayOrderValue() > RecordMax)
		{
			RecordMax = Task->GetDisplayOrderValue();
		}
	}
	return RecordMax;
}

void UAbleAbility::SortTasks(const int SegmentIndex)
{
	if (SegmentIndex < 0)
	{
		for (FAbilitySegmentDefineData& SegmentData : m_Segments)
		{
			SegmentData.m_Tasks.StableSort([](const UAbleAbilityTask& A, const UAbleAbilityTask& B)
			{
				if (!IsValid(&A) || !IsValid(&B)) return false;
				return A.GetStartTime() == B.GetStartTime() ? A.GetDisplayOrderValue() < B.GetDisplayOrderValue() : A.GetStartTime() < B.GetStartTime();
			});
		}
	}
	else
	{
		if (m_Segments.IsValidIndex(SegmentIndex))
		{
			m_Segments[SegmentIndex].m_Tasks.StableSort([](const UAbleAbilityTask& A, const UAbleAbilityTask& B)
			{
				if (!IsValid(&A) || !IsValid(&B)) return false;
				return A.GetStartTime() == B.GetStartTime() ? A.GetDisplayOrderValue() < B.GetDisplayOrderValue() : A.GetStartTime() < B.GetStartTime();
			});
		}
	}
}

void UAbleAbility::ValidateDependencies(const int SegmentIndex)
{
	if (!m_Segments.IsValidIndex(SegmentIndex)) return;
	
	for (UAbleAbilityTask* Task : m_Segments[SegmentIndex].m_Tasks)
	{
		if (!IsValid(Task)) continue;
		
		TArray<const UAbleAbilityTask*>& Dependencies = Task->GetMutableTaskDependencies();
		for (int i = 0; i < Dependencies.Num();)
		{
			const UAbleAbilityTask* CurrentDependency = Dependencies[i];
			// Task no longer exists in our Ability, remove it. 
			if (!m_Segments[SegmentIndex].m_Tasks.Contains(CurrentDependency))
			{
				// See if we have a replacement.
				UAbleAbilityTask** replacementTask = m_Segments[SegmentIndex].m_Tasks.FindByPredicate([&](const UAbleAbilityTask* LHS) {
					return LHS->GetClass()->GetFName() == CurrentDependency->GetClass()->GetFName() &&
					FMath::IsNearlyEqual(LHS->GetStartTime(), CurrentDependency->GetStartTime()) &&
					FMath::IsNearlyEqual(LHS->GetEndTime(), CurrentDependency->GetEndTime()); 
				});

				if (replacementTask && *replacementTask)
				{
					Dependencies[i] = *replacementTask;
					++i;
				}
				else
				{
					Dependencies.RemoveAt(i, 1, false);
				}
			}
			else
			{
				++i;
			}
		}
		Dependencies.Shrink();
	}
}


void UAbleAbility::MarkTasksAsPublic()
{
	for (int32 i = 0; i < m_Tasks.Num(); ++i)
	{
		m_Tasks[i]->SetFlags(m_Tasks[i]->GetFlags() | RF_Public);
		m_Tasks[i]->MarkPackageDirty();
	}
}

void UAbleAbility::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Loop End can never be past our length.
	m_LoopEnd = FMath::Min(m_LoopEnd, m_Length);

	for (auto Segment : m_Segments)
	{
		Segment.m_LoopEnd = FMath::Min(Segment.m_LoopEnd, Segment.m_Length);
	}

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAbleAbility, m_Tasks))
	{
		// Our Tasks have changed, rebuild dependencies.
		for (int SegmentIndex = 0; SegmentIndex < m_Segments.Num(); SegmentIndex++)
		{
			ValidateDependencies(SegmentIndex);
			BuildDependencyList(SegmentIndex);
		}
	}

	if (!m_AbilityNameHash)
	{
		m_AbilityNameHash = FCrc::StrCrc32(*GetName());
	}

}

void UAbleAbility::OnReferencedTaskPropertyModified(UAbleAbilityTask& Task, struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName(TEXT("m_Dependencies")))
	{
		m_DependenciesDirty = true;
		// Our Task changed dependencies. Validate/Rebuild.
		for (int SegmentIndex = 0; SegmentIndex < m_Segments.Num(); SegmentIndex++)
		{
			ValidateDependencies(SegmentIndex);
			BuildDependencyList(SegmentIndex);
		}
	}

	if (!m_AbilityNameHash)
	{
		m_AbilityNameHash = FCrc::StrCrc32(*GetName());
	}

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName(TEXT("m_DynamicPropertyIdentifer")))
	{
		if (RequestEditorRefresh.IsBound())
		{
			RequestEditorRefresh.Broadcast();
		}
    }

	BindDynamicProperties();
}

bool UAbleAbility::HasInvalidTasks() const
{
	for (const UAbleAbilityTask* Task : m_Tasks)
	{
		if (!Task || Task->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated))
		{
			return true;
		}
	}

	return false;
}

bool UAbleAbility::NeedsCompactData(const int SegmentIndex) const
{
	if (SegmentIndex < 0 || SegmentIndex >= m_Segments.Num())
	{
		for (const FAbilitySegmentDefineData& SegmentData : m_Segments)
		{
			const UAbleAbilityTask* const * CompactDataTask = SegmentData.m_Tasks.FindByPredicate([](const UAbleAbilityTask* task) { return task && task->HasCompactData(); });
			if (SegmentData.CompactData.Num() == 0 && (CompactDataTask && *CompactDataTask != nullptr))
			{
				return true;
			}
		}
	}
	else
	{
		if (!m_Segments.IsValidIndex(SegmentIndex)) return false;
		
		const FAbilitySegmentDefineData& SegmentData = m_Segments[SegmentIndex];
		const UAbleAbilityTask* const * CompactDataTask = SegmentData.m_Tasks.FindByPredicate([](const UAbleAbilityTask* task) { return task && task->HasCompactData(); });
		if (SegmentData.CompactData.Num() == 0 && (CompactDataTask && *CompactDataTask != nullptr))
		{
			return true;
		}
	}
	return false;
}

void UAbleAbility::SaveCompactData(const int SegmentIndex)
{
	if (SegmentIndex < 0 || SegmentIndex >= m_Segments.Num())
	{
		for (FAbilitySegmentDefineData& SegmentData : m_Segments)
		{
			SegmentData.CompactData.Empty();
			for (UAbleAbilityTask* Task : SegmentData.m_Tasks)
			{
				if (Task && Task->HasCompactData())
				{
					Task->GetCompactData(SegmentData.CompactData.AddDefaulted_GetRef());
				}
			}
		}
	}
	else
	{
		FAbilitySegmentDefineData& SegmentData = m_Segments[SegmentIndex];
		SegmentData.CompactData.Empty();
		for (UAbleAbilityTask* Task : SegmentData.m_Tasks)
		{
			if (Task && Task->HasCompactData())
			{
				Task->GetCompactData(SegmentData.CompactData.AddDefaulted_GetRef());
			}
		}
	}
}

void UAbleAbility::LoadCompactData(const int SegmentIndex)
{
	for (int Index = 0; Index < m_Segments.Num(); Index++)
	{
		if (SegmentIndex > 0 && SegmentIndex < m_Segments.Num())
		{
			if(SegmentIndex != Index) continue;
		}
		FAbilitySegmentDefineData& SegmentData = m_Segments[Index];
		// Remove all compact data Tasks.
		SegmentData.m_Tasks.RemoveAll([](const UAbleAbilityTask* LHS) { return !LHS || LHS->HasCompactData(); });
		// Reconstruct our Tasks that we removed.
		for (FAbleCompactTaskData& data : SegmentData.CompactData)
		{
			if (data.TaskClass.IsValid())
			{
				UAbleAbilityTask* NewTask = NewObject<UAbleAbilityTask>(this, data.TaskClass.Get(), NAME_None, GetMaskedFlags(RF_PropagateToSubObjects) | RF_Transactional);
				if (!NewTask)
				{
					UE_LOG(LogAbleSP, Warning, TEXT("Failed to create task from compact data using class %s\n"), *data.TaskClass.ToString());
					continue;
				}
				NewTask->LoadCompactData(data);
				SegmentData.m_Tasks.Add(NewTask);
			}
			else
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Task had invalid class data. Please re-add it manually. \n"));
			}
		}
	}
	for (int Index = 0; Index < m_Segments.Num(); Index++)
	{
		if (SegmentIndex > 0 && SegmentIndex < m_Segments.Num())
		{
			if (SegmentIndex != Index) continue;
		}
		// Sort
		SortTasks(SegmentIndex);

		// Fix up dependencies
		ValidateDependencies(SegmentIndex);
		BuildDependencyList(SegmentIndex);
	}
}

EDataValidationResult UAbleAbility::IsDataValid(TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    const FText AssetName = FText::FromString(GetAbilityName());

    for (const UAbleChannelingBase* Filter : m_ChannelConditions)
    {
        if (Filter == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullCollisionFilter", "Null Collision Filter: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    for (UAbleAbilityTask* Task : m_Tasks)
    {
        if (Task == nullptr)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("NullTask", "Null Task: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }

        if (Task->IsTaskDataValid(this, AssetName, ValidationErrors) == EDataValidationResult::Invalid)
            result = EDataValidationResult::Invalid;
    }

	// This is tossing false positives somehow... Loading order issue with others BPs perhaps?
    //for (const UAbleAbilityTask* Task : m_AllDependentTasks)
    //{
    //    if (Task == nullptr)
    //    {
    //        ValidationErrors.Add(FText::Format(LOCTEXT("NullDependency", "Null Dependency: {0}"), AssetName));
    //        result = EDataValidationResult::Invalid;
    //    }
    //}

    return result;
}

void UAbleAbility::ValidateDataAgainstTemplate(const UAbleAbility* Template)
{
	// Only allowed on Transient objects.
	if (!Template || !(GetFlags() & RF_Transient))
	{
		return;
	}

	// Check Tasks.
	const TArray<UAbleAbilityTask*>& SourceTasks = Template->GetTasks();
	for (int32 i = 0; i < m_Tasks.Num(); ++i)
	{
		if (m_Tasks[i] == nullptr && SourceTasks[i] != nullptr)
		{
			m_Tasks[i] = SourceTasks[i];
		}
	}

	// Check Channel Conditions.
	const TArray<UAbleChannelingBase*>& SourceConditions = Template->GetChannelConditions();
	for (int32 i = 0; i < m_ChannelConditions.Num(); ++i)
	{
		if (m_ChannelConditions[i] == nullptr && SourceConditions[i] != nullptr)
		{
			m_ChannelConditions[i] = SourceConditions[i];
		}
	}
}

#endif

void UAbleAbilityRuntimeParametersScratchPad::SetIntParameter(FName Id, int Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_IntParameters.Add(Id, Value);
}

void UAbleAbilityRuntimeParametersScratchPad::SetFloatParameter(FName Id, float Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_FloatParameters.Add(Id, Value);
}

void UAbleAbilityRuntimeParametersScratchPad::SetStringParameter(FName Id, const FString& Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_StringParameters.Add(Id, Value);
}

void UAbleAbilityRuntimeParametersScratchPad::SetUObjectParameter(FName Id, UObject* Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ScratchpadVariablesLock);
	m_UObjectParameters.Add(Id, Value);
}

int UAbleAbilityRuntimeParametersScratchPad::GetIntParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const int* var = m_IntParameters.Find(Id))
	{
		return *var;
	}
	return 0;
}

float UAbleAbilityRuntimeParametersScratchPad::GetFloatParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const float* var = m_FloatParameters.Find(Id))
	{
		return *var;
	}
	return 0.0f;
}

UObject* UAbleAbilityRuntimeParametersScratchPad::GetUObjectParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (m_UObjectParameters.Contains(Id)) // Find is being weird with double ptr.
	{
		return m_UObjectParameters[Id];
	}
	return nullptr;
}

const FString& UAbleAbilityRuntimeParametersScratchPad::GetStringParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ScratchpadVariablesLock);
	if (const FString* var = m_StringParameters.Find(Id))
	{
		return *var;
	}
	static FString EmptyString;
	return EmptyString;
}

void UAbleAbilityRuntimeParametersScratchPad::ResetScratchpad()
{
	m_FloatParameters.Empty();
	m_IntParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
}

#undef LOCTEXT_NAMESPACE