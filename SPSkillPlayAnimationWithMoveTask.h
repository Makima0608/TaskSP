// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Game/SPGame/SPAnimMoveXmlManager.h"
#include "Tasks/ablePlayAnimationTask.h"
#include "SPSkillPlayAnimationWithMoveTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPSkillPlayAnimationWithMoveTask : public UAblePlayAnimationTask
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;
	UFUNCTION(BlueprintNativeEvent)
	void OnTaskStartBP_Override(const UAbleAbilityContext* Context) const;

	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;
	UFUNCTION(BlueprintNativeEvent)
	void OnTaskEndBP_Override(UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Anim Move Ratio", EditCondition = "m_AnimationAsset!=nullptr"))
	float m_AnimMoveRatio = 1.1f;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Anim Close Friction", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_CloseFriction = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Z Move By Loc", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_OnlyZMoveByLoc = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="XY Move By Loc", EditCondition = "false"))
	bool m_OnlyXYMoveByLoc = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Direct Load Xml", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_DirectLoadXml = true;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Enable Root Motion Source", EditCondition = "false"))
	bool m_EnableRootMotion = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Root Motion Source Ratio", EditCondition = "false"))
	float m_RootMotionRatio = 1.0f;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Mesh 跟着胶囊体运动", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_IsMeshSmoothTogether = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Mesh 固定在胶囊体初始位置", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_IsMeshForceToCapsule = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="是否延续速度", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_IsRemainVelocity = false;

	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="是否开局Z轴速度归零", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_IsSetVelocityZtoZero = false;
#if WITH_EDITOR
	virtual FText GetTaskName() const { return LOCTEXT("SPSkillPlayAnimationTask", "Play Animation With Move"); }

#endif

	UPROPERTY()
	TSoftObjectPtr<USPAnimMovementXmlData> CurrentAnimMovementXmlData = nullptr;

	UPROPERTY()
	uint16 RootMotionSourceID;
protected:
	UPROPERTY(EditAnywhere, Category = "AnimMove", meta=(DisplayName="Enable Anim Move"))
	bool m_EnableAnimMove;
};

#undef LOCTEXT_NAMESPACE