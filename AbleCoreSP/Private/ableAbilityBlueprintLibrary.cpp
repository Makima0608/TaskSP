// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityBlueprintLibrary.h"

#include "ableAbility.h"
#include "ableAbilityContext.h"
#include "ableAbilityComponent.h"
#include "ableAbilityDebug.h"
#include "AbleCoreSPPrivate.h"
#include "ableSubSystem.h"
#include "MoeGameplay/Core/MoeGameLibrary.h"
#include "MoeGameplay/Core/MoeGameLibrary.h"
#include "Tasks/SPAbilityTask.h"

#define LOCTEXT_NAMESPACE "AbleCore"

UAbleAbility* UAbleAbilityBlueprintLibrary::GetAbilityObjectFromClass(TSubclassOf<UAbleAbility> Class)
{
	if (UClass* AbilityClass = Class.Get())
	{
		if (UAbleAbility* AbilityObject = AbilityClass->GetDefaultObject<UAbleAbility>())
		{
			return AbilityObject;
		}
		else
		{
			UE_LOG(LogAbleSP, Error, TEXT("GetAbilityObjectFromClass failed to get default object for class %s"), *Class->GetName());
		}
	}
	else
	{
		UE_LOG(LogAbleSP, Error, TEXT("GetAbilityObjectFromClass was passed a null/invalid class. Check your parameters!"));
	}


	return nullptr;
}

EAbleAbilityStartResult UAbleAbilityBlueprintLibrary::ActivateAbility(UAbleAbilityContext* Context)
{
	// 禁止外部调用该方法，请使用SPAbilityComponent::TryActivateAbility()
	return EAbleAbilityStartResult::FailedCustomCheck;
	
	/*if (Context)
	{
		if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
		{
			return AbilityComponent->ActivateAbility(Context);
		}
		else
		{
			UE_LOG(LogAbleSP, Error, TEXT("ActivateAbility was passed a context with a null ability component. Check your parameters!"));
		}
	}
	else
	{
		UE_LOG(LogAbleSP, Error, TEXT("ActivateAbility was passed a null context. Check your parameters!"));
	}

	return EAbleAbilityStartResult::InternalSystemsError;*/
}

UAbleAbilityContext* UAbleAbilityBlueprintLibrary::CreateAbilityContext(const UAbleAbility* Ability, UAbleAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator)
{
	return UAbleAbilityContext::MakeContext(Ability, AbilityComponent, Owner, Instigator);
}

UAbleAbilityContext* UAbleAbilityBlueprintLibrary::CreateAbilityContextWithLocation(const UAbleAbility* Ability, UAbleAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator, FVector Location)
{
	return UAbleAbilityContext::MakeContext(Ability, AbilityComponent, Owner, Instigator, Location);
}

bool UAbleAbilityBlueprintLibrary::IsSuccessfulStartResult(EAbleAbilityStartResult Result)
{
	// Async Processing is technically a Success as we can't determine whether it passed or failed for a frame.
	return Result == EAbleAbilityStartResult::Success ||
		Result == EAbleAbilityStartResult::AsyncProcessing;
}

bool UAbleAbilityBlueprintLibrary::GetDrawCollisionQueries()
{
#if !UE_BUILD_SHIPPING
	return FAbleAbilityDebug::ShouldDrawQueries();
#else
	return false;
#endif
}

bool UAbleAbilityBlueprintLibrary::SetDrawCollisionQueries(bool Enable)
{
#if !UE_BUILD_SHIPPING
	FAbleAbilityDebug::EnableDrawQueries(Enable);

	return FAbleAbilityDebug::ShouldDrawQueries();
#else
	return false;
#endif
}

USPAbilityTaskScratchPad* UAbleAbilityBlueprintLibrary::CreateCustomScratchPad(UAbleAbilityContext* Context, TSubclassOf<USPAbilityTaskScratchPad> ScratchPadClass)
{
	if (*ScratchPadClass)
	{
		return NewObject<USPAbilityTaskScratchPad>(Context, *ScratchPadClass);
	}

	return nullptr;
}

TEnumAsByte<ETraceTypeQuery> UAbleAbilityBlueprintLibrary::GetQueryType(const UAbleAbilityContext* Context, ESPAbleTraceType TraceType)
{
	if (!Context) return ETraceTypeQuery::TraceTypeQuery_MAX;
	
	if (UAbleAbilityComponent* Component = Context->GetSelfAbilityComponent())
	{
		return Component->GetQueryType(TraceType);
	}
	
	return ETraceTypeQuery::TraceTypeQuery_MAX;
}

TArray<TEnumAsByte<ETraceTypeQuery>> UAbleAbilityBlueprintLibrary::GetQueryTypePresent(const UAbleAbilityContext* Context,
                                                                          TEnumAsByte<EAbleChannelPresent> Present, TArray<TEnumAsByte<ESPAbleTraceType>>& TraceTypes)
{
	TArray<TEnumAsByte<ETraceTypeQuery>> Ret;

	for (const TEnumAsByte<ESPAbleTraceType> TraceType : TraceTypes)
	{
		if (TraceType != ESPAbleTraceType::AT_None)
		{
			Ret.AddUnique(GetQueryType(Context, TraceType));
		}
	}
	
	for (const TEnumAsByte<ESPAbleTraceType> ChannelInPresent : GetTraceTypeDataTable(Context, Present))
	{
		Ret.AddUnique(GetQueryType(Context, ChannelInPresent));
	}

	return Ret;
}

TEnumAsByte<ECollisionChannel> UAbleAbilityBlueprintLibrary::GetCollisionChannelFromTraceType(const UAbleAbilityContext* Context, const ESPAbleTraceType TraceType)
{
	if (!Context) return ECollisionChannel::ECC_MAX;
	
	if (UAbleAbilityComponent* Component = Context->GetSelfAbilityComponent())
	{
		return Component->GetCollisionChannelFromTraceType(TraceType);
	}
	
	return ECollisionChannel::ECC_MAX;
}

TEnumAsByte<EObjectTypeQuery> UAbleAbilityBlueprintLibrary::GetObjectTypeFromTraceType(
	const UAbleAbilityContext* Context, const ESPAbleTraceType TraceType)
{
	TEnumAsByte<ECollisionChannel> CollisionChannel = GetCollisionChannelFromTraceType(Context, TraceType);
	
	return UEngineTypes::ConvertToObjectType(CollisionChannel);
}

TArray<TEnumAsByte<ECollisionChannel>> UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(const UAbleAbilityContext* Context,
                                                                                   const TEnumAsByte<EAbleChannelPresent> Present, const TArray<TEnumAsByte<ESPAbleTraceType>> TraceTypes)
{
	TArray<TEnumAsByte<ECollisionChannel>> Ret;

	for (const TEnumAsByte<ESPAbleTraceType> TraceType : TraceTypes)
	{
		if (TraceType != ESPAbleTraceType::AT_None)
		{
			Ret.AddUnique(GetCollisionChannelFromTraceType(Context, TraceType));
		}
	}

	for (const TEnumAsByte<ESPAbleTraceType> ChannelInPresent : GetTraceTypeDataTable(Context, Present))
	{
		Ret.AddUnique(GetCollisionChannelFromTraceType(Context, ChannelInPresent));
	}

	return Ret;
}

TArray<TEnumAsByte<EObjectTypeQuery>> UAbleAbilityBlueprintLibrary::GetObjectTypesPresent(
	const UAbleAbilityContext* Context, TEnumAsByte<EAbleChannelPresent> Present,
	const TArray<TEnumAsByte<ESPAbleTraceType>> TraceTypes)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> Ret;

	for (const TEnumAsByte<ESPAbleTraceType> TraceType : TraceTypes)
	{
		if (TraceType != ESPAbleTraceType::AT_None)
		{
			Ret.AddUnique(GetObjectTypeFromTraceType(Context, TraceType));
		}
	}

	for (const TEnumAsByte<ESPAbleTraceType> ChannelInPresent : GetTraceTypeDataTable(Context, Present))
	{
		Ret.AddUnique(GetObjectTypeFromTraceType(Context, ChannelInPresent));
	}
	
	return Ret;
}

TArray<TEnumAsByte<ESPAbleTraceType>> UAbleAbilityBlueprintLibrary::GetTraceTypeDataTable(const UObject* WorldContext, const TEnumAsByte<EAbleChannelPresent> Present)
{
	TArray<TEnumAsByte<ESPAbleTraceType>> Ret;
	if (!WorldContext) return Ret;
	if (Present == EAbleChannelPresent::ACP_NonePresent) return Ret;
	
	const UDataTable* DataTable = nullptr;
	if (const UWorld* CurrentWorld = WorldContext->GetWorld())
	{
		if (UAbleAbilityUtilitySubsystem* UtilitySubsystem = Cast<UAbleAbilityUtilitySubsystem>(UMoeGameLibrary::GetGameFeatureSystem(CurrentWorld,UAbleAbilityUtilitySubsystem::StaticClass()))
)
		{
			DataTable = UtilitySubsystem->TryGetChannelPresentDataTable();
		}
	}
	
	if (DataTable)
	{
		const FName RowName = UEnum::GetValueAsName(Present);
		const FAbleChannelPresent* DataSetRow = DataTable->FindRow<FAbleChannelPresent>(RowName, TEXT("AbleChannelPresent"));
		if (DataSetRow)
		{
			Ret.Append(DataSetRow->Channels);
		}
		else
		{
			UE_LOG(LogAbleSP, Warning, TEXT("[SPAbility] UAbleAbilityBlueprintLibrary::GetTraceTypeDataTable failed with Present [%s]"), *UEnum::GetValueAsString(Present));
		}
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("[SPAbility] UAbleAbilityBlueprintLibrary::GetTraceTypeDataTable Cannot Load Present datatable !"));
	}

	return Ret;
}

bool UAbleAbilityBlueprintLibrary::CheckBranchCond(UAbleAbilityContext* Context,
	const TArray<UAbleChannelingBase*>& Conditions, const bool bMustPassAllConditions)
{
	bool bRet;
	if (bMustPassAllConditions)
	{
		bRet = true;
		for (auto& Cond : Conditions)
		{
			if (Cond == nullptr)
				continue;
			
			if (Cond->GetConditionResult(*Context) == EAbleConditionResults::ACR_Failed)
			{
				bRet = false;
				break;
			}
		}
	}
	else
	{
		bRet = Conditions.IsEmpty();
		for (auto& Cond : Conditions)
		{
			if (Cond == nullptr)
				continue;
				
			if (Cond->GetConditionResult(*Context) == EAbleConditionResults::ACR_Passed)
			{
				bRet = true;
				break;
			}
		}
	}
	
	return bRet;
}

bool UAbleAbilityBlueprintLibrary::CheckBranchCondWithBranchData(UAbleAbilityContext* Context,
	const FAbilitySegmentBranchData& BranchData)
{
	return CheckBranchCond(Context, BranchData.Conditions, BranchData.bMustPassAllConditions);
}

bool UAbleAbilityBlueprintLibrary::IsValidForNetwork(const AActor* RefActor, const EAbleAbilityTaskRealm Realm)
{
	if (!IsValid(RefActor))
	{
		return false;
	}
	
	ENetMode NetMode = RefActor->GetNetMode();
	
	if (NetMode == NM_Standalone)
	{
		return true;
	}

	switch (Realm)
	{
		case EAbleAbilityTaskRealm::ATR_Client:
		{
			return NetMode == NM_Client || NetMode == NM_ListenServer;
		}
		case EAbleAbilityTaskRealm::ATR_Server:
		{
			return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
		}
		case EAbleAbilityTaskRealm::ATR_ClientAndServer:
		{
			return true;
		}
		default:
			break;
	}

	return false;
}

AActor* UAbleAbilityBlueprintLibrary::GetSingleActorFromTargetType(const UAbleAbilityContext* Context,
                                                                   const EAbleAbilityTargetType TargetType, int32 TargetIndex)
{
	if (!Context)
	{
		return nullptr;
	}
	
	switch (TargetType)
	{
	case EAbleAbilityTargetType::ATT_Location:
	case EAbleAbilityTargetType::ATT_Camera:
	case EAbleAbilityTargetType::ATT_Self:
		{
			if (AActor* Actor = Context->GetSelfActor())
			{
				if (IsValid(Actor))
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
				if (IsValid(Actor))
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
				if (IsValid(Actor))
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
				if (IsValid(Actor))
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

void UAbleAbilityBlueprintLibrary::SetComponentTickEnableImplicitRef(const FName ReferenceKey,
	UActorComponent* Component, const UAbleAbilityContext* Context)
{
	if (!Context || !Component) return;
	
	if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		int Result;
		if (const int* ReferenceCountPtr = AbilityComponent->GetReferenceCountMap().Find(ReferenceKey))
		{
			Result = *ReferenceCountPtr + 1;
			AbilityComponent->GetReferenceCountMap()[ReferenceKey] = Result;
		}
		else
		{
			Result = 1;
			AbilityComponent->GetReferenceCountMap().Add(ReferenceKey, Result);
		}
		
		Component->SetComponentTickEnabled(true);
		UE_LOG(LogAbleSP, Log, TEXT("Ability[%s] SetComponentTickEnableImplicitRef[%s] for[%s] Succeed, RefCount[%d]"),
			*Context->GetAbility()->GetAbilityName(), *ReferenceKey.ToString(), *GetNameSafe(Component->GetOwner()), Result)
	}
}

void UAbleAbilityBlueprintLibrary::SetComponentTickDisableImplicitRef(const FName ReferenceKey,
	UActorComponent* Component, const UAbleAbilityContext* Context)
{
	if (!Context || !Component) return;
	
	if (UAbleAbilityComponent* AbilityComponent = Context->GetSelfAbilityComponent())
	{
		if (int* ReferenceCountPtr = AbilityComponent->GetReferenceCountMap().Find(ReferenceKey))
		{
			(*ReferenceCountPtr)--;
			AbilityComponent->GetReferenceCountMap()[ReferenceKey] = *ReferenceCountPtr;
			if (*ReferenceCountPtr <= 0)
			{
				Component->SetComponentTickEnabled(false);
				UE_LOG(LogAbleSP, Log, TEXT("Ability[%s] SetComponentTickDisableImplicitRef[%s] for[%s] Succeed, RefCount[%d]"),
					*Context->GetAbility()->GetAbilityName(), *ReferenceKey.ToString(), *GetNameSafe(Component->GetOwner()), *ReferenceCountPtr)
			}
			else
			{
				UE_LOG(LogAbleSP, Log, TEXT("Ability[%s] SetComponentTickDisableImplicitRef[%s] for[%s], Remain RefCount[%d]"),
					*Context->GetAbility()->GetAbilityName(), *ReferenceKey.ToString(), *GetNameSafe(Component->GetOwner()), *ReferenceCountPtr)
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
