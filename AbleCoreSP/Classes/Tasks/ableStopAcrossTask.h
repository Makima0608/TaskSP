// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableAbility.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableStopAcrossTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UCLASS()
class ABLECORESP_API UAbleStopAcrossTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleStopAcrossTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleStopAcrossTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/* Returns true if our Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	FORCEINLINE const TArray<const UAbleAbilityTask*>& GetAcrossTasks() const { return m_AcrossTasks; }

	void AddAcrossTask(const UAbleAbilityTask* Task);

	void RemoveAcrossTask(const UAbleAbilityTask* Task);

	void ClearAcrossTasks() { m_AcrossTasks.Empty(); }

	/* Returns the StatId for this Task, used by the Profiler. */
	virtual TStatId GetStatId() const;

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;
	
#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleStopAcrossCategory", "Segment"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleStopAcrossTask", "Stop Across Task"); }
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleStopAcrossTaskDesc", "Stop Across Task"); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(1.0f, 1.0f, 1.0f); }
	
	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }

	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;
	
#endif
protected:
	UPROPERTY(VisibleAnywhere, Category = "Segment", meta = (DisplayName = "AcrossTasks"))
	TArray<const UAbleAbilityTask*> m_AcrossTasks;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE
