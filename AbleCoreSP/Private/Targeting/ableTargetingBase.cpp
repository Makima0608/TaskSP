// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingBase.h"

#include "AbleCoreSPPrivate.h"
#include "Camera/CameraActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Targeting/ableTargetingFilters.h"
#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityComponent.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTargeting"

UAbleTargetingBase::UAbleTargetingBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_AutoCalculateRange(true),
	  m_Range(0.0f),
	  m_CalculateAs2DRange(true),
	  m_Location(),
	  m_ChannelPresent(ACP_Default),
	  m_UseAsync(false),
	  m_bSaveTargetLocation(false)
{
}

UAbleTargetingBase::~UAbleTargetingBase()
{

}

bool UAbleTargetingBase::ShouldClearTargets() const
{
	return ShouldClearTargetsBP();
}

bool UAbleTargetingBase::ShouldClearTargetsBP_Implementation() const
{
	return true;
}

void UAbleTargetingBase::CleanUp()
{
	if (m_Filters.Num())
	{
		for (UAbleAbilityTargetingFilter* Filter : m_Filters)
		{
			if (IsValid(Filter))
			{
				Filter->MarkPendingKill();
			}
		}

		m_Filters.Empty();
	}
}

void UAbleTargetingBase::GetCollisionObjectParams(const UAbleAbilityContext* Context, FCollisionObjectQueryParams& outParams) const
{
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context, m_ChannelPresent, m_CollisionChannels);
	
	for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		outParams.AddObjectTypesToQuery(Channel.GetValue());
	}
}

void UAbleTargetingBase::FilterTargets(UAbleAbilityContext& Context) const
{
	if(m_bRemoveDuplicates)
	{
		// 去重
		TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();
		TSet<TWeakObjectPtr<AActor>> TargetActorSet(TargetActors);
		TargetActors = TargetActorSet.Array();
	}
	
	//不被索敌 Actor Tag contain IgnoreAbleFilterTargetsQuery
	Context.GetMutableTargetActors().RemoveAll([](const TWeakObjectPtr<AActor>& FilterActor)
	{
		if(FilterActor != nullptr)
		{
			bool bHasIgnoreAbleQuery =  FilterActor->ActorHasTag("IgnoreAbleFilterTargetsQuery");
			return bHasIgnoreAbleQuery;
		}
		return false;
	});

	if (UAbleAbilityComponent* AbilityComponent = Context.GetSelfAbilityComponent())
	{
		AbilityComponent->FilterTargets(Context);
	}

	for (UAbleAbilityTargetingFilter* TargetFilter : m_Filters)
	{
		if (IsValid(TargetFilter))
		{
			TargetFilter->Filter(Context, *this);	
		}
	}
	if (m_bSaveTargetLocation && Context.HasAnyTargets())
	{
		TWeakObjectPtr<AActor> TargetActor = nullptr;
		// 先判断目标数组长度合法性
		if (Context.GetTargetActorsWeakPtr().Num())
		{
			// 配置的目标Index，并Clamp到0和目标数组长度之间，保证当配置错误时能返回数组的最后一个单位
			int32 TargetActorIndex = FMath::Clamp(m_Location.GetTargetIndex(), 0, Context.GetTargetActorsWeakPtr().Num() - 1);
			if (Context.GetTargetActorsWeakPtr()[TargetActorIndex].IsValid())
			{
				TargetActor = Context.GetTargetActorsWeakPtr()[TargetActorIndex];
			}
		}
		if (TargetActor.IsValid())
		{
			Context.SetTargetLocation(TargetActor->GetActorLocation());
		}
	}
}

FName UAbleTargetingBase::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Targeting_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAbleTargetingBase::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Query Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Range, "Range");
}

#if WITH_EDITOR

void UAbleTargetingBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (m_AutoCalculateRange)
	{
		m_Range = CalculateRange();
	}
}

bool UAbleTargetingBase::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	bool modified = oldFlags != GetFlags();

	if (modified)
	{
		Modify();
	}

	for (UAbleAbilityTargetingFilter* Filter : m_Filters)
	{
		modified |= Filter->FixUpObjectFlags();
	}

	return modified;
}

#endif

#undef LOCTEXT_NAMESPACE
