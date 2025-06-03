// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Tasks/IAbleAbilityTask.h"
#include "SPSetMeshVisibilityTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

UCLASS()
class FEATURE_SP_API USPSetMeshVisibilityTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
	
public:
	USPSetMeshVisibilityTask(const FObjectInitializer& ObjectInitializer);
	
	virtual ~USPSetMeshVisibilityTask();

	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

	virtual TStatId GetStatId() const override;

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

#if WITH_EDITOR

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPSetMeshVisibilityTask", "Visibility"); }

	virtual FText GetTaskName() const override { return LOCTEXT("USPSetMeshVisibilityTask", "Set Mesh Visibility"); }

	virtual bool CanEditTaskRealm() const override { return true; }
#endif

protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (DisplayName = "目标"))
	TEnumAsByte<EAbleAbilityTargetType> m_Target = EAbleAbilityTargetType::ATT_Self;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "开始时可见性"))
	bool m_StartVisibility;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "结束时可见性", EditCondition = "m_IsDuration==true&&m_ResetOnEnd==true"))
	bool m_EndVisibility;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "是否设置碰撞"))
	bool m_bSetCollision;
	
	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "是否是Duration"))
	bool m_IsDuration;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "是否在结束时重置", EditCondition = "m_IsDuration==true"))
	bool m_ResetOnEnd;
	
	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "是否仅仅只设置Mesh"))
	bool m_SetMeshOnly;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "是否同时设置布告板组件可见性"))
	bool m_SetBillboard;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (DisplayName = "可见性是否推送到mesh的子项"))
	bool m_PropagateToChildren;
	
	UPROPERTY(VisibleAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE