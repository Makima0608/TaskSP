// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Game/SPGame/Skill/Task/SPPlaySoundTask.h"

#include "MoeBlueprintLibrary.h"
#include "ableSubSystem.h"
#include "AkGameplayStatics.h"
#include "LetsGoSubSystem.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Core/LogMacrosEx.h"
#include "Game/SPGame/Gameplay/SPActorInterface.h"
#include "Sound/MoeSoundAnimComponent.h"
#include "Sound/MoeSoundManager.h"
#include "Sound/MoeSoundManagerUtility.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"
#include "Game/SPGame/Utils/LuaUtility.h"
#include "MoeGameplay/Character/Component/MoeCharAvatarComponent.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

USPAblePlaySoundTaskScratchPad::USPAblePlaySoundTaskScratchPad()
{
}

USPAblePlaySoundTaskScratchPad::~USPAblePlaySoundTaskScratchPad()
{
}

USPPlaySoundTask::USPPlaySoundTask(const FObjectInitializer& Initializer)
	: Super(Initializer),
	  m_Target(ATT_Self),
	  m_IsSingleFrame(true),
	  m_Is3D(false),
	  m_bStopWhenAttachedToDestroy(true),
	  m_TransitionDuration(0),
	  m_BeStop(false),
	  m_UnLoadCD(0),
	  m_IsCollectLog(false),
	  m_InterruptWhenEnd(true)
{
}

USPPlaySoundTask::~USPPlaySoundTask()
{
}

bool USPPlaySoundTask::IsCharacter(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	AActor* Owner = Context->GetOwner();
	if (Owner)
	{
		if(ISPActorInterface* SPActor = Cast<ISPActorInterface>(Owner))
		{
			if (SPActor->GetSPActorType() == ESPActorType::Player)
			{
				return true;
			}
		}
	}
	return false;
}

FString USPPlaySoundTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPPlaySoundTask");
}

void USPPlaySoundTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPPlaySoundTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
#if !UE_SERVER 
	if (m_BankName.IsEmpty())
	{
		return;
	}
	AActor* AnimActor = GetSingleActorFromTargetType(Context, m_Target);
	if (!IsValid(AnimActor)) return;

	const USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(
		AnimActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (!IsValid(MeshComponent)) return;

#if WITH_EDITOR
	if (!FApp::IsGame())
	{
		const FString Str = "{UseSkin}";
		const FString DefaultCharacterIdBank = "VO_CommonA";
		FString BankNameSkin = m_BankName.Replace(*Str, *DefaultCharacterIdBank);
		UAkGameplayStatics::LoadAndCheckBankByName(BankNameSkin);
		if (!AnimActor) return;
		USceneComponent* RootComp = AnimActor->GetRootComponent();
		if (!RootComp) return;
		bool ComponentCreated = false;
		UAkComponent* AkComp = UAkGameplayStatics::GetAkComponent(RootComp, ComponentCreated, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset);
		if (AkComp)
		{
			const FString PlayEventSkin = m_PlayEvent.Replace(*Str, *DefaultCharacterIdBank);
			const FString PlayEvent3PSKin = m_PlayEvent3P.Replace(*Str, *DefaultCharacterIdBank);
			const FString EventName = PlayEventSkin.IsEmpty() ? PlayEvent3PSKin: PlayEventSkin;
			AkComp->PostAkEvent(nullptr, 0, FOnAkPostEventCallback(), TArray<FAkExternalSourceInfo>(), EventName);
		}
		return;
	}
#endif
	if (IsCharacter(Context))
	{
		UMoeCharAvatarComponent* CharAvatarComponent = AnimActor->FindComponentByClass<UMoeCharAvatarComponent>();
		if(!CharAvatarComponent || !IsValid(CharAvatarComponent) )
		{
			return;
		}
		if (USPAblePlaySoundTaskScratchPad* ScratchPad = Cast<USPAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
		{
			ScratchPad->m_SuitId = CharAvatarComponent->GetSuitID();
        	if(ScratchPad->m_SuitId == 0)
        	{
        		ScratchPad->m_SuitId = 400020; // 编辑器环境下，关卡测试中，如果没有皮肤，则给一个默认皮肤，方便测试
        	}
        	
        	ASPGameLuaUtility* LuaUtility = USPGameLibrary::GetSPLuaUtility(GetWorld(), AnimActor);
			if (!LuaUtility) return;
        	ScratchPad->m_CharacterIdBank = LuaUtility->GetCharacterIdBankBySuitId(ScratchPad->m_SuitId);
        	if(ScratchPad->m_CharacterIdBank == "0")
        	{
        		MOE_LOG(LogMoeSound, Log, TEXT("Get CharacterId Bank By SuitId Failed!"));
        		return;
        	}
		}
	}
	if (MeshComponent && MeshComponent->IsA(USkeletalMeshComponentBudgeted::StaticClass()))
	{
		const USkeletalMeshComponentBudgeted* BudgetedSkeletalMeshComponent = Cast<USkeletalMeshComponentBudgeted>(
			MeshComponent);
		bool bEnable = BudgetedSkeletalMeshComponent && BudgetedSkeletalMeshComponent->GetEnableSoundNotify();
		if (bEnable)
		{
			PlaySFX(AnimActor, Context);
		}
	}
	else
	{
		PlaySFX(AnimActor, Context);
	}
#endif
}

void USPPlaySoundTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                                 const EAbleAbilityTaskResult result) const
{
	OnTaskEndBP(Context.Get(), result);
	Super::OnTaskEnd(Context, result);
}

void USPPlaySoundTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                  const EAbleAbilityTaskResult result) const
{
    #if WITH_EDITOR
	if(!FApp::IsGame() && !m_IsSingleFrame)
	{
		AActor* AnimActor = GetSingleActorFromTargetType(Context, m_Target);
		if(!AnimActor) return;
		USceneComponent* RootComp = AnimActor->GetRootComponent();
		if(!RootComp) return;
		bool ComponentCreated = false;
		UAkComponent* AkComp = UAkGameplayStatics::GetAkComponent(RootComp, ComponentCreated, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset);
		if(AkComp)
		{
			AkComp->Stop();
		}
		return;
	}
#endif
	if(!m_IsSingleFrame)
	{
		if(m_InterruptWhenEnd)
		{
			if(result == Successful || result == Interrupted || result == BranchSegment)
			{
				AActor* Actor = Context->GetSelfActor();
				const bool Is1P = UMoeSoundManagerUtility::IsActorIs1P(Actor);
				int32 PlayingID = 0;
				FString EventName = Is1P ? m_PlayEvent : m_PlayEvent3P.IsEmpty() ? m_PlayEvent : m_PlayEvent3P;
				FString BankNameSkin = m_BankName;
				EMoeSoundType SoundType = m_Is3D ? EMoeSoundType::DEFAULT : Is1P ? EMoeSoundType::SFX : EMoeSoundType::DEFAULT;
				if (UMoeSoundManagerUtility::Has1PTag(Actor))
				{
					SoundType = EMoeSoundType::SFX;
				}
				UMoeSoundManager* MoeSoundManager = UMoeSoundManagerUtility::GetSoundManager(Actor);
			
				if(USPAblePlaySoundTaskScratchPad* ScratchPad = Cast<USPAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
				{
					PlayingID = ScratchPad->m_PlayingID;
				}
				MoeSoundManager->StopSound(PlayingID, false, EventName, m_StopEvent, static_cast<uint8>(SoundType), BankNameSkin, m_UnLoadCD,
							   false,
							   Actor);
			}
		}
	}
}

bool USPPlaySoundTask::IsSingleFrame() const
{
	return IsSingleFrameBP();
}

bool USPPlaySoundTask::IsSingleFrameBP_Implementation() const
{
	return m_IsSingleFrame;
}

void USPPlaySoundTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);
}

void USPPlaySoundTask::PlaySFX(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
#if WITH_EDITOR
	if (m_IsCollectLog)
	{
		const FString ActorName = Actor == nullptr || !IsValid(Actor) ? TEXT("InValidActor") : *Actor->GetName();
		MOE_LOG(LogMoeSound, Log, TEXT("AnimNotify PlaySFX:%s,EventName:%s"), *ActorName, *m_PlayEvent);
	}
#endif

	if (IsInGameThread())
	{
		PlaySfxInMainThread(Actor, Context);
	}
	else
	{
		if (ULetsGoSubSystem::Get())
		{
			ULetsGoSubSystem::Get()->RunOnGameThread([=]
			{
				PlaySfxInMainThread(Actor, Context);
			});
		}
	}
}

void USPPlaySoundTask::PlaySfxInMainThread(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (!Actor || !IsValid(Actor))
	{
		return;
	}
	UMoeSoundManager* MoeSoundManager = UMoeSoundManagerUtility::GetSoundManager(Actor);
	if (!MoeSoundManager)
	{
		return;
	}
	const bool Is1P = UMoeSoundManagerUtility::IsActorIs1P(Actor);
	FString EventName = Is1P ? m_PlayEvent : m_PlayEvent3P;
	FString BankNameSkin = m_BankName;
	if (IsCharacter(Context))
	{
		const FString Str = "{UseSkin}";
		if (USPAblePlaySoundTaskScratchPad* ScratchPad = Cast<USPAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
		{
			BankNameSkin = m_BankName.Replace(*Str, *ScratchPad->m_CharacterIdBank);
			EventName = Is1P ? m_PlayEvent.Replace(*Str, *ScratchPad->m_CharacterIdBank) : m_PlayEvent3P.Replace(*Str, *ScratchPad->m_CharacterIdBank);	
		}
	}

	EMoeSoundType SoundType = m_Is3D ? EMoeSoundType::DEFAULT : Is1P ? EMoeSoundType::SFX : EMoeSoundType::DEFAULT;
	if (UMoeSoundManagerUtility::Has1PTag(Actor))
	{
		SoundType = EMoeSoundType::SFX;
	}
	const int32 PlayingID = MoeSoundManager->DelayPlayAudio(0, BankNameSkin, EventName, static_cast<uint8>(SoundType),
	                                                        m_bStopWhenAttachedToDestroy,
	                                                        false, Actor);
	if(USPAblePlaySoundTaskScratchPad* ScratchPad = Cast<USPAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
	{
		ScratchPad->m_PlayingID = PlayingID;
	}
	MOE_LOG(LogAudio, Display, TEXT("BankName is %s, EventName is %s, SourceActor is %s"), *BankNameSkin, *EventName, *Actor->GetName());
	UMoeBlueprintLibrary::DispatchLuaEventWithObjectAndParam("ON_SPGame_AUDIOEVENT_ABLE", Actor, EventName);
	if (m_BeStop)
	{
		float aniLength = 0;
		const UMoeSoundAnimComponent* Comp = UMoeSoundAnimComponent::GetOrAddAnimComponent(Actor);
		if (IsValid(Comp))
		{
			Comp->CacheSFX(Actor, BankNameSkin, EventName, SoundType, m_PlayEvent3P, m_StopEvent, m_Is3D, m_UnLoadCD,
			               aniLength,
			               PlayingID, m_TransitionDuration, AkCurveInterpolation_SCurve);
		}
	}
#if WITH_EDITOR
	if (m_IsCollectLog)
	{
		MOE_LOG(LogMoeSound, Log, TEXT("AnimNotify PlaySFX:%s,%d,%s, bank:%s"),
		        Actor == nullptr || !IsValid(Actor) ? TEXT("InValidActor") : *Actor->GetName(), PlayingID, *EventName,
		        *BankNameSkin);
	}
#endif
}

void USPPlaySoundTask::StopSFX(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context)
{
	if (!Actor || !IsValid(Actor))
	{
		return;
	}
	UMoeSoundManager* MoeSoundManager = UMoeSoundManagerUtility::GetSoundManager(Actor);
	if (!MoeSoundManager)
	{
		return;
	}
	const bool Is1P = UMoeSoundManagerUtility::IsActorIs1P(Actor);
	FString EventName = Is1P ? m_PlayEvent : m_PlayEvent3P.IsEmpty() ? m_PlayEvent : m_PlayEvent3P;
	FString BankNameSkin = m_BankName;
	if (IsCharacter(Context))
	{
		if (USPAblePlaySoundTaskScratchPad* ScratchPad = Cast<USPAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
		{
			const FString Str = "{UseSkin}";
			BankNameSkin = m_BankName.Replace(*Str, *ScratchPad->m_CharacterIdBank);
			EventName = Is1P ? m_PlayEvent.Replace(*Str, *ScratchPad->m_CharacterIdBank) : m_PlayEvent3P.Replace(*Str, *ScratchPad->m_CharacterIdBank);
		}
	}
	if (m_UnLoadCD == 0)
	{
		m_UnLoadCD = MoeSoundManager->GetBankUnloadCd(BankNameSkin);
	}
	if (m_UnLoadCD == 0)
	{
		m_UnLoadCD = -1;
	}
	EMoeSoundType SoundType = m_Is3D ? EMoeSoundType::DEFAULT : Is1P ? EMoeSoundType::SFX : EMoeSoundType::DEFAULT;
	if (UMoeSoundManagerUtility::Has1PTag(Actor)) //如果强制设置成了1P，那么默认就在Listener的位置播放
	{
		SoundType = EMoeSoundType::SFX;
	}
	MoeSoundManager->StopSound(0, false, EventName, m_StopEvent, static_cast<uint8>(SoundType), BankNameSkin, m_UnLoadCD,
	                           false,
	                           Actor);
}

TStatId USPPlaySoundTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPPlaySoundTask, STATGROUP_USPAbility);
}

UAbleAbilityTaskScratchPad* USPPlaySoundTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = USPAblePlaySoundTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}
	return NewObject<USPAblePlaySoundTaskScratchPad>(Context.Get());
}

EAbleAbilityTaskRealm USPPlaySoundTask::GetTaskRealmBP_Implementation() const	
{
	return EAbleAbilityTaskRealm::ATR_Client;
}

#undef LOCTEXT_NAMESPACE
