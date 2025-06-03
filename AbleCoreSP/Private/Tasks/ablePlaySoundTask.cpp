// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlaySoundTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePlaySoundTaskScratchPad::UAblePlaySoundTaskScratchPad()
{

}

UAblePlaySoundTaskScratchPad::~UAblePlaySoundTaskScratchPad()
{

}

UAblePlaySoundTask::UAblePlaySoundTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_Sound(nullptr),
	  m_2DSound(false),
	  m_SoundStartTime(0.0f),
	  m_VolumeModifier(1.0f),
	  m_PitchModifier(1.0f),
	  m_Attenuation(nullptr),
	  m_Concurrency(nullptr),
	  m_AttachToSocket(false),
	  m_DestroyOnEnd(false),
	  m_DestroyOnActorDestroy(false),
	  m_DestroyFadeOutDuration(0.25f),
	  m_AllowAutoDestroy(false)
{
}

UAblePlaySoundTask::~UAblePlaySoundTask()
{

}

FString UAblePlaySoundTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePlaySoundTask");
}

void UAblePlaySoundTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePlaySoundTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	if (!m_Sound)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("No Sound set for PlaySoundTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
		return;
	}


	FTransform SpawnTransform;

	USoundBase* Sound = m_Sound;
	float startTime = m_SoundStartTime;
	float VolumeMod = m_VolumeModifier;
	float PitchMod = m_PitchModifier;
	USoundAttenuation* SoundAtten = m_Attenuation;
	USoundConcurrency* SoundConCur = m_Concurrency;
	FAbleAbilityTargetTypeLocation Location = m_Location;

	if (m_2DSound)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Spawning 2D Sound [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s]"),
				*Sound->GetName(),
				VolumeMod,
				PitchMod,
				startTime,
				SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
				SoundConCur ? *SoundConCur->GetName() : TEXT("NULL")));
		}
#endif

		UGameplayStatics::PlaySound2D(GetWorld(), Sound, m_VolumeModifier, m_PitchModifier, startTime, SoundConCur);
		return; // We can early out, no reason to play a 2D sound multiple times.
	}

	TWeakObjectPtr<UAudioComponent> AttachedSound = nullptr;
	
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);
	
	UAblePlaySoundTaskScratchPad* ScratchPad = nullptr;
	if (m_DestroyOnEnd)
	{
		ScratchPad = Cast<UAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this));
		ScratchPad->AttachedSounds.Empty();
	}

	for (int i = 0; i < TargetArray.Num(); ++i)
	{
		TWeakObjectPtr<AActor> Target = TargetArray[i];
		SpawnTransform.SetIdentity();

		if (Location.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
		{
			Location.GetTargetTransform(*Context, i, SpawnTransform);
		}
		else
		{
			Location.GetTransform(*Context, SpawnTransform);
		}

		if (m_AttachToSocket)
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Spawning Attached Sound [%s], Transform [%s], DestroyOnActorDestroy [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s], Auto Destroy [%s]"),
															*Sound->GetName(),
															*SpawnTransform.ToString(),
															 m_DestroyOnActorDestroy ? TEXT("TRUE") : TEXT("FALSE"),
															 VolumeMod,
															 PitchMod,
															 startTime,
															 SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
															 SoundConCur ? *SoundConCur->GetName() : TEXT("NULL"),
															 m_AllowAutoDestroy ? TEXT("TRUE") : TEXT("FALSE")));
			}
#endif

			AttachedSound = UGameplayStatics::SpawnSoundAttached(Sound, 
																Target->FindComponentByClass<USceneComponent>(), 
																Location.GetSocketName(), 
																Location.GetOffset(), 
																EAttachLocation::KeepRelativeOffset,
																m_DestroyOnActorDestroy,
																VolumeMod,
																PitchMod,
																startTime,
																SoundAtten,
																SoundConCur, 
																m_AllowAutoDestroy);
		}
		else
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Spawning Sound [%s], Transform [%s], Volume Modifier [%1.3f], Pitch Modifier [%1.3f], Sound Start Time [%4.2f], Attenuation [%s], Concurrency [%s], Auto Destroy [%s]"),
					*Sound->GetName(),
					*SpawnTransform.ToString(),
					VolumeMod,
					PitchMod,
					startTime,
					SoundAtten ? *SoundAtten->GetName() : TEXT("NULL"),
					SoundConCur ? *SoundConCur->GetName() : TEXT("NULL"),
					m_AllowAutoDestroy ? TEXT("TRUE") : TEXT("FALSE")));
			}
#endif
			AttachedSound = UGameplayStatics::SpawnSoundAtLocation(Target->GetWorld(),
																   Sound,
																   SpawnTransform.GetTranslation(),
																   SpawnTransform.GetRotation().Rotator(),
																   VolumeMod,
																   PitchMod,
																   startTime,
																   SoundAtten,
																   SoundConCur, 
																   m_AllowAutoDestroy);
		}

		if (AttachedSound.IsValid() && ScratchPad)
		{
			ScratchPad->AttachedSounds.Add(AttachedSound);
		}

		AttachedSound = nullptr;
	}
}

void UAblePlaySoundTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);
	OnTaskEndBP(Context.Get(), Result);
}

void UAblePlaySoundTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const
{

	if (m_DestroyOnEnd && Context)
	{
		if (UAblePlaySoundTaskScratchPad* ScratchPad = Cast<UAblePlaySoundTaskScratchPad>(Context->GetScratchPadForTask(this)))
		{
			for (TWeakObjectPtr<UAudioComponent>& AudioComponent : ScratchPad->AttachedSounds)
			{
				if (AudioComponent.IsValid())
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, TEXT("Destroying Sound."));
					}
#endif
					AudioComponent->bAutoDestroy = true;
					AudioComponent->FadeOut(m_DestroyFadeOutDuration, 0.0f);
				}
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAblePlaySoundTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_DestroyOnEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAblePlaySoundTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblePlaySoundTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAblePlaySoundTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePlaySoundTask, STATGROUP_Able);
}

void UAblePlaySoundTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Sound, TEXT("Sound"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SoundStartTime, TEXT("Sound Start Time"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_VolumeModifier, TEXT("Volume Modifier"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_PitchModifier, TEXT("Pitch Modifier"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Attenuation, TEXT("Attenuation"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Concurrency, TEXT("Concurrency"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
}

#if WITH_EDITOR

FText UAblePlaySoundTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlaySoundTaskFormat", "{0}: {1}");
	FString SoundName = TEXT("<null>");
	if (m_Sound)
	{
		SoundName = m_Sound->GetName();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(SoundName));
}

FText UAblePlaySoundTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString SoundName;
	if (m_SoundDelegate.IsBound())
	{
		SoundName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_SoundDelegate.GetFunctionName().ToString() });
	}
	else
	{
		SoundName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Sound\" Filter=\"SoundBase\">{0}</>"), { m_Sound ? m_Sound->GetName() : TEXT("NULL") });
	}

	FString SoundStartString;
	if (m_SoundStartTimeDelegate.IsBound())
	{
		SoundStartString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_SoundStartTimeDelegate.GetFunctionName().ToString() });
	}
	else
	{
		SoundStartString = FString::Format(TEXT("<a id=\"AbleTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_SoundStartTime\" MinValue=\"0.0\">{0}</>"), { m_SoundStartTime });
	}

	FString VolumeString;
	if (m_VolumeModifierDelegate.IsBound())
	{
		VolumeString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_VolumeModifierDelegate.GetFunctionName().ToString() });
	}
	else
	{
		VolumeString = FString::Format(TEXT("<a id=\"AbleTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_VolumeModifier\" MinValue=\"0.0\">{0}</>"), { m_VolumeModifier });
	}

	FString PitchString;
	if (m_PitchModifierDelegate.IsBound())
	{
		PitchString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_PitchModifierDelegate.GetFunctionName().ToString() });
	}
	else
	{
		PitchString = FString::Format(TEXT("<a id=\"AbleTextDecorators.FloatValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_PitchModifier\" MinValue=\"0.0\">{0}</>"), { m_PitchModifier });
	}

	FString AttenuationString;
	if (m_AttenuationDelegate.IsBound())
	{
		AttenuationString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AttenuationDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AttenuationString = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Attenuation\" Filter=\"SoundAttenuation\">{0}</>"), { m_Attenuation ? m_Attenuation->GetName() : TEXT("NULL") });
	}

	FString ConcurrencyString;
	if (m_ConcurrencyDelegate.IsBound())
	{
		ConcurrencyString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_ConcurrencyDelegate.GetFunctionName().ToString() });
	}
	else
	{
		ConcurrencyString = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_Attenuation\" Filter=\"SoundConcurrency\">{0}</>"), { m_Concurrency ? m_Concurrency->GetName() : TEXT("NULL") });
	}

	StringBuilder.AppendLineFormat(LOCTEXT("AblePlaySoundTaskRichFmt", "\t- Sound: {0}\n\t- Sound Start Time: {1}\n\t- Volume Mod: {2}\n\t- Pitch Mod: {3}\n\t- Atten: {4}\n\t- Concur: {5}"), 
								   FText::FromString(SoundName),
								   FText::FromString(SoundStartString),
								   FText::FromString(VolumeString),
								   FText::FromString(PitchString),
								   FText::FromString(AttenuationString),
								   FText::FromString(ConcurrencyString));

	return StringBuilder.ToText();
}

#endif

bool UAblePlaySoundTask::IsSingleFrameBP_Implementation() const { return !m_DestroyOnEnd; }

EAbleAbilityTaskRealm UAblePlaySoundTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_Client; }

#undef LOCTEXT_NAMESPACE

