// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSetCollisionChannelResponseTask.h"

#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Misc/EnumRange.h"
#include "Stats/Stats2.h"

#if !(UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

ENUM_RANGE_BY_FIRST_AND_LAST(ECollisionChannel, ECC_WorldStatic, ECC_MAX);

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleSetCollisionChannelResponseTaskScratchPad::UAbleSetCollisionChannelResponseTaskScratchPad()
{

}

UAbleSetCollisionChannelResponseTaskScratchPad::~UAbleSetCollisionChannelResponseTaskScratchPad()
{

}

UAbleSetCollisionChannelResponseTask::UAbleSetCollisionChannelResponseTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Channel(ECollisionChannel::ECC_WorldDynamic),
	m_Response(ECollisionResponse::ECR_Ignore),
	m_SetAllChannelsToResponse(false),
	m_RestoreOnEnd(true)
{

}

UAbleSetCollisionChannelResponseTask::~UAbleSetCollisionChannelResponseTask()
{

}

FString UAbleSetCollisionChannelResponseTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleSetCollisionChannelResponseTask");
}

void UAbleSetCollisionChannelResponseTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleSetCollisionChannelResponseTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleSetCollisionChannelResponseTaskScratchPad* ScratchPad = nullptr;
	
	if (m_RestoreOnEnd)
	{
		ScratchPad = Cast<UAbleSetCollisionChannelResponseTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		ScratchPad->PreviousCollisionValues.Empty();
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	TArray<FCollisionChannelResponsePair> AllCRPairs;
	// Append our deprecated value.
	AllCRPairs.Add(FCollisionChannelResponsePair(m_Channel.GetValue(), m_Response.GetValue()));
	AllCRPairs.Append(m_Channels);

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			PrimitiveComponents.AddUnique(PrimitiveComponent);
		}
	}

	for (TWeakObjectPtr<UPrimitiveComponent>& Component : PrimitiveComponents)
	{
		if (Component.IsValid())
		{
			if (m_RestoreOnEnd)
			{
				if (m_SetAllChannelsToResponse)
				{
					const FCollisionResponseContainer& Container = Component->GetCollisionResponseToChannels();
					for (ECollisionChannel Channel : TEnumRange<ECollisionChannel>())
					{
						ScratchPad->PreviousCollisionValues.Add(FCollisionLayerResponseEntry(Component.Get(), Channel, Container.GetResponse(Channel)));
					}
				}
				else
				{
					for (const FCollisionChannelResponsePair& Pair : AllCRPairs)
					{
						ScratchPad->PreviousCollisionValues.Add(FCollisionLayerResponseEntry(Component.Get(), Pair.CollisionChannel.GetValue(), Component->GetCollisionResponseToChannel(Pair.CollisionChannel.GetValue())));
					}
				}
			}

			if (m_SetAllChannelsToResponse)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting All Collision Responses on Actor %s to %s."), *Component->GetOwner()->GetName(), *FAbleSPLogHelper::GetCollisionResponseEnumAsString(m_Response.GetValue())));
				}
#endif
				Component->SetCollisionResponseToAllChannels(m_Response.GetValue());
			}
			else
			{
				for (const FCollisionChannelResponsePair& Pair : AllCRPairs)
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Channel %s Response on Actor %s to %s."), *FAbleSPLogHelper::GetCollisionChannelEnumAsString(Pair.CollisionChannel.GetValue()), *Component->GetOwner()->GetName(), *FAbleSPLogHelper::GetCollisionResponseEnumAsString(Pair.CollisionResponse.GetValue())));
					}
#endif
					Component->SetCollisionResponseToChannel(Pair.CollisionChannel.GetValue(), Pair.CollisionResponse.GetValue());
				}
			}
		}
	}
}

void UAbleSetCollisionChannelResponseTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleSetCollisionChannelResponseTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (m_RestoreOnEnd && Context)
	{
		UAbleSetCollisionChannelResponseTaskScratchPad* ScratchPad = Cast<UAbleSetCollisionChannelResponseTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		for (const FCollisionLayerResponseEntry& Entry : ScratchPad->PreviousCollisionValues)
		{
			if (Entry.Primitive.IsValid())
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Channel %s Response on Actor %s to %s."), *FAbleSPLogHelper::GetCollisionChannelEnumAsString(Entry.Channel), *Entry.Primitive->GetOwner()->GetName(), *FAbleSPLogHelper::GetCollisionResponseEnumAsString(Entry.Response)));
				}
#endif
				Entry.Primitive->SetCollisionResponseToChannel(Entry.Channel, Entry.Response);
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleSetCollisionChannelResponseTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_RestoreOnEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleSetCollisionChannelResponseTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAbleSetCollisionChannelResponseTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleSetCollisionChannelResponseTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleSetCollisionChannelResponseTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleSetCollisionChannelResponseTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleSetCollisionChannelResponseTaskFormat", "{0}: {1}->{2}");

	FString CollisionChannelName = m_SetAllChannelsToResponse ? TEXT("All") : FAbleSPLogHelper::GetCollisionChannelEnumAsString(m_Channel);
	FString ResponseName = FAbleSPLogHelper::GetCollisionResponseEnumAsString(m_Response);

	if (m_Channels.Num() != 0)
	{
		if (m_Channels.Num() == 1)
		{
			CollisionChannelName = FAbleSPLogHelper::GetCollisionChannelEnumAsString(m_Channels[0].CollisionChannel);
			ResponseName = FAbleSPLogHelper::GetCollisionResponseEnumAsString(m_Channels[0].CollisionResponse);
		}
		else
		{
			CollisionChannelName = TEXT("(multiple)");
			ResponseName = TEXT("(multiple)");
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(CollisionChannelName), FText::FromString(ResponseName));
}

#endif

EAbleAbilityTaskRealm UAbleSetCollisionChannelResponseTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE