// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Tasks/IAbleAbilityTask.h"
#include "SPPlaySoundTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class USPAblePlaySoundTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	USPAblePlaySoundTaskScratchPad();
	virtual ~USPAblePlaySoundTaskScratchPad();

	UPROPERTY(Transient)
	int32 m_SuitId = 0;

	UPROPERTY(Transient)
	FString m_CharacterIdBank = "0";

	UPROPERTY(Transient)
	int32 m_PlayingID = 0;
};

/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPPlaySoundTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:
	USPPlaySoundTask(const FObjectInitializer& Initializer);

	~USPPlaySoundTask();

	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	                       const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context,
					 const EAbleAbilityTaskResult result) const;

	virtual bool IsSingleFrame() const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;
	
	void PlaySFX(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
	
	void PlaySfxInMainThread(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
	
	void StopSFX(AActor* Actor, const TWeakObjectPtr<const UAbleAbilityContext>& Context);

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual TStatId GetStatId() const override;

	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const override;

	bool IsCharacter(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
#if WITH_EDITOR

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPPlaySoundTask", "Misc"); }

	virtual FText GetTaskName() const override { return LOCTEXT("USPPlaySoundTask", "Play Sound"); }

	FString GetPlayEvent() const { return m_PlayEvent; }
	FString GetPlayEvent3P() const { return m_PlayEvent3P; }
	FString GetBankName() const { return m_BankName; }

#endif

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sound", meta = (DisplayName = "Target"))
	TEnumAsByte<EAbleAbilityTargetType> m_Target;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisPlayNmae = "Is Single Frame"))
	bool m_IsSingleFrame;
	
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Bank Name"))
	FString m_BankName;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Play Event"))
	FString m_PlayEvent;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Play Event 3P"))
	FString m_PlayEvent3P;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Stop Event"))
	FString m_StopEvent;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Is 3D"))
	bool m_Is3D;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Stop When Destroy"))
	bool m_bStopWhenAttachedToDestroy;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Transition Duration"))
	float m_TransitionDuration; // 默认打断延时

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "BeStop"))
	bool m_BeStop;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "UnLoad CD"))
	float m_UnLoadCD;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Collect Log"))
	bool m_IsCollectLog;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisPlayName = "Interrupt When End"))
	bool m_InterruptWhenEnd;
};

#undef LOCTEXT_NAMESPACE
