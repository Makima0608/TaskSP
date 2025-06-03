// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "AlphaBlend.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableSetShaderParameterTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UAbleSetParameterValue;

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleSetShaderParameterTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleSetShaderParameterTaskScratchPad();
	virtual ~UAbleSetShaderParameterTaskScratchPad();

	/* All the Dynamic Materials we've affected. */
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	/* The previous value of any parameters we've set. */
	UPROPERTY()
	TArray<UAbleSetParameterValue*> PreviousValues;

	/* Blend In Time. */
	UPROPERTY()
	FAlphaBlend BlendIn;

	/* Blend Out Time. */
	UPROPERTY()
	FAlphaBlend BlendOut;
};

UCLASS()
class ABLECORESP_API UAbleSetShaderParameterTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleSetShaderParameterTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSetShaderParameterTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* Task On Tick. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const;
	
	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return false; } 	// I'd love to run this Async given the blending, etc. But creating the MID isn't thread safe. :(
	
	/* Returns true if our Task lasts for a single frame. */
	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;
	
	/* Returns true if our Task needs its Tick method called. */
	virtual bool NeedsTick() const override { return !IsSingleFrame(); }
	
	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for this Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler stat ID of our Task. */
	virtual TStatId GetStatId() const override;

	/* Get our Value to set.*/
	const UAbleSetParameterValue* GetParam() const { return m_Value; }
#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleSetShaderParameterCategory", "Material"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleSetShaderParameterTask", "Set Material Parameter"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleSetShaderParameterTaskDesc", "Sets a dynamic parameter on the Target's material instance."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(82.0f / 255.0f, 137.0f / 255.0f, 237.0f / 255.0f); }

	// UObject Overrides. 
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
#endif

private:
	/* Helper Method to cache current shader parameter values. */
	UAbleSetParameterValue* CacheShaderValue(UAbleSetShaderParameterTaskScratchPad* ScratchPad, UMaterialInstanceDynamic* DynMaterial) const;
	
	/* Helper method to set Shader parameters. */
	void InternalSetShaderValue(const TWeakObjectPtr<const UAbleAbilityContext>& Context, UMaterialInstanceDynamic* DynMaterial, UAbleSetParameterValue* Value, UAbleSetParameterValue* PreviousValue, float BlendAlpha) const;
	
	/* Helper method, returns true if the Material has the parameter this Task is looking for. */
	bool CheckMaterialHasParameter(UMaterialInterface* Material) const;

	/* Bind our Dynamic Delegates. */
	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;
protected:
	/* The name of our Shader Parameter. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Name"))
	FName m_ParameterName;

	/* The Shader Parameter value to set. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Parameter", meta=(DisplayName="Value"))
	UAbleSetParameterValue* m_Value;

	/* When setting the Shader parameter, this is the blend we will use as we transition over. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Blend In"))
	FAlphaBlend m_BlendIn;

	/* If true, restore the value of the variable when we started the task. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Restore Value On End"))
	bool m_RestoreValueOnEnd;

	/* If restoring the value at the end, this blend is used when transition back to the original value. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (DisplayName = "Blend Out", EditCondition ="m_RestoreValueOnEnd"))
	FAlphaBlend m_BlendOut;
};

#undef LOCTEXT_NAMESPACE