// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "AlphaBlend.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableTurnToTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Helper Struct */
USTRUCT()
struct FTurnToTaskEntry
{
	GENERATED_USTRUCT_BODY()
public:
	FTurnToTaskEntry() {};
	FTurnToTaskEntry(AActor* InActor, FRotator InTarget, const bool InDirection = false)
		: Actor(InActor),
		Target(InTarget)
	{ }
	TWeakObjectPtr<AActor> Actor;
	FRotator Target;
};

/* Scratchpad for our Task. */
UCLASS(Transient)
class ABLECORESP_API UAbleTurnToTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleTurnToTaskScratchPad();
	virtual ~UAbleTurnToTaskScratchPad();

	/* Any turns in progress. */
	UPROPERTY(transient)
	TArray<FTurnToTaskEntry> InProgressTurn;

	/* Blend to use for turns. */
	UPROPERTY(transient)
	FAlphaBlend TurningBlend;
};

UCLASS()
class ABLECORESP_API UAbleTurnToTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleTurnToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTurnToTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* On Task Tick. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task only lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;
	
	/* Returns true if our Task needs its tick method called. */
	virtual bool NeedsTick() const override { return !IsSingleFrame(); }

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for this Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

	virtual void TurnToSetActorRotation(const TWeakObjectPtr<AActor> TargetActor, const FRotator& TargetRotation) const;
	
	virtual void TurnAnimation(const TWeakObjectPtr<AActor> TargetActor, const float DeltaYaw) const ;
	
	UFUNCTION(BlueprintNativeEvent)
	float GetCustomAngularVelocity(AActor* TargetActor) const;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleTurnToTaskCategory", "Movement"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleTurnToTask", "Turn To"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleTurnToTaskDesc", "Turns the task target towards the destination target over the duration of the task. If this is a single frame, then it will snap the character to the facing."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(104.0f / 255.0f, 88.0f / 255.0f, 237.0f / 255.0f); }

	// UObject Overrides
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif
protected:
	/* Helper method to get our Target rotation. */
	virtual FRotator GetTargetRotation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const;

	/* Helper method for returning a Target Vector. */
	FVector GetTargetVector(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const;

	/* The Target we want to rotate towards. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Rotation Target"))
	TEnumAsByte<EAbleAbilityTargetType> m_RotationTarget;

	/* If true, use the Rotation Vector. Otherwise, use the Rotation Target. Can be overriden at runtime. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Use Rotation Vector", AbleBindableProperty))
	bool m_UseRotationVector;

	UPROPERTY()
	FGetAbleBool m_UseRotationVectorDelegate;

	/* If you don't have a target, but instead a direction you want to rotate to. You can use this field. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Rotation Vector", EditConditional = m_useRotationVector, AbleBindableProperty))
	FVector m_RotationVector;

	UPROPERTY()
	FGetAbleVector m_RotationVectorDelegate;

	/* If true, we'll continually try to turn towards this target for the duration of the task. If false, we only take the rotation value at the start of the task. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Track Target"))
	bool m_TrackTarget;

	/* An offset you can apply for the final rotation target. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Rotation Offset"))
	FRotator m_RotationOffset;

	/* If true, we will update the Yaw on actors this task affects. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Set Yaw"))
	bool m_SetYaw;

	/* If true, we will update the Pitch on actors this task affects. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Set Pitch"))
	bool m_SetPitch;

	/* The blend to use when we are turning. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Blend"))
	FAlphaBlend m_Blend;

	/* if true, use the Task Duration as the amount of time to complete the turn. Otherwise, the Blend time specified in the Blend structure will be used. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Use Task Duration As Blend Time"))
	bool m_UseTaskDurationAsBlendTime;

	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Turn To In Client"))
	bool m_bOpenClientTurn;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;

	/* An Rotation the Actor Self. */
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "Actor Self Rotation"))
	bool m_UseSelfRotation;

	UPROPERTY(EditAnywhere, Category = "Turn To By Speed", meta = (DisplayName = "是否使用角速度"))
	bool m_bUseAngularVelocity;
	
	UPROPERTY(EditAnywhere, Category = "Turn To By Speed", meta = (DisplayName = "角速度(大于0生效)"))
	float m_AngularVelocity;

	UPROPERTY(EditAnywhere, Category = "Turn To By Speed", meta = (DisplayName = "是否需要缩放角速度"))
	bool m_bScaleAngularVelocity;
};

#undef LOCTEXT_NAMESPACE
