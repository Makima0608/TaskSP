// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SPGame/Skill/Task/SPAddBuffTask.h"

#include "MoeFeatureSPLog.h"
#include "Game/SPGame/Gameplay/SPActorInterface.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"

USPAddBuffTask::USPAddBuffTask(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer), m_BuffID(0),
                                                                             m_Layer(1), m_Instigator(ATT_Self)
{
}

FString USPAddBuffTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPAddBuffTask");
}

void USPAddBuffTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPAddBuffTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_Layer <= 0) return;

	TArray<TWeakObjectPtr<AActor>> Targets;
	GetActorsForTask(Context, Targets);
	if (Targets.Num() <= 0) return;

	AActor* Instigator = GetSingleActorFromTargetType(Context, m_Instigator);

	for (int i = 0; i < Targets.Num(); ++i)
	{
		if (!Targets[i].IsValid()) continue;

		ISPActorInterface* TargetActor = Cast<ISPActorInterface>(Targets[i].Get());
		if (!TargetActor) continue;

		USPAbilityComponent* AbilityComponent = TargetActor->GetAbilityComponent();
		if (!AbilityComponent) continue;

		const bool Ret = AbilityComponent->AddBuff(m_BuffID, Context->GetOwner(), Instigator, m_Layer);
		if (!Ret)
		{
			MOE_SP_ABILITY_LOG(TEXT("[SPAbility] USPAddBuffTask::OnTaskStart AddBuff to [%s] failed !"),
			            *GetNameSafe(TargetActor->GetInterfaceOwner()));
		}
	}
}

void USPAddBuffTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void USPAddBuffTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
}

void USPAddBuffTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                               const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPAddBuffTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                const EAbleAbilityTaskResult result) const
{
}

TStatId USPAddBuffTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPAddBuffTask, STATGROUP_USPAbility);
}

bool USPAddBuffTask::IsSingleFrameBP_Implementation() const
{
	return !m_bRemoveOnEnd;
}

EAbleAbilityTaskRealm USPAddBuffTask::GetTaskRealmBP_Implementation() const
{
	return ATR_Server;
}
