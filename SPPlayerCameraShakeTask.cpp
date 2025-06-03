// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Game/SPGame/Skill/Task/SPPlayerCameraShakeTask.h"

#include "ableSubSystem.h"
#include "Camera/CameraShake.h"
#include "GameFramework/PlayerController.h"
#include "MoeGameplay/Character/MoeGameCharacter.h"
#include "MoeGameLog.h"
#include "Game/SPGame/Character/SPGameCharacterBase.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

USPPlayerCameraShakeTaskScratchPad::USPPlayerCameraShakeTaskScratchPad()
{
}

USPPlayerCameraShakeTaskScratchPad::~USPPlayerCameraShakeTaskScratchPad()
{
}

USPPlayerCameraShakeTask::USPPlayerCameraShakeTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , Shake(nullptr)
	  , WaveFormNum(5)
	  , Amplitude(5)
	  , Frequency(5)
	  , Time(1.f)
{
}

USPPlayerCameraShakeTask::~USPPlayerCameraShakeTask()
{
}

FString USPPlayerCameraShakeTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPPlayerCameraShakeTask");
}

void USPPlayerCameraShakeTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPPlayerCameraShakeTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	USPPlayerCameraShakeTaskScratchPad* ScratchPad = Cast<USPPlayerCameraShakeTaskScratchPad>(
		Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	ScratchPad->Controllers.Empty();
	ScratchPad->ShakeClasses.Empty();

	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TSubclassOf<UMatineeCameraShake> ShakeClass = Shake;
	if (ShakeClass == nullptr)
	{
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context,TEXT("Invalid Shake Class was supplied."));
		}
#endif
		return;
	}

	const uint8 WaveFormNumValue = WaveFormNum;
	const float AmplitudeValue = Amplitude;
	const float FrequencyValue = Frequency;
	const float TimeValue = Time;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (Target.IsValid())
		{
			const AMoeGameCharacter* MoeCharacter = Cast<const AMoeGameCharacter>(Target);
			if (IsValid(MoeCharacter))
			{
				APlayerController* PlayerController = USPGameLibrary::FindRelatedPlayerControllerByPawn(MoeCharacter);
				if (IsValid(PlayerController))
				{
					ScratchPad->Controllers.Add(PlayerController);
					ScratchPad->ShakeClasses.Add(ShakeClass);

					UMoeCameraManagerSubSystem::Action_MainPlayerCamera_StartShake(
						Cast<const AMoeGameCharacter>(MoeCharacter), ShakeClass->GetPathName(), WaveFormNumValue,
						AmplitudeValue, FrequencyValue, TimeValue);

#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Format(
							             TEXT("Controller {0} Played Camera Shake {1}"), {
								             PlayerController->GetName(), ShakeClass->GetName()
							             }));
					}
#endif
				}
			}
		}
	}
}

void USPPlayerCameraShakeTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                                         const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPPlayerCameraShakeTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                          const EAbleAbilityTaskResult result) const
{

	// todo... 临时处理 @bladeyuan
	EAblePlayCameraShakeStopMode StopModeVal = EAblePlayCameraShakeStopMode::DontStop;
	// ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, StopMode);
	if (!Context || (StopModeVal == EAblePlayCameraShakeStopMode::DontStop))
	{
		return;
	}

	USPPlayerCameraShakeTaskScratchPad* ScratchPad = Cast<USPPlayerCameraShakeTaskScratchPad>(
		Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	if (ScratchPad->Controllers.Num() != ScratchPad->ShakeClasses.Num()) return;

	for (int i = 0; i < ScratchPad->Controllers.Num(); ++i)
	{
		TWeakObjectPtr<APlayerController>& PC = ScratchPad->Controllers[i];
		const TSubclassOf<UMatineeCameraShake>& CamShake = ScratchPad->ShakeClasses[i];
		if (PC.IsValid())
		{
			const AMoeGameCharacter* Character = USPGameLibrary::FindRelatedPlayerPawnByController(PC.Get());
			UMoeCameraManagerSubSystem::Action_Camera_StopShake(Character);

#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Format(
					             TEXT("Controller {0} Played Camera Shake {1}"), {PC->GetName(), CamShake->GetName()}));
			}
#endif
		}
	}
}

UAbleAbilityTaskScratchPad* USPPlayerCameraShakeTask::CreateScratchPad(
	const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass =
			USPPlayerCameraShakeTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<USPPlayerCameraShakeTaskScratchPad>(Context.Get());
}

TStatId USPPlayerCameraShakeTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPPlayerCameraShakeTask, STATGROUP_USPAbility);
}

void USPPlayerCameraShakeTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, Shake, TEXT("Shake"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, WaveFormNum, TEXT("Shake WaveFormNum"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, Amplitude, TEXT("Shake Amplitude"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, Frequency, TEXT("Shake Frequency"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, Time, TEXT("Shake Time"));
}

#if WITH_EDITOR

FText USPPlayerCameraShakeTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("SPSkillPlayerCameraShakeTaskFormat", "{0}: {1}");
	FString TargetName = FString::Format(TEXT("({0})"), {Shake ? Shake->GetName() : TEXT("<null>")});
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

#endif

bool USPPlayerCameraShakeTask::IsSingleFrameBP_Implementation() const { return false; }

EAbleAbilityTaskRealm USPPlayerCameraShakeTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_Client; }

#undef LOCTEXT_NAMESPACE
