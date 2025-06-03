// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlayParticleEffectParams.h"

#include "ableAbility.h"
#include "AbleCoreSPPrivate.h"

FName UAbleParticleEffectParam::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Particle_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

float UAbleParticleEffectParamFloat::GetFloat(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Float);
}

void UAbleParticleEffectParamFloat::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Float, "Float");
}

FLinearColor UAbleParticleEffectParamColor::GetColor(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Color);
}

void UAbleParticleEffectParamColor::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Color, "Color");
}

UMaterialInterface* UAbleParticleEffectParamMaterial::GetMaterial(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Material);
}

void UAbleParticleEffectParamMaterial::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Material, "Material");
}

FVector UAbleParticleEffectParamVector::GetVector(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Vector);
}

void UAbleParticleEffectParamVector::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Vector, "Vector");
}

bool UAbleParticleEffectParamBool::GetBool(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Bool);
}

void UAbleParticleEffectParamBool::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Bool, "Bool");
}

void UAbleParticleEffectParamLocation::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
}

const FAbleAbilityTargetTypeLocation UAbleParticleEffectParamLocation::GetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Location);
}

