// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Game/SPGame/Skill/Task/SPSpawnDecalTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "Game/SPGame/Gameplay/SPDecalActor.h"
#include "Runtime/Engine/Public/ParticleEmitterInstances.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"

USPSpawnDecalTaskScratchPad::USPSpawnDecalTaskScratchPad()
{
}

USPSpawnDecalTaskScratchPad::~USPSpawnDecalTaskScratchPad()
{
}

USPSpawnDecalTask::USPSpawnDecalTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	MaterialInstance(nullptr),
	m_Scale(1),
	m_DestroyAtEnd(true),
	m_IsServer(false)
{
}

USPSpawnDecalTask::~USPSpawnDecalTask()
{
}

FString USPSpawnDecalTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPSpawnDecalTask");
}

void USPSpawnDecalTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPSpawnDecalTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	if (!m_DecalActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Particle System set for PlayParticleEffectTask in Ability [%s]"),
		       *GetNameSafe(Context->GetAbility()));
		return;
	}
	
	FTransform OffsetTransform(FTransform::Identity);
	FTransform SpawnTransform = OffsetTransform;
	TWeakObjectPtr<ASPDecalActor> SpawnerDecal = nullptr;
	
	USPSpawnDecalTaskScratchPad* ScratchPad = nullptr;
	if (m_DestroyAtEnd)
	{
		ScratchPad = Cast<USPSpawnDecalTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (ScratchPad) ScratchPad->SpawnedDecalInfos.Empty();
	}
	
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);
	
	for (int i = 0; i < TargetArray.Num(); ++i)
	{
		TWeakObjectPtr<AActor> Target = TargetArray[i];
		SpawnTransform.SetIdentity();
	
		if (m_Location.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
		{
			m_Location.GetTargetTransform(*Context, i, SpawnTransform);
		}
		else
		{
			m_Location.GetTransform(*Context, SpawnTransform);
		}
	
		FVector emitterScale(m_Scale);
		SpawnTransform.SetScale3D(emitterScale);
		
		SpawnerDecal = Target->GetWorld()->SpawnActor<ASPDecalActor>(m_DecalActor, SpawnTransform);
		FSPDecalInfo DecalInfo;
		DecalInfo.Material = MaterialInstance;
		for (auto& Parameter : m_Parameters)
		{
			DecalInfo.ParameterNames.Add(Parameter.Key);
			DecalInfo.ParameterCurves.Add(Parameter.Value);
		}
		DecalInfo.Duration = GetEndTime() - GetStartTime();

		if (AttachInfo.m_FollowTarget && Target.IsValid())
		{
			DecalInfo.FollowTargetTimePercentage = AttachInfo.m_FollowTargetTimePercentage;
			DecalInfo.DecalTarget = Target.Get();
		}
			
		SpawnerDecal->InitWithAbility(DecalInfo);
	
		if (ScratchPad)
		{
			if (SpawnerDecal.IsValid() && Target.IsValid())
			{
				ScratchPad->SpawnedDecalInfos.Add(FSpawnedDecalInfo(SpawnerDecal, Target));
			}
		}
	}
}

void USPSpawnDecalTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void USPSpawnDecalTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	
}

void USPSpawnDecalTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                                  const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPSpawnDecalTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                   const EAbleAbilityTaskResult result) const
{
	if (m_DestroyAtEnd && Context)
	{
		USPSpawnDecalTaskScratchPad* ScratchPad = Cast<USPSpawnDecalTaskScratchPad>(
			Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
	
		for (FSpawnedDecalInfo SpawnedDecalInfo : ScratchPad->SpawnedDecalInfos)
		{
			if (SpawnedDecalInfo.SpawnedDecal.IsValid())
			{
				SpawnedDecalInfo.SpawnedDecal->Destroy();
			}
		}
	}
}

TStatId USPSpawnDecalTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPSpawnDecalTask, STATGROUP_USPAbility);
}

UAbleAbilityTaskScratchPad* USPSpawnDecalTask::CreateScratchPad(
	const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = USPSpawnDecalTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<USPSpawnDecalTaskScratchPad>(Context.Get());
}

EAbleAbilityTaskRealm USPSpawnDecalTask::GetTaskRealmBP_Implementation() const
{
	return m_IsServer ? EAbleAbilityTaskRealm::ATR_Server : EAbleAbilityTaskRealm::ATR_Client;
}

bool USPSpawnDecalTask::IsSingleFrameBP_Implementation() const { return false; }
