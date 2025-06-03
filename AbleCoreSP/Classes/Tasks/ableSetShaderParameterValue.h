// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Engine/Texture.h"
#include "UObject/ObjectMacros.h"
#include "IAbleAbilityTask.h"

#include "ableSetShaderParameterValue.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

/* Base class for all our supported parameter types. */
UCLASS(Abstract)
class ABLECORESP_API UAbleSetParameterValue : public UObject
{
	GENERATED_BODY()
public:
	enum Type
	{
		None = 0,
		Scalar,
		Vector,
		Texture,

		TotalTypes
	};

	UAbleSetParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Type(None)
	{ }
	virtual ~UAbleSetParameterValue() { }

	FORCEINLINE Type GetType() const { return m_Type; }
	virtual const FString ToString() const { return FString(TEXT("Invalid")); }
	
	/* Get our Dynamic Identifier. */
	const FString& GetDynamicPropertyIdentifier() const { return m_DynamicPropertyIdentifer; }

	/* Get Dynamic Delegate Name. */
	FName GetDynamicDelegateName(const FString& PropertyName) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability) {};

#if WITH_EDITOR
	/* Fix our flags. */
	bool FixUpObjectFlags();
#endif

protected:
	Type m_Type;

private:
	/* The Identifier applied to any Dynamic Property methods for this task. This can be used to differentiate multiple tasks of the same type from each other within the same Ability. */
	UPROPERTY(EditInstanceOnly, Category = "Dynamic Properties", meta = (DisplayName = "Identifier"))
	FString m_DynamicPropertyIdentifer;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Scalar Parameter"))
class ABLECORESP_API UAbleSetScalarParameterValue : public UAbleSetParameterValue
{
	GENERATED_BODY()
public:
	UAbleSetScalarParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Scalar(0.0f)
	{
		m_Type = UAbleSetParameterValue::Scalar;
	}
	virtual ~UAbleSetScalarParameterValue() { }

	/* Sets the Scalar value. */
	FORCEINLINE void SetScalar(float InScalar) { m_Scalar = InScalar; }
	
	/* Returns the Scalar value. */
	float GetScalar(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual const FString ToString() const { return FString::Printf(TEXT("%4.3f"), m_Scalar); }

	virtual void BindDynamicDelegates(class UAbleAbility* Ability);
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Scalar", AbleBindableProperty))
	float m_Scalar;

	UPROPERTY()
	FGetAbleFloat m_ScalarDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Vector Parameter"))
class ABLECORESP_API UAbleSetVectorParameterValue : public UAbleSetParameterValue
{
	GENERATED_BODY()
public:
	UAbleSetVectorParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Color()
	{
		m_Type = UAbleSetParameterValue::Vector;
	}
	virtual ~UAbleSetVectorParameterValue() { }

	/* Sets the Color value. */
	FORCEINLINE void SetColor(const FLinearColor& InVector) { m_Color = InVector; }
	
	/* Returns the Color value. */
	const FLinearColor GetColor(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability);

	virtual const FString ToString() const { return m_Color.ToString(); }
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Color", AbleBindableProperty))
	FLinearColor m_Color;

	UPROPERTY()
	FGetAbleColor m_ColorDelegate;
};

UCLASS(EditInlineNew, meta = (DisplayName = "Texture Parameter"))
class ABLECORESP_API UAbleSetTextureParameterValue : public UAbleSetParameterValue
{
	GENERATED_BODY()
public:
	UAbleSetTextureParameterValue(const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer),
		m_Texture(nullptr)
	{
		m_Type = UAbleSetParameterValue::Texture;
	}
	virtual ~UAbleSetTextureParameterValue() { }

	/* Sets the Texture value. */
	FORCEINLINE void SetTexture(/*const*/ UTexture* InTexture) { m_Texture = InTexture; }
	
	/* Returns the Texture value. */
	UTexture* GetTexture(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const;

	virtual void BindDynamicDelegates(class UAbleAbility* Ability);

	virtual const FString ToString() const { return m_Texture ? m_Texture->GetName() : Super::ToString(); }
private:
	UPROPERTY(EditInstanceOnly, Category = "Value", meta = (DisplayName = "Texture", AbleBindableProperty))
	/*const*/ UTexture* m_Texture; // Fix this when the API returns const

	UPROPERTY()
	FGetAbleTexture m_TextureDelegate;
};

#undef LOCTEXT_NAMESPACE