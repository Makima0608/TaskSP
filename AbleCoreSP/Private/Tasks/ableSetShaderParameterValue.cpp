// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSetShaderParameterValue.h"

#include "ableAbility.h"
#include "AbleCoreSPPrivate.h"

FName UAbleSetParameterValue::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Shader_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

float UAbleSetScalarParameterValue::GetScalar(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Scalar);
}

void UAbleSetScalarParameterValue::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Scalar, "Scalar");
}

const FLinearColor UAbleSetVectorParameterValue::GetColor(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Color);
}

void UAbleSetVectorParameterValue::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Color, "Color");
}

UTexture* UAbleSetTextureParameterValue::GetTexture(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Texture);
}

void UAbleSetTextureParameterValue::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Texture, "Texture");
}

#if WITH_EDITOR
bool UAbleSetParameterValue::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}
#endif