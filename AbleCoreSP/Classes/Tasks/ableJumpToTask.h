// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Components/AudioComponent.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableJumpToTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleJumpToScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleJumpToScratchPad();
	virtual ~UAbleJumpToScratchPad();

	FVector CurrentTargetLocation;
	TArray<TWeakObjectPtr<APawn>> JumpingPawns;
};

UENUM()
enum EAbleJumpToTarget
{
	JTT_Actor UMETA(DisplayName = "Actor"),
	JTT_Location UMETA(DisplayName = "Location")
};

UCLASS()
class ABLECORESP_API UAbleJumpToTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleJumpToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleJumpToTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	/* Tick our Task. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const;

	/* Is our Task finished yet? */
	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; }

	/* Returns true if our Task is a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleJumpToTaskCategory", "Movement"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleJumpToTask", "Jump To [已废弃]"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleJumpToTaskDesc", "(EXPERIMENTAL) Causes an actor to jump/leap, using physics and a target destination."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(74.0 / 255.0f, 63.0 / 255.0f, 163.0f / 255.0f); }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
	
	virtual bool IsDeprecated() const override { return true; }
#endif

protected:
	FVector GetTargetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
	void SetPhysicsVelocity(const TWeakObjectPtr<const UAbleAbilityContext>& Context, APawn* Target, const FVector& EndLocation, UAbleJumpToScratchPad* ScratchPad, bool addToScratchPad = true) const;

	/* Which Target to jump towards: Location, or Actor.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Type"))
	TEnumAsByte<EAbleJumpToTarget> m_TargetType;

	/* The Target Actor to move to. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Actor", EditCondition = "m_TargetType == EAbleJumpToTarget::Actor"))
	TEnumAsByte<EAbleAbilityTargetType> m_TargetActor;

	/* Follow the actor, in mid air if need be to ensure we land near the actor.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Track Actor", EditCondition = "m_TargetType == EAbleJumpToTarget::Actor"))
	bool m_TrackActor;

	/* The Target Location to move to. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Location", EditCondition = "m_TargetType == EAbleJumpToTarget::Location", AbleBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAbleVector m_TargetLocationDelegate;

	/* An offset, from our direction to our target, on where to land. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Target Offset"))
	float m_TargetActorOffset;

	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Jump Height", ClampMin = 0.0f, AbleBindableProperty))
	float m_JumpHeight;

	UPROPERTY()
	FGetAbleFloat m_JumpHeightDelegate;

	/* What our speed should be if using Physics to drive movement. */
	UPROPERTY(EditAnywhere, Category = "Move|Physics", meta = (DisplayName = "Speed", ClampMin = 0.0f, AbleBindableProperty))
	float m_Speed;

	UPROPERTY()
	FGetAbleFloat m_SpeedDelegate;

	/* How close we need to be to our Target for this task to be completed or when we need to update our target location.*/
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Acceptable Radius", ClampMin = 0.0f))
	float m_AcceptableRadius;

	/* If true, ignore the Z axis when checking for new locations and if we're at our end goal. This is generally optimal and should be left on unless you need 3D checks due to verticality. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Use 2D Distance Checks", ClampMin = 0.0f))
	bool m_Use2DDistanceChecks;

	/* If true, don't wait till we reach our goal. Set the jump velocity and then exit. */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "End Task Immediately", ClampMin = 0.0f))
	bool m_EndTaskImmediately;

	/* If true, cancel our velocity when we're interrupted.  */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Cancel Jump On Interrupt"))
	bool m_CancelMoveOnInterrupt;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE