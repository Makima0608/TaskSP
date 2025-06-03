// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePossessionTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAblePossessionTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblePossessionTaskScratchPad();
	virtual ~UAblePossessionTaskScratchPad();

	/* The controller we've possessed. */
	UPROPERTY(transient)
	TWeakObjectPtr<APlayerController> PossessorController;
};

UCLASS()
class ABLECORESP_API UAblePossessionTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAblePossessionTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePossessionTask();

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

	/* Returns the realm our Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for our Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual  TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePossessionTaskCategory", "Misc"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePossessionTask", "Possess"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePossessionTaskDesc", "Causes the player to possess the target."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(204.0f / 255.0f, 169.0f / 255.0f, 45.0f / 255.0f); }
#endif
protected:
	/* The Context Target that will be possessing the target. */
	UPROPERTY(EditAnywhere, Category = "Possession", meta = (DisplayName = "Possessor"))
	TEnumAsByte<EAbleAbilityTargetType> m_Possessor;

	/* The Context Target that will be possessed. */
	UPROPERTY(EditAnywhere, Category="Possession", meta=(DisplayName="Possession Target"))
	TEnumAsByte<EAbleAbilityTargetType> m_PossessionTarget;

	/* If true, the Possession is undone at the end of the Task. */
	UPROPERTY(EditAnywhere, Category = "Possession", meta = (DisplayName = "UnPossess On End"))
	bool m_UnPossessOnEnd;
};

#undef LOCTEXT_NAMESPACE