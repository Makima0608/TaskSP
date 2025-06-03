// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSetShaderParameterTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tasks/ableSetShaderParameterValue.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleSetShaderParameterTaskScratchPad::UAbleSetShaderParameterTaskScratchPad()
{

}

UAbleSetShaderParameterTaskScratchPad::~UAbleSetShaderParameterTaskScratchPad()
{

}

UAbleSetShaderParameterTask::UAbleSetShaderParameterTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ParameterName(NAME_None),
	m_Value(nullptr),
	m_RestoreValueOnEnd(false)
{

}

UAbleSetShaderParameterTask::~UAbleSetShaderParameterTask()
{

}

FString UAbleSetShaderParameterTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleSetShaderParameterTask");
}

void UAbleSetShaderParameterTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleSetShaderParameterTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	if (!m_Value)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("No Value set for SetShaderParameter in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
		return;
	}

	// We need to convert our Actors to primitive components.
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	TArray<TWeakObjectPtr<UPrimitiveComponent>> PrimitiveComponents;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		TInlineComponentArray<UPrimitiveComponent*> TargetComponents(Target.Get());
		PrimitiveComponents.Append(TargetComponents);
	}

	UAbleSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAbleSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	ScratchPad->DynamicMaterials.Empty();
	ScratchPad->PreviousValues.Empty();

	ScratchPad->BlendIn = m_BlendIn;
	ScratchPad->BlendIn.Reset();

	ScratchPad->BlendOut = m_BlendOut;
	ScratchPad->BlendOut.Reset();

	UAbleSetParameterValue* CachedValue = nullptr;
	for (TWeakObjectPtr<UPrimitiveComponent>& PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent.IsValid())
		{
			for (int32 i = 0; i < PrimitiveComponent->GetNumMaterials(); ++i)
			{
				UMaterialInterface* MaterialInterface = PrimitiveComponent->GetMaterial(i);
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);

				if (!MaterialInterface)
				{
					continue;
				}

				// If our material currently isn't dynamic, but we have the parameter we're looking for - instantiate a dynamic version.
				if (!DynamicMaterial && CheckMaterialHasParameter(MaterialInterface))
				{
					DynamicMaterial = PrimitiveComponent->CreateDynamicMaterialInstance(i, MaterialInterface);
				}

				if (DynamicMaterial)
				{
					CachedValue = CacheShaderValue(ScratchPad, DynamicMaterial);
					if (CachedValue)
					{
						ScratchPad->DynamicMaterials.Add(DynamicMaterial);
						ScratchPad->PreviousValues.Add(CachedValue);

						if (ScratchPad->BlendIn.IsComplete())
						{
							// If there isn't any blend. Just set it here since we won't be ticking.
							InternalSetShaderValue(Context, DynamicMaterial, m_Value, CachedValue, ScratchPad->BlendIn.GetAlpha());
						}
					}
				}
			}
		}
	}

}

void UAbleSetShaderParameterTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleSetShaderParameterTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

	UAbleSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAbleSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

	if (!ScratchPad->BlendIn.IsComplete())
	{
		ScratchPad->BlendIn.Update(deltaTime);

		verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
		for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
		{
			InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], m_Value, ScratchPad->PreviousValues[i], ScratchPad->BlendIn.GetBlendedValue());
		}
	}
	else if (m_RestoreValueOnEnd && !ScratchPad->BlendOut.IsComplete())
	{
		// If we're within range to start blending out, go ahead and start that process.
		if (GetEndTime() - Context->GetCurrentTime() < ScratchPad->BlendOut.GetBlendTime())
		{
			ScratchPad->BlendOut.Update(deltaTime);
			
			verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
			for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
			{
				InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], ScratchPad->PreviousValues[i], m_Value, ScratchPad->BlendOut.GetBlendedValue());
			}
		}
	}
}

void UAbleSetShaderParameterTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);
	OnTaskEndBP(Context.Get(), Result);
}

void UAbleSetShaderParameterTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (m_RestoreValueOnEnd && Context)
	{
		UAbleSetShaderParameterTaskScratchPad* ScratchPad = Cast<UAbleSetShaderParameterTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		verify(ScratchPad->DynamicMaterials.Num() == ScratchPad->PreviousValues.Num());
		for (int32 i = 0; i < ScratchPad->DynamicMaterials.Num(); ++i)
		{
			InternalSetShaderValue(Context, ScratchPad->DynamicMaterials[i], ScratchPad->PreviousValues[i], m_Value, 1.0);
		}
	}

}

UAbleAbilityTaskScratchPad* UAbleSetShaderParameterTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleSetShaderParameterTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleSetShaderParameterTaskScratchPad>(Context.Get());
}

TStatId UAbleSetShaderParameterTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleSetShaderParameterTask, STATGROUP_Able);
}

UAbleSetParameterValue* UAbleSetShaderParameterTask::CacheShaderValue(UAbleSetShaderParameterTaskScratchPad* ScratchPad, UMaterialInstanceDynamic* DynMaterial) const
{
	check(DynMaterial);
	UAbleSetParameterValue* Value = nullptr;
	if (m_Value->GetType() == UAbleSetParameterValue::Scalar)
	{
		float Scalar = 0.0f;
		if (DynMaterial->GetScalarParameterValue(m_ParameterName, Scalar))
		{
			UAbleSetScalarParameterValue* ScalarValue = NewObject<UAbleSetScalarParameterValue>(ScratchPad);
			ScalarValue->SetScalar(Scalar);
			Value = ScalarValue;
		}
	}
	else if (m_Value->GetType() == UAbleSetParameterValue::Vector)
	{
		FLinearColor VectorParam;
		if (DynMaterial->GetVectorParameterValue(m_ParameterName, VectorParam))
		{
			UAbleSetVectorParameterValue* VectorValue = NewObject<UAbleSetVectorParameterValue>(ScratchPad);
			VectorValue->SetColor(VectorParam);
			Value = VectorValue;
		}
	}
	else if (m_Value->GetType() == UAbleSetParameterValue::Texture)
	{
		UTexture* Texture = nullptr;
		if (DynMaterial->GetTextureParameterValue(m_ParameterName, Texture))
		{
			UAbleSetTextureParameterValue* TextureValue = NewObject<UAbleSetTextureParameterValue>(ScratchPad);
			TextureValue->SetTexture(Texture);
			Value = TextureValue;
		}
	}
	else
	{
		checkNoEntry();
	}

	return Value;
}

void UAbleSetShaderParameterTask::InternalSetShaderValue(const TWeakObjectPtr<const UAbleAbilityContext>& Context, UMaterialInstanceDynamic* DynMaterial, UAbleSetParameterValue* Value, UAbleSetParameterValue* PreviousValue, float BlendAlpha) const
{
	check(DynMaterial);
	check(Value);
	check(PreviousValue);

#if !(UE_BUILD_SHIPPING)
	if (IsVerbose())
	{
		PrintVerbose(Context, FString::Printf(TEXT("Setting material parameter %s on Material %s to %s with a blend of %1.4f."), *m_ParameterName.ToString(), *DynMaterial->GetName(), *Value->ToString(), BlendAlpha));
	}
#endif

	if (UAbleSetScalarParameterValue* ScalarValue = Cast<UAbleSetScalarParameterValue>(Value))
	{
		UAbleSetScalarParameterValue* PreviousScalarValue = CastChecked<UAbleSetScalarParameterValue>(PreviousValue);
		float InterpolatedValue = FMath::Lerp(PreviousScalarValue->GetScalar(Context), ScalarValue->GetScalar(Context), BlendAlpha);
		DynMaterial->SetScalarParameterValue(m_ParameterName, InterpolatedValue);
	}
	else if (UAbleSetVectorParameterValue* VectorValue = Cast<UAbleSetVectorParameterValue>(Value))
	{
		UAbleSetVectorParameterValue* PreviousVectorValue = CastChecked<UAbleSetVectorParameterValue>(PreviousValue);
		FLinearColor InterpolatedValue = FMath::Lerp(PreviousVectorValue->GetColor(Context), VectorValue->GetColor(Context), BlendAlpha);
		DynMaterial->SetVectorParameterValue(m_ParameterName, InterpolatedValue);
	}
	else if (UAbleSetTextureParameterValue* TextureValue = Cast<UAbleSetTextureParameterValue>(Value))
	{
		// No Lerping allowed.
		DynMaterial->SetTextureParameterValue(m_ParameterName, TextureValue->GetTexture(Context));
	}
	else
	{
		checkNoEntry();
	}
}


bool UAbleSetShaderParameterTask::CheckMaterialHasParameter(UMaterialInterface* Material) const
{
	if (UAbleSetScalarParameterValue* ScalarValue = Cast<UAbleSetScalarParameterValue>(m_Value))
	{
		float TempScalar;
		return Material->GetScalarParameterValue(m_ParameterName, TempScalar);
	}
	else if (UAbleSetVectorParameterValue* VectorValue = Cast<UAbleSetVectorParameterValue>(m_Value))
	{
		FLinearColor TempVector;
		return Material->GetVectorParameterValue(m_ParameterName, TempVector);
	}
	else if (UAbleSetTextureParameterValue* TextureValue = Cast<UAbleSetTextureParameterValue>(m_Value))
	{
		UTexture* TempTexture;
		return Material->GetTextureParameterValue(m_ParameterName, TempTexture);
	}

	UE_LOG(LogAbleSP, Warning, TEXT("Could not find Parameter [%s] in Material [%s]"), *m_ParameterName.ToString(), *Material->GetName());
	return false;
}

void UAbleSetShaderParameterTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	if (m_Value)
	{
		m_Value->BindDynamicDelegates(Ability);
	}
}

#if WITH_EDITOR

FText UAbleSetShaderParameterTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlaySetShaderParameterTaskFormat", "{0}: {1}");
	FString ParameterName = TEXT("<none>");
	if (!m_ParameterName.IsNone())
	{
		ParameterName = m_ParameterName.ToString();
	}
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ParameterName));
}

void UAbleSetShaderParameterTask::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	float MinimumRequiredTimeForBlending = m_BlendIn.GetBlendTime();

	if (m_RestoreValueOnEnd)
	{
		MinimumRequiredTimeForBlending += m_BlendOut.GetBlendTime();
	}

	m_EndTime = FMath::Max(m_EndTime, MinimumRequiredTimeForBlending);
}

bool UAbleSetShaderParameterTask::FixUpObjectFlags()
{
	bool modified = Super::FixUpObjectFlags();

	if (m_Value)
	{
		modified |= m_Value->FixUpObjectFlags();
	}

	return modified;
}

#endif

bool UAbleSetShaderParameterTask::IsSingleFrameBP_Implementation() const { return m_BlendIn.IsComplete() && !m_RestoreValueOnEnd; } // Only tick if we have a blend to do, or we have to restore our value at the end.

EAbleAbilityTaskRealm UAbleSetShaderParameterTask::GetTaskRealmBP_Implementation() const { return EAbleAbilityTaskRealm::ATR_Client; }

#undef LOCTEXT_NAMESPACE