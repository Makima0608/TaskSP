// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "AlphaBlend.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableIgnoreInputTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleIgnoreInputTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleIgnoreInputTaskScratchPad();
	virtual ~UAbleIgnoreInputTaskScratchPad();

    UPROPERTY(transient)
    TArray<TWeakObjectPtr<class APawn>> Pawns;
};

UCLASS()
class ABLECORESP_API UAbleIgnoreInputTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleIgnoreInputTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleIgnoreInputTask();

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
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleIgnoreInputTaskCategory", "Movement"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleIgnoreInput", "Ignore Input"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleIgnoreInputTaskDesc", "Ignores move/look input while the task is active."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0, 0.69, 0.4f); } // Peach

	// UObject Overrides
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:
    UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Move Input"))
    bool m_MoveInput = true;

	UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Look Input"))
	bool m_LookInput = false;

    UPROPERTY(EditAnywhere, Category = "Ignore Input", meta = (DisplayName = "Input"))
    bool m_Input = false;
};

#undef LOCTEXT_NAMESPACE
