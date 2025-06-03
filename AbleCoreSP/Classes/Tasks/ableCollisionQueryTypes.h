// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "ableAbilityContext.h"
#include "ableAbilityTypes.h"
#include "UnLuaInterface.h"
#include "Targeting/ableTargetingBase.h"

#include "UObject/ObjectMacros.h"

#include "ableCollisionQueryTypes.generated.h"

struct FAbleQueryResult;

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Base class for all our Collision Query Shapes. */
UCLASS(Abstract)
class ABLECORESP_API UAbleCollisionShape : public UObject
{
	GENERATED_BODY()
public:
	UAbleCollisionShape(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionShape();

	/* Perform the Synchronous Query.*/
	virtual FTransform DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform = FTransform::Identity, bool bOverrideTransform = false) const { return FTransform::Identity; }
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const { return FTraceHandle(); }

	/* Returns true, or false, if this Query is Async or not. */
	FORCEINLINE bool IsAsync() const { return m_UseAsyncQuery; }
	
	/* Returns the World this Query is occurring in.*/
	UWorld* GetQueryWorld(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	/* Helper method to help process Async Query. */
	virtual void ProcessAsyncOverlaps(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAbleQueryResult>& OutResults) const;

	/* Returns Query Information */
    const FAbleAbilityTargetTypeLocation& GetQueryLocation() const { return m_QueryLocation; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability);
	
#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const { return FString(TEXT("None")); }

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const {}

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);

	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	UPROPERTY(EditInstanceOnly, Category="Query", meta=(DisplayName="Query Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_QueryLocation;

	UPROPERTY()
	FGetAbleTargetLocation m_QueryLocationDelegate;

	UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channel Present"))
	TEnumAsByte<EAbleChannelPresent> m_ChannelPresent;
	
    UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
    TArray<TEnumAsByte<ESPAbleTraceType>> m_CollisionChannels;

	/* If true, the query is placed in the Async queue. This can help performance by spreading the query out by a frame or two. */
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async Query"))
	bool m_UseAsyncQuery;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Box", ShortToolTip = "A box based query volume."))
class ABLECORESP_API UAbleCollisionShapeBox : public UAbleCollisionShape
{
	GENERATED_BODY()
public:
	UAbleCollisionShapeBox(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionShapeBox();

	/* Perform the Synchronous Query.*/
	virtual FTransform DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform = FTransform::Identity, bool bOverrideTransform = false) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability);

	const FVector& HalfExtents() const { return m_HalfExtents; }
#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Half Extents of the Box */
	UPROPERTY(EditInstanceOnly, Category = "Box", meta = (DisplayName = "Half Extents", AbleBindableProperty))
	FVector m_HalfExtents;

	UPROPERTY()
	FGetAbleVector m_HalfExtentsDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Sphere", ShortToolTip = "A sphere based query volume."))
class ABLECORESP_API UAbleCollisionShapeSphere : public UAbleCollisionShape
{
	GENERATED_BODY()
public:
	UAbleCollisionShapeSphere(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionShapeSphere();

	/* Perform the Synchronous Query.*/
	virtual FTransform DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform = FTransform::Identity, bool bOverrideTransform = false) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability);
#if WITH_EDITOR
	/* Text name for shape. */	
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Radius of the Sphere */
	UPROPERTY(EditInstanceOnly, Category = "Sphere", meta = (DisplayName = "Radius", AbleBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Capsule", ShortToolTip = "A capsule based query volume."))
class ABLECORESP_API UAbleCollisionShapeCapsule : public UAbleCollisionShape
{
	GENERATED_BODY()
public:
	UAbleCollisionShapeCapsule(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionShapeCapsule();

	/* Perform the Synchronous Query.*/
	virtual FTransform DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform = FTransform::Identity, bool bOverrideTransform = false) const override;
	
	/* Perform the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability);

#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* Height of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Height", AbleBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAbleFloat m_HeightDelegate;

	/* Radius of the Capsule */
	UPROPERTY(EditInstanceOnly, Category = "Capsule", meta = (DisplayName = "Radius", AbleBindableProperty))
	float m_Radius;

	UPROPERTY()
	FGetAbleFloat m_RadiusDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Cone", ShortToolTip = "A cone based query volume, supports angles > 180 degrees."))
class ABLECORESP_API UAbleCollisionShapeCone : public UAbleCollisionShape
{
	GENERATED_BODY()
public:
	UAbleCollisionShapeCone(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleCollisionShapeCone();

	/* Do the Synchronous Query.*/
	virtual FTransform DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform = FTransform::Identity, bool bOverrideTransform = false) const override;
	
	/* Do the Async Query.*/
	virtual FTraceHandle DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const override;

	/* Helper method to help process our Async Query*/
	virtual void ProcessAsyncOverlaps(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAbleQueryResult>& OutResults) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
#if WITH_EDITOR
	/* Text name for shape. */
	virtual const FString DescribeShape() const;

	/* Called by Ability Editor to allow any special logic. */
    virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const override;

	/* Data Validation Tests */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors);
#endif

protected:
	/* The Field of View (Angle/Azimuth) of the cone, in degrees. Supports Angles greater than 180 degrees. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "FOV", ClampMin = 1.0f, ClampMax = 360.0f, AbleBindableProperty))
	float m_FOV; // Azimuth

	UPROPERTY()
	FGetAbleFloat m_FOVDelegate;

	/* Length of the Cone. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Length", ClampMin = 0.1f, AbleBindableProperty))
	float m_Length;

	UPROPERTY()
	FGetAbleFloat m_LengthDelegate;

	/* Height of the Cone */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Height", ClampMin = 0.1f, EditCondition="!m_Is2DQuery", AbleBindableProperty))
	float m_Height;

	UPROPERTY()
	FGetAbleFloat m_HeightDelegate;

	/* If true, the Height of the cone is ignored. */
	UPROPERTY(EditInstanceOnly, Category = "Cone", meta = (DisplayName = "Is 2D Query"))
	bool m_Is2DQuery;

	/* 3D Slice acts similar to a cone, except it does not pinch in towards the origin, but is instead an equal height throughout the volume (like a slice of cake).*/
	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "Use 3D Slice"))
	bool m_3DSlice;
};

#undef LOCTEXT_NAMESPACE