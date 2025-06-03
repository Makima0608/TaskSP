// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Targeting/ableTargetingBase.h"
#include "ablePlayParticleEffectParams.h"
#include "IAbleAbilityTask.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ObjectMacros.h"

#include "ablePlayParticleEffectTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"


class UParticleSystemComponent;
class UFXSystemComponent;

/* Scratchpad for our Task. */
UCLASS(Transient)
class ABLECORESP_API UAblePlayParticleEffectTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblePlayParticleEffectTaskScratchPad();
	virtual ~UAblePlayParticleEffectTaskScratchPad();

	/* All the Particle effects we've spawned. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UParticleSystemComponent>> SpawnedEffects;

	UPROPERTY(Transient)
	TMap<int, TWeakObjectPtr<UParticleSystemComponent>> SpawnedEffectsMap;
};

UENUM(BlueprintType)
enum class ESPPlayFallingType : uint8
{
	None,
	Hidden,
};

UCLASS()
class ABLECORESP_API UAblePlayParticleEffectTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()
public:
	UAblePlayParticleEffectTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePlayParticleEffectTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;
	
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
		
	/* Update our Task. */
	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;
	
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTickChangeTransform"))
	void OnTickChangeTransform(const UAbleAbilityContext* Context, float deltaTime) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const override { return false; }
	
	/* Returns true if the Task only lasts a single frame. */
	virtual bool IsSingleFrame() const override { return !m_DestroyAtEnd; }

	/* Returns true if our Task requires a Tick update.*/
	virtual bool NeedsTick() const override;
	
	/* Returns the realm of our Task. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return EAbleAbilityTaskRealm::ATR_Client; }

	/* Creates the Scratchpad for our Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const override;
	
	/* Returns the Profiler Stat ID of our Task. */
	virtual TStatId GetStatId() const override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAbleAbility* Ability ) override;

	/* Sets the Effect Template. */
	void SetEffectTemplate(UParticleSystem* Effect);
	
	/* Get our Parameter values. */
	const TArray<UAbleParticleEffectParam*>& GetParams() const { return m_Parameters; }

	UFUNCTION(BlueprintImplementableEvent)
	float GetShapeScale_Lua(AActor* Context) const;

	UFUNCTION(BlueprintImplementableEvent)
	UParticleSystem* GetCustomEffectTemplateByElementType_Lua(AActor* Context, const int CustomEffectId) const;

	UFUNCTION(BlueprintImplementableEvent)
	float OnPlaySFX_Lua(const FString& BankName, const FString& EventName, AActor* Context, bool bUsePosition, FVector Position) const;

	/*Get ParticleSystem */
	const UParticleSystem* GetParticleSystem() const { return m_EffectTemplate.Get(); }

	TSoftObjectPtr<UParticleSystem> GetSoftParticleSystem() const { return m_EffectTemplate; }
	
#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePlayParticleEffectTaskCategory", "Effects"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePlayParticleEffectTask", "Play Particle Effect"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePlayParticleEffectTaskDesc", "Plays a particle effect, can be attached to a bone."); }
	
	/* Returns a Rich Text version of the Task summary, for use within the Editor. */
	virtual FText GetRichTextTaskSummary() const;
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(149.0f / 255.0f, 61.0f / 255.0f, 73.0f / 255.0f); }

	/* Data Validation Tests. */
    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;

	/* Fix up our flags. */
	virtual bool FixUpObjectFlags() override;
	
	/* Sets destroy at end. */
	FORCEINLINE void SetDestroyAtEnd(bool DestroyAtEnd) { m_DestroyAtEnd = DestroyAtEnd; }

	/* Sets destroy immediately. */
	FORCEINLINE void SetDestroyImmediately(bool DestroyImmediately) { m_DestroyImmediately = DestroyImmediately; }
#endif

protected:
	bool UseWorldRotation(const UAbleAbilityContext& Context) const;
	
	void ApplyParticleEffectParameter(const UAbleParticleEffectParam* Parameter, const TWeakObjectPtr<const UAbleAbilityContext>& Context, UFXSystemComponent* EffectSystem, TWeakObjectPtr<UParticleSystemComponent>& CascadeComponent) const;

	void ResetPcsTransform(const UAbleAbilityContext* Context) const;

	virtual void OnAbilityPlayRateChanged(const UAbleAbilityContext* Context, float NewPlayRate) override;

	/* Particle Effect to play (has priority over a Niagara system if both are specified). */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (AbleBindableProperty, DisplayName="Effect Template"))
	TSoftObjectPtr<UParticleSystem> m_EffectTemplate;
	
	/*Use Custom Effect*/
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use Custom Effect"))
	bool m_UseCustomEffect;

	/*Use Custom Effect*/
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use Instigator Element Type"))
	bool m_UseInstigatorElementType;

	/*Use Custom Effect*/
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Custom Effect Id"))
	int m_CustomEffectId;

	UPROPERTY()
	FGetAbleParticleSystem m_EffectTemplateDelegate;

	UPROPERTY(EditAnywhere, Category = "Particle", meta=(DisplayName = "Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAbleTargetLocation m_LocationDelegate;
    
	/* If true, the particle effect will follow the transform of the socket. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Attach To Socket"))
	bool m_AttachToSocket;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Tick Change"))
	bool m_bTickChangeTransform;
	
	/* Uniform scale applied to the particle effect. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Scale", AbleBindableProperty))
	float m_Scale;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "AbsoluteSetScale"))
	bool m_AbsoluteSetScale = false;

	UPROPERTY()
	FGetAbleFloat m_ScaleDelegate;

    /* If non zero, used to calculate a scaling factor based on the target actor radius. */
    UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Dynamic Scale", AbleBindableProperty))
    float m_DynamicScaleSize;

	UPROPERTY()
	FGetAbleFloat m_DynamicScaleSizeDelegate;

	/* Whether or not we try to create an FX using the pool, or just try to spawn a new one.  Set this to false if you expect the actor this FX is being attached to be destroyed earlier, and you want the FX to end with it */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use FX from pool"))
	bool m_UseFXPool = true;
	
	/* Whether or not we destroy the effect at the end of the task. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Destroy on End", EditCondition = "m_UseFXPool"))
	bool m_DestroyAtEnd = false;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Destroy Immediately"))
	bool m_DestroyImmediately = false;
	
	/* Context Driven Parameters to set on the Particle instance.*/
	UPROPERTY(EditAnywhere, Instanced, Category = "Particle", meta = (DisplayName = "Instance Parameters"))
	TArray<UAbleParticleEffectParam*> m_Parameters;

	/* if checked, use the configuration ratio for scaling. */
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Adapted Body", AbleBindableProperty))
	bool m_AdaptedBody;

	UPROPERTY()
	FGetAbleBool m_AdaptedBodyDelegate;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use Weapon Location"))
	bool m_bUseWeaponLocation = false;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use Weapon Socket Location"))
	FName m_WeaponSocketLocation;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "双持武器", EditCondition="m_bUseWeaponLocation", EditConditionHides))
	bool bIsDualWeapon = false;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "左手武器", EditCondition="bIsDualWeapon", EditConditionHides))
	bool bUseLeftWeapon = false;

	UPROPERTY(EditAnywhere, Category = "Scale", meta = (ToolTip = "跟随首领缩放"))
	bool bSupportScale = false;

	UPROPERTY(EditAnywhere, Category = "Scale", meta = (ToolTip = "额外首领缩放系数", EditCondition = bSupportScale, EditConditionHides))
	float ScaleCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Falling", meta = (ToolTip = "None:不处理 Hidden:空中不显示", DisplayName = "空中显示处理"))
	ESPPlayFallingType PlayFallingType = ESPPlayFallingType::None;
	
public:
	virtual FString GetModuleName_Implementation() const override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void GetUltimateLaserLocation_Lua(const UAbleAbilityContext* Context, FTransform& Transform) const;

	UFUNCTION(BlueprintImplementableEvent)
	void GetWeaponLocation_Lua(const UAbleAbilityContext* Context, const FString& SocketLocation, FTransform& Transform) const;

	UFUNCTION(BlueprintImplementableEvent)
	USceneComponent* GetWeaponSocketComp_Lua(const UAbleAbilityContext* Context) const;

	UFUNCTION(BlueprintNativeEvent)
	float GetOwnerScale_Lua(const UAbleAbilityContext* Context) const;
protected:
	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Use Ultimate Laser Location"))
	bool m_UseUltimateLaserLocation = false;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Lock Rotation To First Frame"))
	bool m_LockRotationToFirstFrame = false;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Translucency Sort Priority"))
	int32 m_TranslucencySortPriority = 0;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (DisplayName = "Scale With Ability Play Rate"))
	bool m_ScaleWithAbilityPlayRate = false;

public:
	UPROPERTY(EditAnywhere, Category = "SFX", meta = (DisplayName="Bank Name"))
	FString SFXBankName;

	UPROPERTY(EditAnywhere, Category = "SFX", meta = (DisplayName="Event Name"))
	FString SFXEventName;

	UPROPERTY(EditAnywhere, Category = "SFX", meta = (DisplayName="Use Position"))
	bool bSFXUsePosition = false;
};
#undef LOCTEXT_NAMESPACE
