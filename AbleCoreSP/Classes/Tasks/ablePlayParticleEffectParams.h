// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Targeting/ableTargetingBase.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePlayParticleEffectParams.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Base class for all our Particle Effect Parameters. */
UCLASS(Abstract)
class ABLECORESP_API UAbleParticleEffectParam : public UObject
{
	GENERATED_BODY()
public:
	UAbleParticleEffectParam(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer),
	                                                                       m_ParameterName(NAME_None),
	                                                                       m_EnableTickUpdate(false)
	{
	}

	virtual ~UAbleParticleEffectParam() { }

	FORCEINLINE const FName GetParameterName() const { return m_ParameterName; }
	FORCEINLINE bool GetEnableTickUpdate() const { return m_EnableTickUpdate; }

	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	/* Bind any Dynamic Delegates */
	virtual void BindDynamicDelegates(class UAbleAbility* Ability) {};

private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta=(DisplayName="Property Name"))
	FName m_ParameterName;

	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;

	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta=(DisplayName = "Enable Tick Update"))
	bool m_EnableTickUpdate;
};

UCLASS(EditInlineNew, meta=(DisplayName="Context Actor", ShortToolTip="Reference a Context Actor"))
class ABLECORESP_API UAbleParticleEffectParamContextActor : public UAbleParticleEffectParam
{
	GENERATED_BODY()
public:
	UAbleParticleEffectParamContextActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), m_ContextActor(EAbleAbilityTargetType::ATT_Self) { }
	virtual ~UAbleParticleEffectParamContextActor() { }

	FORCEINLINE EAbleAbilityTargetType GetContextActorType() const { return m_ContextActor.GetValue(); }
private:
	UPROPERTY(EditInstanceOnly, Category="Parameter", meta=(DisplayName="Context Actor"))
	TEnumAsByte<EAbleAbilityTargetType> m_ContextActor;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Location", ShortToolTip = "Reference a Location (can use a Context Actor as reference)"))
class UAbleParticleEffectParamLocation: public UAbleParticleEffectParam
{
	GENERATED_BODY()
public:
	UAbleParticleEffectParamLocation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { }
	virtual ~UAbleParticleEffectParamLocation() { } 

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;

	const FAbleAbilityTargetTypeLocation GetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Location", AbleBindableProperty))
	FAbleAbilityTargetTypeLocation m_Location;

	UPROPERTY()
	FGetAbleTargetLocation m_LocationDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Float", ShortToolTip = "Set a float parameter"))
class UAbleParticleEffectParamFloat : public UAbleParticleEffectParam
{
    GENERATED_BODY()
public:
    UAbleParticleEffectParamFloat(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Float(0.0f)
    {
    }
    virtual ~UAbleParticleEffectParamFloat() { }

	float GetFloat(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Float", AbleBindableProperty))
    float m_Float;

	UPROPERTY()
	FGetAbleFloat m_FloatDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Color", ShortToolTip = "Set a color parameter"))
class UAbleParticleEffectParamColor : public UAbleParticleEffectParam
{
    GENERATED_BODY()
public:
    UAbleParticleEffectParamColor(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Color(FLinearColor::White) 
    {
    }
    virtual ~UAbleParticleEffectParamColor() { }

	FLinearColor GetColor(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Color", AbleBindableProperty))
    FLinearColor m_Color;

	UPROPERTY()
	FGetAbleColor m_ColorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Material", ShortToolTip = "Set a material parameter (Only works with Cascade systems, and not Niagara)"))
class UAbleParticleEffectParamMaterial : public UAbleParticleEffectParam
{
    GENERATED_BODY()
public:
    UAbleParticleEffectParamMaterial(const FObjectInitializer& ObjectInitializer) \
        : Super(ObjectInitializer)
        , m_Material(nullptr)
    {
    }
    virtual ~UAbleParticleEffectParamMaterial() { }

	class UMaterialInterface* GetMaterial(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
private:
    UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Material", AbleBindableProperty))
    class UMaterialInterface* m_Material;

	UPROPERTY()
	FGetAbleMaterial m_MaterialDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Vector", ShortToolTip = "Set a Vector parameter"))
class UAbleParticleEffectParamVector : public UAbleParticleEffectParam
{
	GENERATED_BODY()
public:
	UAbleParticleEffectParamVector(const FObjectInitializer& ObjectInitializer) \
		: Super(ObjectInitializer)
		, m_Vector(FVector::ZeroVector)
	{
	}
	virtual ~UAbleParticleEffectParamVector() { }

	FVector GetVector(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Vector", AbleBindableProperty))
	FVector m_Vector;

	UPROPERTY()
	FGetAbleVector m_VectorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Bool", ShortToolTip = "Set a Bool parameter"))
class UAbleParticleEffectParamBool : public UAbleParticleEffectParam
{
	GENERATED_BODY()
public:
	UAbleParticleEffectParamBool(const FObjectInitializer& ObjectInitializer) \
		: Super(ObjectInitializer)
		, m_Bool(false)
	{
	}
	virtual ~UAbleParticleEffectParamBool() { }

	bool GetBool(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) override;
private:
	UPROPERTY(EditInstanceOnly, Category = "Parameter", meta = (DisplayName = "Bool", AbleBindableProperty))
	bool m_Bool;

	UPROPERTY()
	FGetAbleBool m_BoolDelegate;
};


#undef LOCTEXT_NAMESPACE