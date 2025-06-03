// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "AITypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "NavigationSystemTypes.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableMoveToTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleMoveToScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleMoveToScratchPad();
	virtual ~UAbleMoveToScratchPad();

	FVector CurrentTargetLocation;

	TArray<TPair<uint32, TWeakObjectPtr<APawn>>> AsyncQueryIdArray;
	TArray<TPair<FAIRequestID, TWeakObjectPtr<APawn>>> ActiveMoveRequests;
	TArray<TWeakObjectPtr<AActor>> ActivePhysicsMoves;
	TArray<TPair<uint32, FNavPathSharedPtr>> CompletedAsyncQueries;

	void OnNavPathQueryFinished(uint32 Id, ENavigationQueryResult::Type typeData, FNavPathSharedPtr PathPtr);
	FNavPathQueryDelegate NavPathDelegate;
};

UENUM()
enum EAbleMoveToTarget
{
	MTT_Actor UMETA(DisplayName = "Actor"),
	MTT_Location UMETA(DisplayName = "Location")
};

UENUM()
enum EAblePathFindingType // Navigation System's ENUM isn't exported, so we duplicate it.
{
	Regular UMETA(DisplayName = "Regular"),
	Hierarchical UMETA(DisplayName = "Hierarchical")
};

UCLASS()
class ABLECORESP_API UAbleMoveToTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleMoveToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleMoveToTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual bool NeedsTick() const { return true; }

	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const;

	/* Returns if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; }

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual float GetEndTime() const { return GetStartTime() + FMath::Max<float>(m_TimeOut, 0.5f); }

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Returns the category for our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleMoveToTaskCategory", "Movement"); }

	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleMoveToTask", "Move To"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleMoveToTaskDesc", "(EXPERIMENTAL) Moves an Actor from one location to another, either using physics or the nav mesh."); }

	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(108.0f / 255.0f, 102.0 / 255.0f, 196.0f / 255.0f); }

	/* Returns true if the user is allowed to edit the realm for this Task. */
	virtual bool CanEditTaskRealm() const override { return true; }
#endif

protected:
	FVector GetTargetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
	void StartPathFinding(const TWeakObjectPtr<const UAbleAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAbleMoveToScratchPad* ScratchPad) const;
	void SetPhysicsVelocity(const TWeakObjectPtr<const UAbleAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAbleMoveToScratchPad* ScratchPad) const;


	/* Which Target to move towards: Location, or Actor.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Type"))
	TEnumAsByte<EAbleMoveToTarget> m_TargetType;

	/* The Target Actor to move to. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Actor", EditCondition = "m_TargetType == EAbleMoveToTarget::Actor"))
	TEnumAsByte<EAbleAbilityTargetType> m_TargetActor;

	/* The Target Location to move to. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Target Location", EditCondition = "m_TargetType == EAbleMoveToTarget::Location", AbleBindableProperty))
	FVector m_TargetLocation;

	UPROPERTY()
	FGetAbleVector m_TargetLocationDelegate;

	/* Whether or not to continually update our end point. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Update Target"))
	bool m_UpdateTargetPerFrame;

	/* How close we need to be to our Target for this task to be completed.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Acceptable Radius", ClampMin = 0.0f))
	float m_AcceptableRadius;

	/* If true, ignore the Z axis when checking for new locations and if we're at our end goal. This is generally optimal and should be left on unless you need 3D path finding. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Use 2D Distance Checks", ClampMin = 0.0f))
	bool m_Use2DDistanceChecks;

	/* Whether or not to use Navigational Pathing or simple physics.*/
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Use Nav Pathing"))
	bool m_UseNavPathing;

	/* If using Nav pathing, whether to abort or not if no path can be found. */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "Cancel On No Path", EditCondition = "m_UseNavPathing"))
	bool m_CancelOnNoPathAvailable;

	/* If using Nav pathing, which mode to use when finding a path. */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "NavPath Finding Type", EditCondition = "m_UseNavPathing"))
	TEnumAsByte<EAblePathFindingType> m_NavPathFindingType;

	/* If true, use an Async Path Finding Query vs a blocking one.  */
	UPROPERTY(EditAnywhere, Category = "Move|NavPath", meta = (DisplayName = "Use Async NavPath Query", EditCondition = "m_UseNavPathing"))
	bool m_UseAsyncNavPathFinding;

	/* What our speed should be if using Physics to drive movement. */
	UPROPERTY(EditAnywhere, Category = "Move|Physics", meta = (DisplayName = "Speed", ClampMin = 0.0f, AbleBindableProperty))
	float m_Speed;

	UPROPERTY()
	FGetAbleFloat m_SpeedDelegate;

	/* Timeout for this Task. A value of 0.0 means there is no time out. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Timeout"))
	float m_TimeOut;

	/* If true, cancel our velocity when we're interrupted.  */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (DisplayName = "Cancel Move On Interrupt"))
	bool m_CancelMoveOnInterrupt;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "Realm"))
	TEnumAsByte<EAbleAbilityTaskRealm> m_TaskRealm;
};

#undef LOCTEXT_NAMESPACE