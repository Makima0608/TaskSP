// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "UnLuaInterface.h"
#include "Tasks/IAbleAbilityTask.h"
#include "SPAddBuffTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"
/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPAddBuffTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:
	USPAddBuffTask(const FObjectInitializer& ObjectInitializer);
	
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;
	
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
						   const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context,
					 const EAbleAbilityTaskResult result) const;
	
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	
	virtual FText GetTaskName() const override { return LOCTEXT("USPAddBuffTask", "AddBuff"); }

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPAddBuffTask", "Buff"); }
	
	virtual bool CanEditTaskRealm() const override { return true; }
#endif

	int32 GetBuffId() const { return m_BuffID; }

	int32 GetLayer() const { return m_Layer; }

	bool IsRemoveOnEnd() const { return m_bRemoveOnEnd; }

	TEnumAsByte<EAbleAbilityTargetType> GetInstigator() const { return m_Instigator; } 
	
protected:
	UPROPERTY(EditAnywhere, Category = "Buff", meta = (DisplayName = "BuffID"))
	int m_BuffID;

	UPROPERTY(EditAnywhere, Category = "Buff", meta = (DisplayName = "层数"))
	int m_Layer;

	UPROPERTY(EditAnywhere, Category = "Buff", meta = (DisplayName = "结束移除"))
	bool m_bRemoveOnEnd = false;

	UPROPERTY(EditAnywhere, Category = "Buff", meta = (DisplayName = "Instigator"))
	TEnumAsByte<EAbleAbilityTargetType> m_Instigator;
};

#undef LOCTEXT_NAMESPACE
