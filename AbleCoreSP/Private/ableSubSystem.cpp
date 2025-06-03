// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableSubSystem.h"
#include "ableAbility.h"
#include "ableAbilityContext.h"
#include "ableSettings.h"

UAbleAbilityUtilitySubsystem::UAbleAbilityUtilitySubsystem(const FObjectInitializer& ObjectInitializer)
	: m_Settings(nullptr), m_ChannelPresentDataTable(nullptr)
{

}

UAbleAbilityUtilitySubsystem::~UAbleAbilityUtilitySubsystem()
{

}

void UAbleAbilityUtilitySubsystem::Initialize(UObject& Outer)
{
	Super::Initialize(Outer);

	m_Settings = GetDefault<USPAbleSettings>();
	if (m_Settings && m_Settings->GetInitialContextPoolSize() > 0)
	{
		m_AvailableContexts.Reserve(m_Settings->GetInitialContextPoolSize());
		m_AllocatedContexts.Reserve(m_Settings->GetInitialContextPoolSize());
		for (uint32 i = 0; i < m_Settings->GetInitialContextPoolSize(); ++i)
		{
			UAbleAbilityContext* Context = NewObject<UAbleAbilityContext>(this);
			m_AllocatedContexts.Add(Context);
			m_AvailableContexts.Push(Context);
		}
	}

	TryGetChannelPresentDataTable();
}

FString UAbleAbilityUtilitySubsystem::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Core.AbleAbilityUtilitySubsystem");
}

UAbleAbilityTaskScratchPad* UAbleAbilityUtilitySubsystem::FindOrConstructTaskScratchPad(TSubclassOf<UAbleAbilityTaskScratchPad>& Class)
{
	if (m_Settings && !m_Settings->GetAllowScratchPadReuse())
	{
		return NewObject<UAbleAbilityTaskScratchPad>(this, *Class);
	}

	UAbleAbilityTaskScratchPad* OutInstance = nullptr;
	if (FAbleTaskScratchPadBucket* ExistingBucket = GetTaskBucketByClass(Class))
	{
		if (ExistingBucket->Instances.Num())
		{
			OutInstance = ExistingBucket->Instances.Pop(false);
		}
		else
		{
			// Ran out of Instances, make one.
			OutInstance = NewObject<UAbleAbilityTaskScratchPad>(this, *Class);
		}
	}
	else
	{
		FAbleTaskScratchPadBucket& NewBucket = m_TaskBuckets.AddDefaulted_GetRef();
		NewBucket.ScratchPadClass = Class;
		OutInstance = NewObject<UAbleAbilityTaskScratchPad>(this, *Class);
	}

	check(OutInstance);
	return OutInstance;
}

UAbleAbilityScratchPad* UAbleAbilityUtilitySubsystem::FindOrConstructAbilityScratchPad(TSubclassOf<UAbleAbilityScratchPad>& Class)
{
	if (m_Settings && !m_Settings->GetAllowScratchPadReuse())
	{
		return NewObject<UAbleAbilityScratchPad>(this, *Class);
	}

	UAbleAbilityScratchPad* OutInstance = nullptr;
	if (FAbleAbilityScratchPadBucket* ExistingBucket = GetAbilityBucketByClass(Class))
	{
		if (ExistingBucket->Instances.Num())
		{
			OutInstance = ExistingBucket->Instances.Pop(false);
		}
		else
		{
			// Ran out of Instances, make one.
			OutInstance = NewObject<UAbleAbilityScratchPad>(this, *Class);
		}
	}
	else
	{
		FAbleAbilityScratchPadBucket& NewBucket = m_AbilityBuckets.AddDefaulted_GetRef();
		NewBucket.ScratchPadClass = Class;
		OutInstance = NewObject<UAbleAbilityScratchPad>(this, *Class);
	}

	check(OutInstance);
	return OutInstance;
}

void UAbleAbilityUtilitySubsystem::ClearAllCachePools()
{
	m_AbilityBuckets.Empty();
	m_TaskBuckets.Empty();
	m_AvailableContexts.Empty();
	m_AllocatedContexts.Empty();
}

void UAbleAbilityUtilitySubsystem::ReturnTaskScratchPad(UAbleAbilityTaskScratchPad* Scratchpad)
{
	if (m_Settings)
	{
		if (!m_Settings->GetAllowScratchPadReuse())
		{
			return;
		}

		if (m_Settings->GetMaxScratchPadPoolSize() > 0U && GetTotalScratchPads() > m_Settings->GetMaxScratchPadPoolSize())
		{
			return;
		}
	}

	TSubclassOf<UAbleAbilityTaskScratchPad> ClassSubClass = Scratchpad->GetClass();
	if (FAbleTaskScratchPadBucket* Bucket = GetTaskBucketByClass(ClassSubClass))
	{
		Bucket->Instances.Push(Scratchpad);
	}

	// If we don't have a bucket then we somehow mixed worlds... which doesn't make sense. Just let it release through the GC system.
}

void UAbleAbilityUtilitySubsystem::ReturnAbilityScratchPad(UAbleAbilityScratchPad* Scratchpad)
{
	if (m_Settings)
	{
		if (!m_Settings->GetAllowScratchPadReuse())
		{
			return;
		}

		if (m_Settings->GetMaxScratchPadPoolSize() > 0U && GetTotalScratchPads() > m_Settings->GetMaxScratchPadPoolSize())
		{
			return;
		}
	}

	TSubclassOf<UAbleAbilityScratchPad> ClassSubClass = Scratchpad->GetClass();
	if (FAbleAbilityScratchPadBucket* Bucket = GetAbilityBucketByClass(ClassSubClass))
	{
		Bucket->Instances.Push(Scratchpad);
	}

	// If we don't have a bucket then we somehow mixed worlds... which doesn't make sense. Just let it release through the GC system.
}

UDataTable* UAbleAbilityUtilitySubsystem::TryGetChannelPresentDataTable()
{
	if (!m_ChannelPresentDataTable)
	{
		const FString DataTableReference = "/Game/Feature/StarP/Data/AssetData/SP_AbleChannelPresent.SP_AbleChannelPresent";
		UDataTable* DataTable = Cast<UDataTable>(FSoftObjectPath(DataTableReference).TryLoad());
		m_ChannelPresentDataTable = DataTable;
	}

	return m_ChannelPresentDataTable;
}

FAbleTaskScratchPadBucket* UAbleAbilityUtilitySubsystem::GetTaskBucketByClass(TSubclassOf<UAbleAbilityTaskScratchPad>& Class)
{
	if (!Class.Get())
	{
		return nullptr;
	}

	for (FAbleTaskScratchPadBucket& Bucket : m_TaskBuckets)
	{
		if (Class->GetFName() == Bucket.ScratchPadClass->GetFName())
		{
			return &Bucket;
		}
	}

	return nullptr;
}

FAbleAbilityScratchPadBucket* UAbleAbilityUtilitySubsystem::GetAbilityBucketByClass(TSubclassOf<UAbleAbilityScratchPad>& Class)
{
	if (!Class.Get())
	{
		return nullptr;
	}

	for (FAbleAbilityScratchPadBucket& Bucket : m_AbilityBuckets)
	{
		if (Class->GetFName() == Bucket.ScratchPadClass->GetFName())
		{
			return &Bucket;
		}
	}

	return nullptr;
}

uint32 UAbleAbilityUtilitySubsystem::GetTotalScratchPads() const
{
	uint32 TotalAmount = 0;

	for (const FAbleAbilityScratchPadBucket& Bucket : m_AbilityBuckets)
	{
		TotalAmount += Bucket.Instances.Num();
	}

	for (const FAbleTaskScratchPadBucket& Bucket : m_TaskBuckets)
	{
		TotalAmount += Bucket.Instances.Num();
	}

	return TotalAmount;
}

UAbleAbilityContext* UAbleAbilityUtilitySubsystem::FindOrConstructContext()
{
	UAbleAbilityContext* Context = nullptr;
	if (m_Settings && !m_Settings->GetAllowAbilityContextReuse())
	{
		Context = NewObject<UAbleAbilityContext>(this);
		m_AllocatedContexts.Add(Context);
	}
	else
	{
		if (m_AvailableContexts.Num() != 0)
		{
			Context = m_AvailableContexts.Pop(false);
		}
		else
		{
			Context = NewObject<UAbleAbilityContext>(this);
			m_AllocatedContexts.Add(Context);
		}
	}

	return Context;
}

void UAbleAbilityUtilitySubsystem::ReturnContext(UAbleAbilityContext* Context)
{
	if (m_Settings && !m_Settings->GetAllowAbilityContextReuse())
	{
		if (!m_Settings->GetAllowAbilityContextReuse())
		{
			return;
		}

		if (m_Settings->GetMaxContextPoolSize() > 0U && (uint32)m_AvailableContexts.Num() >= m_Settings->GetMaxContextPoolSize())
		{
			return;
		}
	}

	m_AvailableContexts.Push(Context);
}
