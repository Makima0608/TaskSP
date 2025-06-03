// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SPGame/Skill/Task/SPSetMeshVisibilityTask.h"

#include "MoeFeatureSPLog.h"
#include "Game/SPGame/Character/SPGameMonsterBase.h"
#include "Game/SPGame/Gameplay/SPActorInterface.h"
#include "Game/SPGame/Monster/Component/SPGameMonsterBillBoardComponent.h"


USPSetMeshVisibilityTask::USPSetMeshVisibilityTask(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	m_Target(ATT_Self),
	m_StartVisibility(true),
	m_EndVisibility(true),
	m_bSetCollision(true),
	m_IsDuration(false),
	m_ResetOnEnd(false),
	m_SetMeshOnly(false),
	m_SetBillboard(true),
	m_PropagateToChildren(true),
	m_TaskRealm(ATR_ClientAndServer)
{
}

USPSetMeshVisibilityTask::~USPSetMeshVisibilityTask()
{
}

FString USPSetMeshVisibilityTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPSetMeshVisibilityTask");
}

void USPSetMeshVisibilityTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPSetMeshVisibilityTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	AActor* TaskActor = GetSingleActorFromTargetType(Context, m_Target);
	if (!IsValid(TaskActor))
	{
		return;
	}
	if(m_SetBillboard)
	{
		// 布告板组件
		TArray<USPGameMonsterBillBoardComponent*> BBComps;
		TaskActor->GetComponents<USPGameMonsterBillBoardComponent>(BBComps);

		for(auto BBComp : BBComps)
		{
			BBComp->SetVisibility(m_StartVisibility);
		}
			
	}
	bool bIsPhysicallyHidden = !TaskActor->GetActorEnableCollision();
	USPGameLibrary::SetSPActorHiddenInGame(TaskActor, !m_StartVisibility, m_bSetCollision ? !m_StartVisibility : bIsPhysicallyHidden, FVector::ZeroVector, false, false, true ,m_PropagateToChildren);
	MOE_SP_ABILITY_LOG(TEXT("[SPSkill] SetMeshVisibilityTask set Actor %s visibility to %s on start"), *GetNameSafe(TaskActor), m_StartVisibility ? TEXT("true") : TEXT("false"));
}

void USPSetMeshVisibilityTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPSetMeshVisibilityTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	AActor* TaskActor = GetSingleActorFromTargetType(Context, m_Target);
	if (!IsValid(TaskActor))
	{
		return;
	}
	if (m_IsDuration && m_ResetOnEnd)
	{
		if(m_SetBillboard)
		{
			// 布告板组件
			TArray<USPGameMonsterBillBoardComponent*> BBComps;
			TaskActor->GetComponents<USPGameMonsterBillBoardComponent>(BBComps);

			for(auto BBComp : BBComps)
			{
				BBComp->SetVisibility(m_EndVisibility);
			}
			
		}
		bool bIsPhysicallyHidden = !TaskActor->GetActorEnableCollision();
		USPGameLibrary::SetSPActorHiddenInGame(TaskActor, !m_EndVisibility, m_bSetCollision ? !m_EndVisibility : bIsPhysicallyHidden, FVector::ZeroVector, false, false, true,m_PropagateToChildren);
		MOE_SP_ABILITY_LOG(TEXT("[SPSkill] SetMeshVisibilityTask set Actor %s visibility to %s on end"), *GetNameSafe(TaskActor), m_EndVisibility ? TEXT("true") : TEXT("false"));
	}
}

void USPSetMeshVisibilityTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);
}

TStatId USPSetMeshVisibilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPSetMeshVisibilityTask, STATGROUP_USPAbility);
}

bool USPSetMeshVisibilityTask::IsSingleFrameBP_Implementation() const { return !m_IsDuration; }

EAbleAbilityTaskRealm USPSetMeshVisibilityTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }
