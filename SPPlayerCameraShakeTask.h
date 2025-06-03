// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Tasks/SPAbilityTask.h"
#include "SPPlayerCameraShakeTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class USPPlayerCameraShakeTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()

public:
	USPPlayerCameraShakeTaskScratchPad();
	virtual ~USPPlayerCameraShakeTaskScratchPad();

	UPROPERTY(transient)
	TArray<TWeakObjectPtr<class APlayerController>> Controllers;

	UPROPERTY(transient)
	TArray<TSubclassOf<UMatineeCameraShake>> ShakeClasses;
};

/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPPlayerCameraShakeTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:
	USPPlayerCameraShakeTask(const FObjectInitializer& ObjectInitializer);
	virtual ~USPPlayerCameraShakeTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	                       const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context,
					 const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns true if our Task needs its tick method called. */
	virtual bool NeedsTick() const override { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad*
	CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("SPSkillPlayerCameraShakeCategory", "Player"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("SPSkillPlayerCameraShake", "Play Camera Shake"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override
	{
		return LOCTEXT("SPSkillPlayerCameraShakeDesc",
		               "Starts/stops a camera shake on the player controllers of the target pawns.");
	}

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach
#endif

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake",
		meta = (DisplayName = "Shake", AbleBindableProperty))
	TSubclassOf<UMatineeCameraShake> Shake;

	UPROPERTY()
	FGetCameraShakeClass ShakeDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake",
		meta = (DisplayName = "Shake WaveFormNum", AbleBindableProperty))
	uint8 WaveFormNum;

	UPROPERTY()
	FGetAbleInt WaveFormNumDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake",
		meta = (DisplayName = "Shake Amplitude", AbleBindableProperty))
	float Amplitude;

	UPROPERTY()
	FGetAbleFloat AmplitudeDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake",
		meta = (DisplayName = "Shake Frequency", AbleBindableProperty))
	float Frequency;

	UPROPERTY()
	FGetAbleFloat FrequencyDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shake",
		meta = (DisplayName = "Shake Time", AbleBindableProperty))
	float Time;

	UPROPERTY()
	FGetAbleFloat TimeDelegate;
};

#undef LOCTEXT_NAMESPACE
