// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/SPTargetingBase.h"

USPTargetingBase::USPTargetingBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

USPTargetingBase::~USPTargetingBase()
{
}

void USPTargetingBase::FindTargets(UAbleAbilityContext& Context) const
{
	Super::FindTargets(Context);
	FindTargetsBP(&Context);
}

void USPTargetingBase::FindTargetsBP_Implementation(UAbleAbilityContext* Context) const
{
}

void USPTargetingBase::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);
}

#if WITH_EDITOR
void USPTargetingBase::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	Super::OnAbilityEditorTick(Context, DeltaTime);
	OnAbilityEditorTickBP(&Context, DeltaTime);
}
#endif

void USPTargetingBase::OnAbilityEditorTickBP_Implementation(const UAbleAbilityContext* Context, float DeltaTime) const
{
}

float USPTargetingBase::CalculateRange() const
{
	return Super::CalculateRange();
}

void USPTargetingBase::ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	for (const FOverlapResult& Result : Results)
	{
		if (AActor* ResultActor = Result.GetActor())
		{
			TargetActors.Add(ResultActor);
		}
	}

	FilterTargets(Context);
}

void USPTargetingBase::ProcessActorResults(UAbleAbilityContext* Context, const TArray<AActor*>& Results) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context->GetMutableTargetActors();

	for (AActor* ResultActor : Results)
	{
		if (IsValid(ResultActor))
		{
			TargetActors.Add(ResultActor);
		}
	}

	FilterTargets(*Context);
}
