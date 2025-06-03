// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Targeting/ableTargetingBase.h"
#include "Components/AudioComponent.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePlaySoundTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblePlaySoundTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblePlaySoundTaskScratchPad();
	virtual ~UAblePlaySoundTaskScratchPad();

	/* All the sounds we created. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UAudioComponent>> AttachedSounds;
};

UCLASS()
class ABLECORESP_API UAblePlaySoundTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAblePlaySoundTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePlaySoundTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }
	
	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;
	
	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;
	
	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAbleAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePlaySoundTaskCategory", "Audio"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePlaySoundTask", "Play Sound"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePlaySoundTaskDesc", "Plays a sound at the given location, can be attached to socket, or played as a 2D (rather than 3D) sound."); }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(135.0f / 255.0f, 50.0f / 255.0f, 105.0f / 255.0f); }
#endif

protected:
	/* The Sound to play. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Sound", AbleBindableProperty))
	USoundBase* m_Sound;

	UPROPERTY()
	FGetAbleSound m_SoundDelegate;

	/* Plays the Sound as a 2D sound */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Play as 2D"))
	bool m_2DSound;

	/* The time, within the sound file, to begin playing the Sound from. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Sound Start Time", AbleBindableProperty))
	float m_SoundStartTime;

	UPROPERTY()
	FGetAbleFloat m_SoundStartTimeDelegate;

	/* Volume modifier to apply with this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Volume Modifier", AbleBindableProperty))
	float m_VolumeModifier;

	UPROPERTY()
	FGetAbleFloat m_VolumeModifierDelegate;

	/* Pitch modifier to apply with this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Pitch Modifier", AbleBindableProperty))
	float m_PitchModifier;

	UPROPERTY()
	FGetAbleFloat m_PitchModifierDelegate;

	/* Attenuation settings for this sound.*/
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Attenuation", AbleBindableProperty))
	USoundAttenuation* m_Attenuation;

	UPROPERTY()
	FGetAbleAttenuation m_AttenuationDelegate;

	/* Concurrency settings for this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Concurrency", AbleBindableProperty))
	USoundConcurrency* m_Concurrency;

	UPROPERTY()
	FGetAbleConcurrency m_ConcurrencyDelegate;

	/* Location for this sound. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAbleTargetLocation m_LocationDelegate;

	/* Attach the sound to a socket. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Attach to Socket"))
	bool m_AttachToSocket;

	/* Stop the sound when the task ends. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Destroy on End"))
	bool m_DestroyOnEnd;

	/* Stop the sound when the attached Actor is destroyed. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Destroy on Actor Destroy"))
	bool m_DestroyOnActorDestroy;

	/* If the sound is being destroyed early, how long, in seconds, to fade out so we don't have any hard audio stops. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Fade Out Duration"))
	float m_DestroyFadeOutDuration;

	/* Allow the sound to clean up itself once done playing, this can happen outside of the Task and should be left ON by default. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (DisplayName = "Allow Auto Destroy"))
	bool m_AllowAutoDestroy;
};

#undef LOCTEXT_NAMESPACE