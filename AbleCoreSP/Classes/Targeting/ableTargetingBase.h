// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbilityContext.h"
#include "ableAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"

#include "ableTargetingBase.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTargeting"

class UPrimitiveComponent;
class UAbleAbilityTargetingFilter;

/* Base class for all our Targeting volumes/types. */
UCLASS(Abstract, EditInlineNew)
class ABLECORESP_API UAbleTargetingBase : public UObject
{
	GENERATED_BODY()
public:
	UAbleTargetingBase(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleTargetingBase();

	void CleanUp();

	/* Override in child classes, this method should find any targets according to the targeting volume/rules.*/
	virtual void FindTargets(UAbleAbilityContext& Context) const { };
	
	virtual bool ShouldClearTargets() const;

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldClearTargetsBP() const;
	
	void GetCollisionObjectParams(const UAbleAbilityContext* Context, FCollisionObjectQueryParams& outParams) const;
	
	/* Returns the Context Target Type to use as the source for any location-based logic. */
	FORCEINLINE EAbleAbilityTargetType GetSource() const { return m_Location.GetSourceTargetType(); }

	/* Returns the Collision Channels to execute this query on. */
	FORCEINLINE const TArray<TEnumAsByte<ESPAbleTraceType>>& GetCollisionChannels() const { return m_CollisionChannels; }

	/* Returns true if Targeting is using an Async query. */
	FORCEINLINE bool IsUsingAsync() const { return m_UseAsync; }

	/* Returns the range of this Targeting query.*/
	FORCEINLINE float GetRange() const { return m_Range; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsResetForIteration() const { return m_FetchTargetForIteration; }

#if WITH_EDITOR
	// UObject Overrides.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	// Fix up our flags. 
	bool FixUpObjectFlags();

	/* Called by Ability Editor to allow any special logic. */
	virtual void OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const {}
#endif
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability);
protected:
	/* Method for Child classes to override. This should calculate and return the range for the query. */
	virtual float CalculateRange() const { return 0.0f; }

	/* Runs all Targeting Filters. */
	void FilterTargets(UAbleAbilityContext& Context) const;

	/* If true, the targeting range will be automatically calculated using shape, rotation, and offset information. This does not include socket offsets. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Auto-calculate Range"))
	bool m_AutoCalculateRange;

	/* Range is primarily used by AI and other systems that have a target they wish to check against to avoid the full cost of the query. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Range", EditCondition = "!m_AutoCalculateRange", AbleBindableProperty))
	float m_Range;

	/* Whether to Remove Duplicate Targets - Default - True. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "去除重复Target"))
	bool m_bRemoveDuplicates = true;

	UPROPERTY()
	FGetAbleFloat m_RangeDelegate;

	/* If true, when calculating the range, it will be calculated as a 2D distance (XY plane) rather than 3D. */
	UPROPERTY(EditInstanceOnly, Category = "Targeting|Range", meta = (DisplayName = "Calculate as 2D", EditCondition = "m_AutoCalculateRange"))
	bool m_CalculateAs2DRange;

	/* Where to begin our targeting query from. */
	UPROPERTY(EditInstanceOnly, Category="Targeting", meta=(DisplayName="Query Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAbleTargetLocation m_LocationDelegate;

	UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channel Present"))
	TEnumAsByte<EAbleChannelPresent> m_ChannelPresent;
	
	/* The collision channels to use when we perform the query. */
	UPROPERTY(EditInstanceOnly, Category = "Query", meta = (DisplayName = "Collision Channels"))
	TArray<TEnumAsByte<ESPAbleTraceType>> m_CollisionChannels;

	/* Filters to run the initial results through. These are executed in order. */
	UPROPERTY(EditInstanceOnly, Instanced, Category = "Targeting", meta = (DisplayName = "Filters"))
	TArray<UAbleAbilityTargetingFilter*> m_Filters;

	/* 
	*  If true, runs the targeting query on as an Async query rather than blocking the game thread. 
	*  This can lead to a performance increase but will cause a one frame delay before the query
	*  completes. If you don't need frame perfect execution - this is probably worth the small delay.
	*/
	UPROPERTY(EditInstanceOnly, Category = "Optimization", meta = (DisplayName = "Use Async"))
	bool m_UseAsync;
	
	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;

	UPROPERTY(EditDefaultsOnly, Category = "Iteration", meta = (DisplayName = "Reset Target For Iteration"))
	bool m_FetchTargetForIteration = false;

	UPROPERTY(EditInstanceOnly, Category = "Targeting", meta = (DisplayName = "Save Target Location"))
	bool m_bSaveTargetLocation;
};

#undef LOCTEXT_NAMESPACE
