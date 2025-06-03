// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityContext.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityComponent.h"
#include "AbleCoreSPPrivate.h"
#include "ableSettings.h"
#include "ableSubSystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"
#include "MoeGameplay/Core/MoeGameLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Tasks/IAbleAbilityTask.h"

class UGameFeatureSystemManager;

FVector FAbleQueryResult::GetLocation() const
{
	FTransform TargetTransform;
	GetTransform(TargetTransform);

	return TargetTransform.GetTranslation();
}

void FAbleQueryResult::GetTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (Actor.IsValid())
	{
		OutTransform = Actor->GetActorTransform();
	}
	else if (PrimitiveComponent.IsValid())
	{
		if (AActor* ActorOwner = PrimitiveComponent->GetOwner())
		{
			OutTransform = ActorOwner->GetActorTransform();
		}
		else
		{
			// Is this ever really valid?
			OutTransform = PrimitiveComponent->GetComponentTransform();
		}
	}
}

UAbleAbilityContext::UAbleAbilityContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_AbilityActorStartLocation(FVector::ZeroVector),
	  m_Ability(nullptr),
	  m_AbilityComponent(nullptr),
	  m_StackCount(1),
	  m_LoopIteration(0),
	  m_SegmentLoopIteration(0),
	  m_CurrentTime(0.0f),
	  m_LastDelta(0.0f),
	  m_AbilityScratchPad(nullptr),
	  m_TargetLocation(FVector::ZeroVector),
	  m_PredictionKey(0),
	  m_AbilityId(0),
	  m_RandomSeed(0),
	  m_TargetActorLocationSnap(FVector::ZeroVector),
	  m_TargetActorRotationSnap(FRotator::ZeroRotator)
{
}

UAbleAbilityContext::~UAbleAbilityContext()
{
}

UAbleAbilityContext* UAbleAbilityContext::MakeContext(const UAbleAbility* Ability,
                                                      UAbleAbilityComponent* AbilityComponent, AActor* Owner,
                                                      AActor* Instigator)
{
	if (!AbilityComponent)
	{
		return nullptr;
	}

	static const USPAbleSettings* Settings = nullptr;
	if (!Settings)
	{
		// This shouldn't be able to change out from under us.
		Settings = GetDefault<USPAbleSettings>();
	}

	UAbleAbilityContext* NewContext = nullptr;
	if (Settings && Settings->GetAllowAbilityContextReuse())
	{
		if (const AActor* ACOwner = AbilityComponent->GetOwner())
		{
			if (UWorld* OwnerWorld = ACOwner->GetWorld())
			{
				UGameFeatureSystemManager* FeatureSystemManager = OwnerWorld->GetSubsystem<UGameFeatureSystemManager>();
				if (FeatureSystemManager)
				{
					if (UAbleAbilityUtilitySubsystem* ContextSubsystem = Cast<UAbleAbilityUtilitySubsystem>(UMoeGameLibrary::GetGameFeatureSystem(OwnerWorld,UAbleAbilityUtilitySubsystem::StaticClass())))
					{
						NewContext = ContextSubsystem->FindOrConstructContext();
					}
				}
			}
		}
	}

	if (NewContext == nullptr)
	{
		NewContext = NewObject<UAbleAbilityContext>(AbilityComponent);
	}
	check(NewContext != nullptr);

	NewContext->m_Ability = Ability;
	NewContext->m_AbilityComponent = AbilityComponent;
	NewContext->SetRandomSeed(FMath::Rand());
	NewContext->SetRandomStream(FRandomStream(NewContext->GetRandomSeed()));
	
	if (Owner)
	{
		NewContext->m_Owner = Owner;
	}

	if (Instigator)
	{
		NewContext->m_Instigator = Instigator;
	}

	NewContext->m_ActiveSegmentIndex = 0;

	const TArray<FAbilitySegmentBranchData>& BranchDataList = Ability->GetBranchData();
	for (const FAbilitySegmentBranchData& BranchData : BranchDataList)
	{
		int32 BranchIndex = Ability->FindSegmentIndexByFName(BranchData.NextName);
		if (BranchIndex < 0)
			continue;
		
		if (UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(NewContext, BranchData))
		{
			NewContext->SetActiveSegmentIndex(BranchIndex);
			break;
		}
	}
	
	return NewContext;
}

UAbleAbilityContext* UAbleAbilityContext::MakeContext(const FAbleAbilityNetworkContext& NetworkContext, const UAbleAbility* Ability)
{
	UAbleAbilityContext* NewContext = MakeContext(Ability,
	                                              NetworkContext.GetAbilityComponent().Get(),
	                                              NetworkContext.GetOwner().Get(),
	                                              NetworkContext.GetInstigator().Get());
	if (!NewContext)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("[UAbleAbilityContext:MakeContext] MakeContext failed!"));
		//ensure(false);
		return nullptr;
	}
	NewContext->GetMutableTargetActors().Append(NetworkContext.GetTargetActors());
	NewContext->SetTargetLocation(NetworkContext.GetTargetLocation());
	NewContext->GetMutableIntParameters().Append(NetworkContext.GetIntParameters());
	NewContext->GetMutableFloatParameters().Append(NetworkContext.GetFloatParameters());
	NewContext->GetMutableStringParameters().Append(NetworkContext.GetStringParameters());
	NewContext->GetMutableUObjectParameters().Append(NetworkContext.GetUObjectParameters());
	NewContext->GetMutableVectorParameters().Append(NetworkContext.GetVectorParameters());
	NewContext->SetAbilityId(NetworkContext.GetAbilityId());
	NewContext->SetRandomSeed(NetworkContext.GetRandomSeed());
	NewContext->SetRandomStream(FRandomStream(NewContext->GetRandomSeed()));
	NewContext->SetActiveSegmentIndex(NetworkContext.GetSegmentIndex());
	NewContext->SetTargetActorLocationSnap(NetworkContext.GetTargetActorLocationSnap());
	NewContext->SetTargetActorRotationSnap(NetworkContext.GetTargetActorRotationSnap());
	NewContext->SetHighPingPawns(NetworkContext.GetHighPingPawns());
	NewContext->SetAbilityUniqueID(NetworkContext.GetAbilityUniqueID());
	return NewContext;
}

UAbleAbilityContext* UAbleAbilityContext::MakeContext(const UAbleAbility* Ability,
                                                      UAbleAbilityComponent* AbilityComponent, AActor* Owner,
                                                      AActor* Instigator, FVector Location)
{
	UAbleAbilityContext* NewContext = MakeContext(Ability, AbilityComponent, Owner, Instigator);
	if (NewContext)
	{
		NewContext->SetTargetLocation(Location);
	}

	return NewContext;
}

void UAbleAbilityContext::AllocateScratchPads()
{
	TSubclassOf<UAbleAbilityScratchPad> AbilityScratchPad = m_Ability->GetAbilityScratchPadClassBP(this);
	if (UClass* AbilityScratchPadClass = AbilityScratchPad.Get())
	{
		if (UAbleAbilityUtilitySubsystem* SubSystem = GetUtilitySubsystem())
		{
			m_AbilityScratchPad = SubSystem->FindOrConstructAbilityScratchPad(AbilityScratchPad);
			if (m_AbilityScratchPad)
			{
				// Reset the state.
				m_Ability->ResetAbilityScratchPad(m_AbilityScratchPad);
			}
		}
		else
		{
			// Couldn't get the Subsystem, fall back to the old system.
			m_AbilityScratchPad = NewObject<UAbleAbilityScratchPad>(this, AbilityScratchPadClass);
		}
	}

	const TArray<UAbleAbilityTask*>& Tasks = m_Ability->GetTasks();

	for (const UAbleAbilityTask* Task : Tasks)
	{
		if (!Task)
		{
			continue;
		}

		if (UAbleAbilityTaskScratchPad* ScratchPad = Task->CreateScratchPad(TWeakObjectPtr<UAbleAbilityContext>(this)))
		{
			m_TaskScratchPadMap.Add(Task->GetUniqueID(), ScratchPad);
		}
	}
}

void UAbleAbilityContext::ReleaseScratchPads()
{
	if (UAbleAbilityUtilitySubsystem* SubSystem = GetUtilitySubsystem())
	{
		if (m_AbilityScratchPad)
		{
			SubSystem->ReturnAbilityScratchPad(m_AbilityScratchPad);
			m_AbilityScratchPad = nullptr;
		}
		const TArray<UAbleAbilityTask*>& Tasks = m_Ability->GetTasks();
		for (const UAbleAbilityTask* Task : Tasks)
		{
			if (!Task)
        	{
        		continue;
        	}
		
			if (UAbleAbilityTaskScratchPad** ScratchPad = m_TaskScratchPadMap.Find(Task->GetUniqueID()))
			{
				Task->ResetScratchPad(*ScratchPad);	
			}
		}
		TMap<uint32, UAbleAbilityTaskScratchPad*>::TConstIterator Iter = m_TaskScratchPadMap.CreateConstIterator();
		for (; Iter; ++Iter)
		{
			SubSystem->ReturnTaskScratchPad(Iter->Value);
		}

		m_TaskScratchPadMap.Empty();
	}
}

void UAbleAbilityContext::UpdateTime(float DeltaTime)
{
	FPlatformMisc::MemoryBarrier();
	m_CurrentTime += DeltaTime;
	m_LastDelta = DeltaTime;
}

UAbleAbilityTaskScratchPad* UAbleAbilityContext::GetScratchPadForTask(const class UAbleAbilityTask* Task) const
{
	if (!IsValid(Task)) return nullptr;
	
	UAbleAbilityTaskScratchPad* const * ScratchPad = m_TaskScratchPadMap.Find(Task->GetUniqueID());
	if (ScratchPad && (*ScratchPad) != nullptr)
	{
		return *ScratchPad;
	}

	return nullptr;
}

UAbleAbilityComponent* UAbleAbilityContext::GetSelfAbilityComponent() const
{
	return m_AbilityComponent;
}

AActor* UAbleAbilityContext::GetSelfActor() const
{
	if (m_AbilityComponent)
	{
		return m_AbilityComponent->GetOwner();
	}

	return nullptr;
}

int32 UAbleAbilityContext::GetCurrentStackCount() const
{
	return m_StackCount;
}

int32 UAbleAbilityContext::GetCurrentLoopIteration() const
{
	return m_LoopIteration;
}

int32 UAbleAbilityContext::GetCurrentSegmentLoopIteration() const
{
	return m_SegmentLoopIteration;
}

void UAbleAbilityContext::SetStackCount(int32 Stack)
{
	int32 MaxStacks = GetAbility()->GetMaxStacks(this);
	m_StackCount = FMath::Clamp(Stack, 1, MaxStacks);
}

void UAbleAbilityContext::SetLoopIteration(int32 Loop)
{
	m_LoopIteration = FMath::Clamp(Loop, 0, (int32)GetAbility()->GetLoopMaxIterations(this));
}

void UAbleAbilityContext::SetSegmentLoopIteration(const int32 Loop, const int32 SegmentIndex)
{
	const FAbilitySegmentDefineData* Segment = GetAbility()->FindSegmentDefineDataByIndex(SegmentIndex);
	if (!Segment)
	{
		UE_LOG(LogAbleSP, Error, TEXT("SetSegmentLoopIteration failed, SegmentIndex %d Invalid !"), SegmentIndex);
		return;
	}
	
	m_SegmentLoopIteration = FMath::Clamp(Loop, 0, Segment->m_LoopMaxIterations);
}

float UAbleAbilityContext::GetCurrentTimeRatio() const
{
	if (m_Ability == nullptr) return .0f;
	
	return FMath::Clamp(m_CurrentTime / m_Ability->GetLength(m_ActiveSegmentIndex), 0.0f, 1.0f);
}

void UAbleAbilityContext::SetIntParameter(FName Id, int Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_IntParameters.Add(Id, Value);
}

void UAbleAbilityContext::SetFloatParameter(FName Id, float Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_FloatParameters.Add(Id, Value);
}

void UAbleAbilityContext::SetStringParameter(FName Id, const FString& Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_StringParameters.Add(Id, Value);
}

void UAbleAbilityContext::SetUObjectParameter(FName Id, UObject* Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_UObjectParameters.Add(Id, Value);
}

void UAbleAbilityContext::SetVectorParameter(FName Id, FVector Value)
{
	ABLE_RWLOCK_SCOPE_WRITE(m_ContextVariablesLock);
	m_VectorParameters.Add(Id, Value);
}

int UAbleAbilityContext::GetIntParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const int* var = m_IntParameters.Find(Id))
	{
		return *var;
	}
	return 0;
}

float UAbleAbilityContext::GetFloatParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const float* var = m_FloatParameters.Find(Id))
	{
		return *var;
	}
	return 0.0f;
}

UObject* UAbleAbilityContext::GetUObjectParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (m_UObjectParameters.Contains(Id)) // Find is being weird with double ptr.
	{
		return m_UObjectParameters[Id];
	}
	return nullptr;
}

FVector UAbleAbilityContext::GetVectorParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const FVector* var = m_VectorParameters.Find(Id))
	{
		return *var;
	}
	return FVector::ZeroVector;
}

const FString& UAbleAbilityContext::GetStringParameter(FName Id) const
{
	ABLE_RWLOCK_SCOPE_READ(m_ContextVariablesLock);
	if (const FString* var = m_StringParameters.Find(Id))
	{
		return *var;
	}
	static FString EmptyString;
	return EmptyString;
}

void UAbleAbilityContext::SetActiveSegmentIndex(const uint8 ActiveSegmentIndex)
{
	if (m_ActiveSegmentIndex != ActiveSegmentIndex)
	{
		m_ActiveSegmentIndex = ActiveSegmentIndex;
		if (GetAbility()->IsCompareWithHistory())
		{
			m_HistorySegments.Add(ActiveSegmentIndex);
		}
	}
}

UAbleAbilityUtilitySubsystem* UAbleAbilityContext::GetUtilitySubsystem() const
{
	if (UWorld* CurrentWorld = GetWorld())
	{
		UGameFeatureSystemManager* FeatureSystemManager = CurrentWorld->GetSubsystem<UGameFeatureSystemManager>();
		if (!FeatureSystemManager)
		{
			return nullptr;
		}
		if (UAbleAbilityUtilitySubsystem* UtilitySubsys = Cast<UAbleAbilityUtilitySubsystem>(UMoeGameLibrary::GetGameFeatureSystem(CurrentWorld,UAbleAbilityUtilitySubsystem::StaticClass())))
		{
			return UtilitySubsys;
		}
	}

	return nullptr;
}

void UAbleAbilityContext::Reset()
{
	m_AbilityActorStartLocation = FVector::ZeroVector;
	m_Ability = nullptr;
	m_AbilityComponent = nullptr;
	m_StackCount = 1;
	m_CurrentTime = 0.0f;
	m_LastDelta = 0.0f;
	m_ActiveSegmentIndex = 0;
	m_SegmentLoopIteration = 0;
	m_Owner.Reset();
	m_Instigator.Reset();
	m_TargetActors.Empty();
	m_TaskScratchPadMap.Empty();
	m_AbilityScratchPad = nullptr;
	m_AsyncHandle._Handle = 0;
	m_AsyncQueryTransform = FTransform::Identity;
	m_TargetLocation = FVector::ZeroVector;
	m_PredictionKey = 0;
	m_IntParameters.Empty();
	m_FloatParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
	m_VectorParameters.Empty();
	m_HistorySegments.Empty();
	m_TargetActorLocationSnap = FVector::ZeroVector;
	m_TargetActorRotationSnap = FRotator::ZeroRotator;
	AbilityUniqueID = 0;

	if (UAbleAbilityUtilitySubsystem* ContextSubsystem = GetUtilitySubsystem())
	{
		ContextSubsystem->ReturnContext(this);
	}
}

void UAbleAbilityContext::SetAbilityId(const int32 AbilityId)
{
	m_AbilityId = AbilityId;
}

int32 UAbleAbilityContext::GetAbilityUniqueID() const
{
	if (AbilityUniqueID > 0)
	{
		return AbilityUniqueID;
	}
	
	if (m_Ability)
	{
		return m_Ability->GetUniqueID();
	}
	
	return 0;
}

void UAbleAbilityContext::SetRandomSeed(const int32 RandomSeed)
{
	m_RandomSeed = RandomSeed;
}

float UAbleAbilityContext::GetRandomFloat(float Min, float Max) const
{
	return m_RandomStream.FRandRange(Min, Max);
}

const TArray<AActor*> UAbleAbilityContext::GetTargetActors() const
{
	// Blueprints don't like Weak Ptrs, so we have to do this fun copy.
	TArray<AActor*> ReturnVal;
	ReturnVal.Reserve(m_TargetActors.Num());

	for (const TWeakObjectPtr<AActor>& Actor : m_TargetActors)
	{
		if (Actor.IsValid())
		{
			ReturnVal.Add(Actor.Get());
		}
	}

	return ReturnVal;
}

void UAbleAbilityContext::AddTargetActors(const TArray<AActor*> Targets)
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = GetMutableTargetActors();

	for (AActor* Target : Targets)
	{
		if (IsValid(Target))
		{
			TargetActors.Add(Target);
		}
	}
}

void UAbleAbilityContext::RemoveTargetActors(const TArray<AActor*>& Targets)
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = GetMutableTargetActors();

	for (int i = Targets.Num() - 1; i >= 0; --i)
	{
		AActor* Target = Targets[i];
		if (IsValid(Target) && TargetActors.Contains(Target))
		{
			TargetActors.Remove(Target);
		}
	}
}

FAbleAbilityNetworkContext::FAbleAbilityNetworkContext()
{
	Reset();
}

FAbleAbilityNetworkContext::FAbleAbilityNetworkContext(const UAbleAbilityContext& Context)
	: m_Ability(Context.GetAbility()),
	  m_AbilityComponent(Context.GetSelfAbilityComponent()),
	  m_Owner(Context.GetOwner()),
	  m_Instigator(Context.GetInstigator()),
	  m_TargetActors(Context.GetTargetActors()),
	  m_CurrentStacks(Context.GetCurrentStackCount()),
	  m_TimeStamp(0.0f),
	  m_Result(EAbleAbilityTaskResult::Successful),
	  m_TargetLocation(Context.GetTargetLocation()),
	  m_PredictionKey(0),
	  m_IntParameters(Context.GetIntParameters()),
	  m_FloatParameters(Context.GetFloatParameters()),
	  m_StringParameters(Context.GetStringParameters()),
	  m_UObjectParameters(Context.GetUObjectParameters()),
	  m_VectorParameters(Context.GetVectorParameters()),
	  m_AbilityId(Context.GetAbilityId()),
	  m_RandomSeed(Context.GetRandomSeed()),
	  m_SegmentIndex(Context.GetActiveSegmentIndex()),
      m_TargetActorLocationSnap(Context.GetTargetActorLocationSnap()),
	  m_TargetActorRotationSnap(Context.GetTargetActorRotationSnap()),
      m_HighPingPawns(Context.GetHighPingPawns()),
	  m_AbilityUniqueID(Context.GetAbilityUniqueID())
{
	if (GEngine && GEngine->GetWorld())
	{
		m_TimeStamp = GEngine->GetWorld()->GetRealTimeSeconds();
	}

	// Instanced Replicated / Server Abilities don't have local prediction.
	if (// m_Ability->GetInstancePolicy() != EAbleInstancePolicy::NewInstanceReplicated &&
		m_Ability->GetAbilityRealm() != EAbleAbilityTaskRealm::ATR_Server)
	{
		// m_PredictionKey = Context.GetSelfAbilityComponent()->GetPredictionKey();
	}
}

FAbleAbilityNetworkContext::FAbleAbilityNetworkContext(const UAbleAbilityContext& Context,
                                                       EAbleAbilityTaskResult Result)
	: m_Ability(Context.GetAbility()),
	  m_AbilityComponent(Context.GetSelfAbilityComponent()),
	  m_Owner(Context.GetOwner()),
	  m_Instigator(Context.GetInstigator()),
	  m_TargetActors(Context.GetTargetActors()),
	  m_CurrentStacks(Context.GetCurrentStackCount()),
	  m_TimeStamp(0.0f),
	  m_Result(Result),
	  m_TargetLocation(Context.GetTargetLocation()),
	  m_PredictionKey(0),
	  m_IntParameters(Context.GetIntParameters()),
	  m_FloatParameters(Context.GetFloatParameters()),
	  m_StringParameters(Context.GetStringParameters()),
	  m_UObjectParameters(Context.GetUObjectParameters()),
	  m_VectorParameters(Context.GetVectorParameters()),
	  m_AbilityId(Context.GetAbilityId()),
	  m_RandomSeed(Context.GetRandomSeed()),
	  m_SegmentIndex(Context.GetActiveSegmentIndex()),
      m_TargetActorLocationSnap(Context.GetTargetActorLocationSnap()),
      m_TargetActorRotationSnap(Context.GetTargetActorRotationSnap()),
      m_HighPingPawns(Context.GetHighPingPawns()),
	  m_AbilityUniqueID(Context.GetAbilityUniqueID())
{
	if (GEngine && GEngine->GetWorld())
	{
		m_TimeStamp = GEngine->GetWorld()->GetRealTimeSeconds();
	}

	// Instanced Replicated / Server Abilities don't have local prediction.
	if (// m_Ability->GetInstancePolicy() != EAbleInstancePolicy::NewInstanceReplicated &&
		m_Ability->GetAbilityRealm() != EAbleAbilityTaskRealm::ATR_Server)
	{
		// m_PredictionKey = Context.GetSelfAbilityComponent()->GetPredictionKey();
	}
}

void FAbleAbilityNetworkContext::Reset()
{
	m_Ability = nullptr;
	m_AbilityComponent = nullptr;
	m_Owner.Reset();
	m_Instigator.Reset();
	m_TargetActors.Empty();
	m_CurrentStacks = 0;
	m_TimeStamp = 0.0f;
	m_Result = EAbleAbilityTaskResult::Successful;
	m_TargetLocation = FVector::ZeroVector;
	m_PredictionKey = 0;
	m_IntParameters.Empty();
	m_FloatParameters.Empty();
	m_StringParameters.Empty();
	m_UObjectParameters.Empty();
	m_VectorParameters.Empty();
	m_AbilityId = 0;
	m_RandomSeed = 0;
	m_SegmentIndex = 0;
	m_TargetActorLocationSnap = FVector::ZeroVector;
	m_TargetActorRotationSnap = FRotator::ZeroRotator;
	m_HighPingPawns.Empty();
	m_AbilityUniqueID = 0;
}

bool FAbleAbilityNetworkContext::IsValid() const
{
	return m_Ability != nullptr && m_AbilityComponent != nullptr;
}

FAbleAbilityNetworkContext FAbleAbilityNetworkContext::UpdateNetworkContext(
	const UAbleAbilityContext& UpdatedContext, const FAbleAbilityNetworkContext& Source)
{
	FAbleAbilityNetworkContext newContext(UpdatedContext);
	newContext.m_PredictionKey = Source.GetPredictionKey(); // Persist prediction key.
	
	return newContext;
}
