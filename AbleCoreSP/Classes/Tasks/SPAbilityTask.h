// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "SPAbilityTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UCLASS(Blueprintable, Transient)
class ABLECORESP_API USPAbilityTaskScratchPad : public UAbleAbilityTaskScratchPad, public IUnLuaInterface
{
	GENERATED_BODY()
public:
	USPAbilityTaskScratchPad();
	virtual ~USPAbilityTaskScratchPad();

	virtual bool SkipObjectReferencer_Implementation() const override;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, hidecategories = ("Optimization"))
class ABLECORESP_API USPAbilityTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()
public:
	USPAbilityTask(const FObjectInitializer& ObjectInitializer);
	virtual ~USPAbilityTask();

	virtual bool SkipObjectReferencer_Implementation() const override;

	virtual void Clear() override;

	UFUNCTION(BlueprintNativeEvent)
	void ClearBP();

	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta=(DisplayName="OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float DeltaTime) const;

	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	virtual TSubclassOf<UAbleAbilityTaskScratchPad> GetTaskScratchPadClass(const UAbleAbilityContext* Context) const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskScratchPadClass"))
	TSubclassOf<UAbleAbilityTaskScratchPad> GetTaskScratchPadClassBP(const UAbleAbilityContext* Context) const;

	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "CreateScratchPad", DeprecatedFunction, DeprecationMessage="CreateScratchPad is deprecated. Please use GetTaskScratchPadClass instead."))
	USPAbilityTaskScratchPad* CreateScratchPadBP(UAbleAbilityContext* Context) const;

	virtual void ResetScratchPad(UAbleAbilityTaskScratchPad* ScratchPad) const override;

	UFUNCTION(BlueprintNativeEvent, Category = "Able|Custom Task", meta = (DisplayName = "ResetScratchPad"))
	void ResetScratchPadBP(UAbleAbilityTaskScratchPad* ScratchPad) const;

	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetScratchPad"))
	USPAbilityTaskScratchPad* GetScratchPad(UAbleAbilityContext* Context) const;

	virtual bool IsAsyncFriendly() const override { return false; }

	virtual bool IsSingleFrame() const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskCategory"))
	FText GetTaskCategoryBP() const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskName"))
	FText GetTaskNameBP() const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetDescriptiveTaskName"))
	FText GetDescriptiveTaskNameBP() const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskDescription"))
	FText GetTaskDescriptionBP() const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskColor"))
	FLinearColor GetTaskColorBP() const;

	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskStartTime"))
	float GetTaskStartTimeBP() const { return GetStartTime(); }

	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskEndTime"))
	float GetTaskEndTimeBP() const { return GetEndTime(); }

	UFUNCTION(BlueprintPure, Category = "Able|Custom Task", meta = (DisplayName = "GetTaskLength"))
	float GetTaskLengthBP() const { return GetEndTime() - GetStartTime(); }

	virtual float GetEndTime() const override;

	UFUNCTION(BlueprintNativeEvent)
	float GetEndTimeBP() const;

#if WITH_EDITOR
	virtual FText GetTaskCategory() const override;

	virtual FText GetTaskName() const override;

	virtual FText GetDescriptiveTaskName() const override;

	virtual FText GetTaskDescription() const override;

	virtual FLinearColor GetTaskColor() const override;

	virtual float GetEstimatedTaskCost() const override { return UAbleAbilityTask::GetEstimatedTaskCost() + ABLETASK_EST_BP_EVENT; }

	// virtual EVisibility ShowEndTime() const { return EVisibility::Collapsed; }

	virtual bool CanEditTaskRealm() const override { return true; }

    EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors);

	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;
#endif

	UFUNCTION(BlueprintNativeEvent, Category = "Able|Custom Task", meta = (DisplayName = "OnAbilityEditorTick"))
	void OnAbilityEditorTickBP(const UAbleAbilityContext* Context, float DeltaTime) const;
	
#if WITH_EDITORONLY_DATA
	virtual bool HasCompactData() const override { return true; }
#endif

};

#undef LOCTEXT_NAMESPACE