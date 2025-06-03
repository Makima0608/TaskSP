// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSetCollisionChannelTask.h"

#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/PrimitiveComponent.h"

#if !(UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleSetCollisionChannelTaskScratchPad::UAbleSetCollisionChannelTaskScratchPad()
{

}

UAbleSetCollisionChannelTaskScratchPad::~UAbleSetCollisionChannelTaskScratchPad()
{

}

UAbleSetCollisionChannelTask::UAbleSetCollisionChannelTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Channel(ECollisionChannel::ECC_Pawn),
	m_RestoreOnEnd(true)
{

}

UAbleSetCollisionChannelTask::~UAbleSetCollisionChannelTask()
{

}

FString UAbleSetCollisionChannelTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleSetCollisionChannelTask");
}

void UAbleSetCollisionChannelTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleSetCollisionChannelTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleSetCollisionChannelTaskScratchPad* ScratchPad = nullptr;
	if (m_RestoreOnEnd)
	{
		ScratchPad = Cast<UAbleSetCollisionChannelTaskScratchPad>(Context->GetScratchPadForTask(this));
		ScratchPad->PrimitiveToChannelMap.Empty();
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			PrimitiveComponents.AddUnique(PrimitiveComponent);
		}
	}

	for (TWeakObjectPtr<UPrimitiveComponent>& Component : PrimitiveComponents)
	{
		if (m_RestoreOnEnd && ScratchPad)
		{
			ECollisionChannel CurrentChannel = Component->GetCollisionObjectType();
			ScratchPad->PrimitiveToChannelMap.Add(Component, CurrentChannel);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Object Type on Actor %s to %s."), *Component->GetOwner()->GetName(), *FAbleSPLogHelper::GetCollisionChannelEnumAsString(m_Channel.GetValue())));
		}
#endif

		Component->SetCollisionObjectType(m_Channel.GetValue());
	}


}

void UAbleSetCollisionChannelTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleSetCollisionChannelTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (!Context)
	{
		return;
	}

	if (UAbleSetCollisionChannelTaskScratchPad* ScratchPad = Cast<UAbleSetCollisionChannelTaskScratchPad>(Context->GetScratchPadForTask(this)))
	{
		for (TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionChannel>>::TConstIterator Iter = ScratchPad->PrimitiveToChannelMap.CreateConstIterator(); Iter; ++Iter)
		{
			if (UPrimitiveComponent* PrimitiveComponent = Iter->Key.Get())
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting Collision Object Type on Actor %s to %s."), *PrimitiveComponent->GetOwner()->GetName(), *FAbleSPLogHelper::GetCollisionChannelEnumAsString(Iter->Value)));
				}
#endif
				PrimitiveComponent->SetCollisionObjectType(Iter->Value);
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleSetCollisionChannelTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_RestoreOnEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleSetCollisionChannelTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAbleSetCollisionChannelTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleSetCollisionChannelTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleSetCollisionChannelTask, STATGROUP_Able);
}

#if WITH_EDITOR

FText UAbleSetCollisionChannelTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleSetCollisionChannelTaskFormat", "{0}: {1}");
	FString CollisionChannelName = FAbleSPLogHelper::GetCollisionChannelEnumAsString(m_Channel);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(CollisionChannelName));
}

#endif

EAbleAbilityTaskRealm UAbleSetCollisionChannelTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_ClientAndServer; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE
