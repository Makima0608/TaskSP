// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableModifyContextTask.h"

#include "ableAbility.h"
#include "ableAbilityComponent.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleModifyContextTask::UAbleModifyContextTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_TargetLocation(ForceInitToZero),
	m_AdditionalTargets(true),
	m_ClearCurrentTargets(false)
{

}

UAbleModifyContextTask::~UAbleModifyContextTask()
{

}


FString UAbleModifyContextTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleModifyContextTask");
}

void UAbleModifyContextTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleModifyContextTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	FVector WantsTargetLocation = m_TargetLocation;

	bool WantsAdditionalTargets = m_AdditionalTargets;
	bool WantsClearTargets = m_ClearCurrentTargets;

	TArray<AActor*> AdditionalTargets;

	if (WantsAdditionalTargets)
	{
		Context->GetAbility()->CustomTargetingFindTargets(Context, AdditionalTargets);
	}

	TArray<TWeakObjectPtr<AActor>> AdditionalWeakTargets(AdditionalTargets);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Modifying Context with Location %s, and %d New Targets. Clear Targets = %s"), 
		WantsTargetLocation.SizeSquared() > 0.0f ? *WantsTargetLocation.ToString() : TEXT("<not changed>"),
		AdditionalWeakTargets.Num(),
		WantsClearTargets ? TEXT("True") : TEXT("False")));
	}
#endif

	Context->GetSelfAbilityComponent()->ModifyContext(Context, nullptr, nullptr, WantsTargetLocation, AdditionalWeakTargets, WantsClearTargets);
}

TStatId UAbleModifyContextTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleModifyContextTask, STATGROUP_Able);
}

void UAbleModifyContextTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AdditionalTargets, TEXT("Additional Targets"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ClearCurrentTargets, TEXT("Clear Targets"));
}

bool UAbleModifyContextTask::IsSingleFrameBP_Implementation() const { return true; }

EAbleAbilityTaskRealm UAbleModifyContextTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_ClientAndServer; }

#undef LOCTEXT_NAMESPACE