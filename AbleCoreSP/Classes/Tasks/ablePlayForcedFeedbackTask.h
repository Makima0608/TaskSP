// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "AlphaBlend.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePlayForcedFeedbackTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblePlayForcedFeedbackTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblePlayForcedFeedbackTaskScratchPad();
	virtual ~UAblePlayForcedFeedbackTaskScratchPad();

    UPROPERTY(transient)
    TArray<TWeakObjectPtr<class APlayerController>> Controllers;
};

UCLASS()
class ABLECORESP_API UAblePlayForcedFeedbackTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAblePlayForcedFeedbackTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePlayForcedFeedbackTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
		
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

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
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePlayForcedFeedbackCategory", "Player"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePlayForcedFeedback", "Play Force Feedback (Haptics)"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePlayForcedFeedbackDesc", "Starts/stops a force feedback effect on the player controllers of the target pawns."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach
#endif
protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Effect"))
    class UForceFeedbackEffect* ForceFeedbackEffect;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Start Tag Name"))
    FName StartTagName;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Looping"))
    bool bLooping;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Ignore Time Dilation"))
    bool bIgnoreTimeDilation;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Play while Paused"))
    bool bPlayWhilePaused;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Stop Tag Name"))
    FName StopTagName;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Force Feedback", meta = (DisplayName = "Stop On Task Exit"))
    bool StopOnTaskExit;
};

#undef LOCTEXT_NAMESPACE
