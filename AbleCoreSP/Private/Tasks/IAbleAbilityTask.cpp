// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/IAbleAbilityTask.h"
#include "AbleCoreSPPrivate.h"
#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "ableAbilityContext.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Serialization/ArchiveCountMem.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"
#include "Targeting/ableTargetingBase.h"

#if !(UE_BUILD_SHIPPING)
#include "ableSettings.h"
#include "ableAbilityUtilities.h"
#endif
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityDebug.h"
#include "ableSubSystem.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetDriver.h"
#include "Kismet/KismetSystemLibrary.h"


#define LOCTEXT_NAMESPACE "AbleAbilityTask"

// TODO: Move this to AbleUtils or some such.
namespace InstancedPropertyUtils
{
	struct FCPFUOArchive
	{
	public:
		FCPFUOArchive(bool bIncludeUntaggedDataIn)
			: bIncludeUntaggedData(bIncludeUntaggedDataIn)
			, TaggedDataScope(0)
		{}

	protected:
		FORCEINLINE void OpenTaggedDataScope() { ++TaggedDataScope; }
		FORCEINLINE void CloseTaggedDataScope() { --TaggedDataScope; }

		FORCEINLINE bool IsSerializationEnabled()
		{
			return bIncludeUntaggedData || (TaggedDataScope > 0);
		}

		bool bIncludeUntaggedData;
	private:
		int32 TaggedDataScope;
	};

	/* Serializes and stores property data from a specified 'source' object. Only stores data compatible with a target destination object. */
	struct FCPFUOWriter : public FObjectWriter, public FCPFUOArchive
	{
	public:
		/* Contains the source object's serialized data */
		TArray<uint8> SavedPropertyData;

	public:
		FCPFUOWriter(UObject* SrcObject, const UEngine::FCopyPropertiesForUnrelatedObjectsParams& Params)
			: FObjectWriter(SavedPropertyData)
			// if the two objects don't share a common native base class, then they may have different
			// serialization methods, which is dangerous (the data is not guaranteed to be homogeneous)
			// in that case, we have to stick with tagged properties only
			, FCPFUOArchive(true)
			, bSkipCompilerGeneratedDefaults(Params.bSkipCompilerGeneratedDefaults)
		{
			ArIgnoreArchetypeRef = true;
			ArNoDelta = !Params.bDoDelta;
			ArIgnoreClassRef = true;
			ArPortFlags |= Params.bCopyDeprecatedProperties ? PPF_UseDeprecatedProperties : PPF_None;

#if USE_STABLE_LOCALIZATION_KEYS
			if (GIsEditor && !(ArPortFlags & (PPF_DuplicateVerbatim | PPF_DuplicateForPIE)))
			{
				SetLocalizationNamespace(TextNamespaceUtil::EnsurePackageNamespace(SrcObject));
			}
#endif // USE_STABLE_LOCALIZATION_KEYS

			SrcObject->Serialize(*this);
		}

		//~ Begin FArchive Interface
		virtual void Serialize(void* Data, int64 Num) override
		{
			if (IsSerializationEnabled())
			{
				FObjectWriter::Serialize(Data, Num);
			}
		}

		virtual void MarkScriptSerializationStart(const UObject* Object) override { OpenTaggedDataScope(); }
		virtual void MarkScriptSerializationEnd(const UObject* Object) override { CloseTaggedDataScope(); }

#if WITH_EDITOR
		virtual bool ShouldSkipProperty(const class FProperty* InProperty) const override
		{
			static FName BlueprintCompilerGeneratedDefaultsName(TEXT("BlueprintCompilerGeneratedDefaults"));
			return bSkipCompilerGeneratedDefaults && InProperty->HasMetaData(BlueprintCompilerGeneratedDefaultsName);
		}
#endif
		//~ End FArchive Interface

	private:
		static UClass* FindNativeSuperClass(UObject* Object)
		{
			UClass* Class = Object->GetClass();
			for (; Class; Class = Class->GetSuperClass())
			{
				if ((Class->ClassFlags & CLASS_Native) != 0)
				{
					break;
				}
			}
			return Class;
		}

		bool bSkipCompilerGeneratedDefaults;
	};

	/* Responsible for applying the saved property data from a FCPFUOWriter to a specified object */
	struct FCPFUOReader : public FObjectReader, public FCPFUOArchive
	{
	public:
		FCPFUOReader(TArray<uint8>& DataSrc, UObject* DstObject)
			: FObjectReader(DataSrc)
			, FCPFUOArchive(true)
		{
			ArIgnoreArchetypeRef = true;
			ArIgnoreClassRef = true;

#if USE_STABLE_LOCALIZATION_KEYS
			if (GIsEditor && !(ArPortFlags & (PPF_DuplicateVerbatim | PPF_DuplicateForPIE)))
			{
				SetLocalizationNamespace(TextNamespaceUtil::EnsurePackageNamespace(DstObject));
			}
#endif // USE_STABLE_LOCALIZATION_KEYS

			DstObject->Serialize(*this);
		}

		//~ Begin FArchive Interface
		virtual void Serialize(void* Data, int64 Num) override
		{
			if (IsSerializationEnabled())
			{
				FObjectReader::Serialize(Data, Num);
			}
		}

		virtual void MarkScriptSerializationStart(const UObject* Object) override { OpenTaggedDataScope(); }
		virtual void MarkScriptSerializationEnd(const UObject* Object) override { CloseTaggedDataScope(); }
		// ~End FArchive Interface
	};
}


UAbleAbilityTask::UAbleAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_StartTime(0.0f),
	  m_EndTime(1.0f),
	  m_Inheritable(false),
	  m_Verbose(false),
	  m_Disabled(false),
      m_OverrideTaskColor(false),
	  m_TaskColor(FLinearColor::Black),
	  m_DynamicPropertyIdentifer(),
	  m_ResetForIteration(true),
	  m_OnceDuringIteration(false),
	  m_MustPassAllConditions(false),
#if WITH_EDITORONLY_DATA
	  m_Locked(false),
#endif
	  m_bAcrossSegment(false),
	  m_Segment(-1)
{
}

UAbleAbilityTask::~UAbleAbilityTask()
{

}

void UAbleAbilityTask::PostLoad()
{
    Super::PostLoad();

    // remove any broken references
    const int32 numDependencies = m_Dependencies.Num();
    m_Dependencies.RemoveAll([](const UAbleAbilityTask* Source) { return Source == nullptr; });
    if (numDependencies < m_Dependencies.Num())
    {
        UE_LOG(LogAbleSP, Warning, TEXT("UAbleAbilityTask.PostLoad() %s had %d NULL Dependencies."), *GetClass()->GetName(), m_Dependencies.Num() - numDependencies);
    }
}

void UAbleAbilityTask::PostInitProperties()
{
	Super::PostInitProperties();

	if (!m_TaskTargets.Num())
	{
		m_TaskTargets.Add(EAbleAbilityTargetType::ATT_Self);
	}
}

UWorld* UAbleAbilityTask::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

int32 UAbleAbilityTask::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UAbleAbilityTask::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
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

void UAbleAbilityTask::Clear()
{
	for (UAbleChannelingBase* Condition : m_Conditions)
	{
		if (IsValid(Condition)) Condition->MarkPendingKill();
	}
}

bool UAbleAbilityTask::CanStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float CurrentTime, float DeltaTime) const
{
	return (CurrentTime + DeltaTime >= GetStartTime()) && CheckConditions(Context) == ACR_Passed;
}

void UAbleAbilityTask::ResetScratchPad(UAbleAbilityTaskScratchPad* ScratchPad) const
{
	return;
}

void UAbleAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
#if !(UE_BUILD_SHIPPING)
	if (FAbleAbilityDebug::IsVerboseDebug() && Context.IsValid())
	{
		if (AActor* Owner = Context->GetOwner())
		{
			if (const UWorld* World = Context->GetOwner()->GetWorld())
			{
				PrintVerbose(Context, FString::Printf(TEXT("OnTaskStart called for Task %s at time %f."),
					*GetName(), Context->GetCurrentTime()));
			}
		}
	}
#endif
}

void UAbleAbilityTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
#if !(UE_BUILD_SHIPPING)
	if (FAbleAbilityDebug::IsVerboseDebug() && Context.IsValid())
	{
		PrintVerbose(Context, FString::Printf(TEXT("OnTaskTick called for Task %s at time %2.2f. Delta Time = %1.5f"), *GetName(), Context->GetCurrentTime(), deltaTime));
	}
#endif
}

bool UAbleAbilityTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return Context.IsValid() ? Context->GetCurrentTime() > GetEndTime() : true;
}

bool UAbleAbilityTask::IsDoneActually(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return Context.IsValid() ? Context->GetCurrentTime() >= GetDuration() + GetActualStartTime() : true;
}

void UAbleAbilityTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
#if !(UE_BUILD_SHIPPING)
	if (FAbleAbilityDebug::IsVerboseDebug() && Context.IsValid())
	{
		if (AActor* Owner = Context->GetOwner())
		{
			if (const UWorld* World = Context->GetOwner()->GetWorld())
			{
				PrintVerbose(Context, FString::Printf(TEXT("OnTaskEnd called for Task %s at time %f. Task Result = %s."),
					*GetName(), Context->GetCurrentTime(), *FAbleSPLogHelper::GetTaskResultEnumAsString(result)));
			}
		}
	}
#endif
}

float UAbleAbilityTask::GetCurrentDurationAsRatio(const UAbleAbilityContext* Context) const
{
	const float currentDuration = FMath::Max(Context->GetCurrentTime() - GetStartTime(),0.0f);
	return FMath::Clamp(currentDuration / GetDuration(), 0.0f, 1.0f);
}

bool UAbleAbilityTask::IsDisabled() const
{
	return m_Disabled; // ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Disabled);
}

bool UAbleAbilityTask::IsValidForNetMode(ENetMode NetMode, const AActor* RefActor, const UAbleAbilityContext* Context) const
{
	return IsValidForNetwork(RefActor, GetTaskRealm());
}

bool UAbleAbilityTask::IsValidForNetwork(const AActor* RefActor, const EAbleAbilityTaskRealm TaskRealm) const
{
	ENetMode NetMode = RefActor->GetNetMode();
	
	if (NetMode == NM_Standalone)
	{
		return true;
	}

	switch (TaskRealm)
	{
	case EAbleAbilityTaskRealm::ATR_Client:
		{
			return NetMode == NM_Client || NetMode == NM_ListenServer;
		}
		break;
	case EAbleAbilityTaskRealm::ATR_Server:
		{
			return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
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

FName UAbleAbilityTask::GetDynamicDelegateName( const FString& DisplayName ) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_") + DisplayName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAbleAbilityTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Disabled, TEXT("Disabled"));
}

#if WITH_EDITOR

FText UAbleAbilityTask::GetRichTextTaskSummary() const
{
	FTextBuilder SummaryString;

	FNumberFormattingOptions timeFormat;
	timeFormat.MaximumFractionalDigits = 2;
	timeFormat.MinimumFractionalDigits = 2;
	
	SummaryString.AppendLine(FText::FromString(FString::Printf(TEXT("%s (%s)"), *GetTaskName().ToString(), *GetName())));

    if (!m_DynamicPropertyIdentifer.IsEmpty())
    {
        SummaryString.AppendLine(FString::Format(TEXT("Dynamic Identifier: {0}"), { *m_DynamicPropertyIdentifer }));
    }

	SummaryString.AppendLineFormat(LOCTEXT("AbleAbilityTaskSummaryFormat", "\t - Start Time <a id=\"AbleTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_StartTime\" MinValue=\"0.0\">{0}</>"),
		FText::AsNumber(GetStartTime(), &timeFormat));

	if (IsSingleFrame())
	{
		SummaryString.AppendLine(LOCTEXT("AbleAbilityTaskSummarySingleFrame", "\t - End Time: Single Frame"));
	}
	else
	{
		SummaryString.AppendLineFormat(LOCTEXT("AbleAbilityTaskSummaryEndTimeFmt", "\t - End Time <a id=\"AbleTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_EndTime\" MinValue=\"{0}\">{1}</>"),
			FText::AsNumber(GetStartTime(), &timeFormat),
			FText::AsNumber(GetEndTime(), &timeFormat));
	}

	if (GetTaskDependencies().Num())
	{
		SummaryString.AppendLine(LOCTEXT("AbleAbilityTaskDependencies",  "\t - Dependencies: "));
		FText DependencyFmt = LOCTEXT("AbleAbilityTaskDependencyFmt", "\t\t - <a id=\"AbleTextDecorators.TaskDependency\" style=\"RichText.Hyperlink\" Index=\"{0}\">{1}</>");
		const TArray<const UAbleAbilityTask*>& Dependencies = GetTaskDependencies();
		for (int i = 0; i < Dependencies.Num(); ++i)
		{
			if (!Dependencies[i])
			{
				continue;
			}

			SummaryString.AppendLineFormat(DependencyFmt, i, GetTaskDependencies()[i]->GetTaskName());
		}
	}
	
	if (m_Disabled)
	{
		SummaryString.AppendLine(LOCTEXT("AbleAbilityTaskDisabled", "(DISABLED)"));
	}
	else if (m_DisabledDelegate.IsBound())
	{
		SummaryString.AppendLine(LOCTEXT("AbleAbilityTaskDisabled", "(Dynamic Disable)"));
	}

	if (!Description.IsEmpty())
	{
		SummaryString.AppendLineFormat(LOCTEXT("AbleAbilityTaskDescriptionFormat", "\t - Description: {0}"), Description);
	}
	
	return SummaryString.ToText();
}

float UAbleAbilityTask::GetEstimatedTaskCost() const
{
	float EstimatedCost = 0.0f;
	if (!IsSingleFrame())
	{
		EstimatedCost += IsAsyncFriendly() ? ABLETASK_EST_ASYNC : ABLETASK_EST_SYNC;
	}
	EstimatedCost += NeedsTick() ? ABLETASK_EST_TICK : 0.0f;

	return EstimatedCost;
}

void UAbleAbilityTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	m_StartTime = FMath::Max(0.0f, m_StartTime);
	m_EndTime = FMath::Max(m_EndTime, m_StartTime + 0.001f);

#if WITH_EDITORONLY_DATA
	m_OnTaskPropertyModified.Broadcast(*this, PropertyChangedEvent);
#endif
}

void UAbleAbilityTask::AddDependency(const UAbleAbilityTask* Task)
{
	m_Dependencies.AddUnique(Task);

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_Dependencies"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

void UAbleAbilityTask::RemoveDependency(const UAbleAbilityTask* Task)
{
	m_Dependencies.Remove(Task);

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_Dependencies"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

bool UAbleAbilityTask::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();
	// Make sure our flags match our packages.
	SetFlags(GetOuter()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}

void UAbleAbilityTask::CopyProperties(UAbleAbilityTask& source)
{
	UEngine::FCopyPropertiesForUnrelatedObjectsParams copyParams;
	copyParams.bDoDelta = true;
	copyParams.bCopyDeprecatedProperties = true;
	copyParams.bSkipCompilerGeneratedDefaults = true;
	copyParams.bClearReferences = false;
	copyParams.bNotifyObjectReplacement = true;
	InstancedPropertyUtils::FCPFUOWriter FCWriter(&source, copyParams);
	InstancedPropertyUtils::FCPFUOReader FCReader(FCWriter.SavedPropertyData, this);
}

void UAbleAbilityTask::GetCompactData(FAbleCompactTaskData& output)
{
	UClass* OurClass = GetClass();
	output.TaskClass = OurClass;
	for (const UAbleAbilityTask* Dependency : m_Dependencies)
	{
		output.Dependencies.Add(TSoftObjectPtr<const UAbleAbilityTask>(Dependency));
	}

	output.DataBlob.Empty();

	UEngine::FCopyPropertiesForUnrelatedObjectsParams copyParams;
	copyParams.bDoDelta = true;
	copyParams.bCopyDeprecatedProperties = true;
	copyParams.bSkipCompilerGeneratedDefaults = true;
	copyParams.bClearReferences = false;
	copyParams.bNotifyObjectReplacement = true;
	InstancedPropertyUtils::FCPFUOWriter FCWriter(this, copyParams);

	output.DataBlob.Append(FCWriter.SavedPropertyData);
}

void UAbleAbilityTask::LoadCompactData(FAbleCompactTaskData& input)
{
	if (!input.DataBlob.Num())
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Tried to load Task [%s] saved data - but it was empty!\n"), *GetTaskName().ToString());
		return;
	}

	InstancedPropertyUtils::FCPFUOReader FCReader(input.DataBlob, this);

	m_Dependencies.Empty();
	for (const TSoftObjectPtr<const UAbleAbilityTask> Dependency : input.Dependencies)
	{
		if (const UAbleAbilityTask* Task = Dependency.Get())
		{
			m_Dependencies.Add(Task);
		}
		else
		{
			UE_LOG(LogAbleSP, Warning, TEXT("Could not get dependency task from soft path %s. Please re-add the dependency manually.\n"), *Dependency.ToString());
		}
	}
}

#endif

void UAbleAbilityTask::SetSegment(int32 Segment)
{
	m_Segment = Segment;

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_Segment"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

void UAbleAbilityTask::OnAbilityPlayRateChanged(const UAbleAbilityContext* Context, float NewPlayRate)
{
}

void UAbleAbilityTask::GetActorsForTask(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<TWeakObjectPtr<AActor>>& OutActorArray) const
{
	verify(m_TaskTargets.Num() != 0 && Context.IsValid());

	OutActorArray.Empty();
	for (TEnumAsByte<EAbleAbilityTargetType> target : m_TaskTargets)
	{
		switch (target)
		{
			case EAbleAbilityTargetType::ATT_Location:
			case EAbleAbilityTargetType::ATT_Camera:
			case EAbleAbilityTargetType::ATT_Self:
			{
				AActor* SelfActor = Context->GetSelfActor();

				if (IsTaskValidForActor(SelfActor))
				{
					OutActorArray.Add(SelfActor);
				}
			}
			break;
			case EAbleAbilityTargetType::ATT_Owner:
			{
				AActor* Owner = Context->GetOwner();
				if (IsTaskValidForActor(Owner))
				{
					OutActorArray.Add(Owner);
				}
			}
			break;
			case EAbleAbilityTargetType::ATT_Instigator:
			{
				AActor* Instigator = Context->GetInstigator();
				if (IsTaskValidForActor(Instigator))
				{
					OutActorArray.Add(Instigator);
				}
			}
			break;
			case EAbleAbilityTargetType::ATT_TargetActor:
			{
				const TArray<TWeakObjectPtr<AActor>>& UnfilteredTargets = Context->GetTargetActorsWeakPtr();
				for (const TWeakObjectPtr<AActor>& Target : UnfilteredTargets)
				{
					if (IsTaskValidForActor(Target.Get()))
					{
						OutActorArray.Add(Target);
					}
				}
			}
			break;
			default:
			{
				checkNoEntry();
			}
			break;
		}
	}
}

void UAbleAbilityTask::GetActorsForTaskBP(const UAbleAbilityContext* Context, TArray<AActor*>& OutActorArray) const
{
	TArray<TWeakObjectPtr<AActor>> TempArray;
	GetActorsForTask(TWeakObjectPtr<const UAbleAbilityContext>(Context), TempArray);

	// BP's don't support Containers of WeakObjectPtrs so we have to do a fun copy here... :/
	OutActorArray.Empty();
	for (TWeakObjectPtr<AActor>& FoundActor : TempArray)
	{
		if (FoundActor.IsValid())
		{
			OutActorArray.Add(FoundActor.Get());
		}
	}
}

AActor* UAbleAbilityTask::GetSingleActorFromTargetType(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
																								EAbleAbilityTargetType TargetType, int32 TargetIndex) const
{
	check(Context.IsValid());

	switch (TargetType)
	{
		case EAbleAbilityTargetType::ATT_Location:
		case EAbleAbilityTargetType::ATT_Camera:
		case EAbleAbilityTargetType::ATT_Self:
		{
			if (AActor* Actor = Context->GetSelfActor())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAbleAbilityTargetType::ATT_Instigator:
		{
			if (AActor* Actor = Context->GetInstigator())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAbleAbilityTargetType::ATT_Owner:
		{
			if (AActor* Actor = Context->GetOwner())
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		case EAbleAbilityTargetType::ATT_TargetActor:
		{
			if (!Context->GetTargetActors().Num())
			{
				return nullptr;
			}

			// 外部传入需要获取的目标Index，并Clamp到0和目标数组长度之间，保证当配置错误时能返回数组的最后一个单位
			int32 TargetActorIndex = FMath::Clamp(TargetIndex, 0, Context->GetTargetActors().Num() - 1);

			if (AActor* Actor = Context->GetTargetActors()[TargetActorIndex])
			{
				if (IsTaskValidForActor(Actor))
				{
					return Actor;
				}
			}
			return nullptr;
		}
		break;
		default:
		{
		}
		break;
	}

	return nullptr;
}

AActor* UAbleAbilityTask::GetSingleActorFromTargetTypeBP(const UAbleAbilityContext* Context,
	EAbleAbilityTargetType TargetType, int32 TargetIndex) const
{
	return GetSingleActorFromTargetType(Context, TargetType, TargetIndex);
}

bool UAbleAbilityTask::IsTaskValidForActor(const AActor* Actor) const
{
	if (!Actor || !Actor->GetWorld() || Actor->IsPendingKillPending() )
	{
		return false;
	}

	bool ActorLocallyControlled = false;
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		ActorLocallyControlled = Pawn->IsLocallyControlled();
	}

	ENetMode WorldNetMode = Actor->GetWorld()->GetNetMode();
	if (WorldNetMode == ENetMode::NM_Standalone)
	{
		// Standalone / Offline game, no need to worry about networking.
		return true;
	}
	else
	{
		switch (GetTaskRealm())
		{
			case EAbleAbilityTaskRealm::ATR_Client:
			{
				// Client tasks are safe to run on any proxy/auth/etc.
				return WorldNetMode == NM_Client || ActorLocallyControlled || WorldNetMode == NM_ListenServer;
			}
			break;
			case EAbleAbilityTaskRealm::ATR_Server:
			{
				return WorldNetMode != NM_Client;
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
	}

	return false;
}

float UAbleAbilityTask::GetTimePercent(const UAbleAbilityContext* Context) const
{
	const float Numerator = Context->GetCurrentTime() - GetStartTime();
	const float Denominator = GetEndTime() - GetStartTime();

	if (Denominator <= 0) return 1.f;
	return Numerator / Denominator;
}

EAbleConditionResults UAbleAbilityTask::CheckConditions(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (GetConditions().Num() == 0) return EAbleConditionResults::ACR_Passed;

	// limit condition checking in task time range [TaskStartTime -> TaskEndTime]
	if (IsDone(Context)) return EAbleConditionResults::ACR_Failed;
	
#if WITH_EDITOR
	if (UWorld* World = Context->GetWorld())
	{
		if (World->WorldType == EWorldType::Type::EditorPreview)
			return EAbleConditionResults::ACR_Passed;
	}
#endif
	EAbleConditionResults ConditionResult = EAbleConditionResults::ACR_Passed;
	for (const UAbleChannelingBase* Condition : GetConditions())
	{
		if (!Condition) continue;
		
		ConditionResult = Condition->GetConditionResult(const_cast<UAbleAbilityContext&>(*Context.Get()));

		if (ConditionResult == EAbleConditionResults::ACR_Passed && !MustPassAllConditions())
		{
			// One condition passed.
			return EAbleConditionResults::ACR_Passed;
		}
		else if (ConditionResult == EAbleConditionResults::ACR_Failed && MustPassAllConditions())
		{
#if !(UE_BUILD_SHIPPING)
			if (FAbleAbilityDebug::IsVerboseDebug() && Context.IsValid())
			{
				UE_LOG(LogAbleSP, Log, TEXT("[SPAbility] Ability [%d] Task [%s] condition [%s] check result failed!"),
						Context->GetAbilityId(), *GetNameSafe(this), *GetNameSafe(Condition))
			}
#endif
			return EAbleConditionResults::ACR_Failed;
		}
		else if (ConditionResult == EAbleConditionResults::ACR_Failed)
		{
#if !(UE_BUILD_SHIPPING)
			if (FAbleAbilityDebug::IsVerboseDebug() && Context.IsValid())
			{
				UE_LOG(LogAbleSP, Log, TEXT("[SPAbility] Ability [%d] Task [%s] condition [%s] check result failed!"),
						Context->GetAbilityId(), *GetNameSafe(this), *GetNameSafe(Condition))
			}
#endif
		}
	}

	return ConditionResult;
}

#if !(UE_BUILD_SHIPPING)

void UAbleAbilityTask::PrintVerbose(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FString& Output) const
{
	static const USPAbleSettings* Settings = nullptr;
	if (!Settings)
	{
		Settings = GetDefault<USPAbleSettings>();
	}

	UWorld* World = nullptr;

#if WITH_EDITOR
	// The play world needs to handle these commands if it exists
	if (GIsEditor && GEditor->PlayWorld && !GIsPlayInEditorWorld)
	{
		World = GEditor->PlayWorld;
	}
#endif

	ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();
	if (Player)
	{
		UWorld* PlayerWorld = Player->GetWorld();
		if (!World)
		{
			World = PlayerWorld;
		}
	}

#if WITH_EDITOR
	if (!World && GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if (World)
    {
		AActor* Owner = Context->GetOwner();
		FString AbilityDisplayName = Context->GetAbility() != nullptr ? Context->GetAbility()->GetAbilityName() : TEXT("NULL ABILITY");

		FString OwnerWorldName = Owner != nullptr ? FAbleSPLogHelper::GetWorldName(Context->GetOwner()->GetWorld()) : TEXT("NULL OWNER");
		UKismetSystemLibrary::PrintString(World,
			FString::Format(TEXT("(World {0}) {1} - {2}"), { OwnerWorldName, AbilityDisplayName, Output }),
			Settings ? Settings->GetEchoVerboseToScreen() : false,
			Settings ? Settings->GetLogVerbose() : true,
			FLinearColor(0.0, 0.66, 1.0),
			Settings ? Settings->GetVerboseScreenLifetime() : 2.0f);
	}
	else
	{
		FString AbilityDisplayName = Context->GetAbility() != nullptr ? Context->GetAbility()->GetAbilityName() : TEXT("NULL ABILITY");
		FString OwnerWorldName = "DedicatedServer World";  // Owner != nullptr ? FAbleSPLogHelper::GetWorldName(Context->GetOwner()->GetWorld()) : TEXT("NULL OWNER");
		UE_LOG(LogAbleSP, Log, TEXT("(World {%s}) {%s} - {%s}"), *OwnerWorldName, *AbilityDisplayName, *Output);
	}
}

#if WITH_EDITOR

EDataValidationResult UAbleAbilityTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

#endif

#endif

#undef LOCTEXT_NAMESPACE