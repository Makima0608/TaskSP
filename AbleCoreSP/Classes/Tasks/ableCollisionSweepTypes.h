// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityContext.h"
#include "ableAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Targeting/ableTargetingBase.h"

#include "ableCollisionSweepTypes.generated.h"

struct FAbleQueryResult;

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Base class for all our Sweep Shapes. */
UCLASS(Abstract)
class ABLECORESP_API UAbleCollisionSweepShape : public UObject
{
	GENERATED_BODY()
public:
	UAbleCollisionSweepShape(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionSweepShape();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const { }
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const { return FTraceHandle(); }
	
	/* Retrieve the Async results and process them. */
	virtual void GetAsyncResults(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTraceHandle& Handle, TArray<FAbleQueryResult>& OutResults) const;

	/* Returns true if this shape is using Async. */
	FORCEINLINE bool IsAsync() const { return m_UseAsyncQuery; }

	/* Helper method to return the transform used in our Query. */
	void GetQueryTransform(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutTransform) const;

    const FAbleAbilityTargetTypeLocation& GetSweepLocation() const { return m_SweepLocation; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability);

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const { return FString(TEXT("None")); }

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const {}

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);

	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	/* The location of our Query. */
	UPROPERTY(EditInstanceOnly, Category="Sweep", meta=(DisplayName="Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_SweepLocation;

	UPROPERTY()
	FGetAbleTargetLocation m_SweepLocationDelegate;
	
	UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channel Present"))
	TEnumAsByte<EAbleChannelPresent> m_ChannelPresent;

    UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
    TArray<TEnumAsByte<ESPAbleTraceType>> m_CollisionChannels;

	/* If true, only return the blocking hit. Otherwise return all hits, including the blocking hit.*/
	UPROPERTY(EditInstanceOnly, Category = "Sweep", meta = (DisplayName = "Only Return Blocking Hit"))
	bool m_OnlyReturnBlockingHit;

	/* If true, the query is placed in the Async queue. This can help performance by spreading the query out by a frame or two. */
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async Query"))
	bool m_UseAsyncQuery;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Box", ShortToolTip = "A box based sweep volume."))
class ABLECORESP_API UAbleCollisionSweepBox : public UAbleCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAbleCollisionSweepBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionSweepBox();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AbleBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAbleVector m_HalfExtentsDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sphere", ShortToolTip = "A sphere based sweep volume."))
class ABLECORESP_API UAbleCollisionSweepSphere : public UAbleCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAbleCollisionSweepSphere(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionSweepSphere();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep. */
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Sphere */
	UPROPERTY(EditInstanceOnly, Category = "Sphere", meta = (DisplayName = "Radius", AbleBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based sweep volume."))
class ABLECORESP_API UAbleCollisionSweepCapsule : public UAbleCollisionSweepShape
{
	GENERATED_BODY()
public:
	UAbleCollisionSweepCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionSweepCapsule();

	/* Perform the Synchronous Sweep. */
	virtual void DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const override;
	
	/* Queue up the Async Sweep.*/
	virtual FTraceHandle DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

#if WITH_EDITOR
	/* Text name for Shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Radius", AbleBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;

	/* Height of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Height", AbleBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAbleFloat m_HeightDelegate;
};

#undef LOCTEXT_NAMESPACE