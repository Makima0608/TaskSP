// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlayParticleEffectTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Runtime/Engine/Public/ParticleEmitterInstances.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePlayParticleEffectTaskScratchPad::UAblePlayParticleEffectTaskScratchPad()
{

}

UAblePlayParticleEffectTaskScratchPad::~UAblePlayParticleEffectTaskScratchPad()
{

}

UAblePlayParticleEffectTask::UAblePlayParticleEffectTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  m_EffectTemplate(nullptr),
	  m_UseCustomEffect(false),
	  m_UseInstigatorElementType(false),
	  m_CustomEffectId(0),
	  m_AttachToSocket(false),
	  m_bTickChangeTransform(false),
	  m_Scale(1.0f),
	  m_DynamicScaleSize(0.0f),
	  m_DestroyAtEnd(false),
	  m_AdaptedBody(false)
{
}

UAblePlayParticleEffectTask::~UAblePlayParticleEffectTask()
{

}

void UAblePlayParticleEffectTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (PlayFallingType == ESPPlayFallingType::Hidden)
	{
		AActor* TargetActor = GetSingleActorFromTargetType(Context, ATT_Self);
		if (TargetActor != nullptr)
		{
			ACharacter* Char = Cast<ACharacter>(TargetActor);
			if (Char != nullptr)
			{
				if (Char->GetMovementComponent() && Char->GetMovementComponent()->IsFalling())
				{
					UE_LOG(LogAbleSP, Warning, TEXT("No need to spawn Particle System set for PlayParticleEffectTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
					return;
				}
			}
		}
	}
    Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePlayParticleEffectTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	UParticleSystem* EffectTemplate = m_EffectTemplate.LoadSynchronous();
    if (!EffectTemplate && !m_UseCustomEffect)
    {
        UE_LOG(LogAbleSP, Warning, TEXT("No Particle System set for PlayParticleEffectTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
        return;
    }

	// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskStart [%s] [%d] [%s]"), *Context->GetAbility()->GetAbilityName(), Context->GetActiveSegmentIndex(), *GetName());
	
    FTransform OffsetTransform(FTransform::Identity);
    FTransform SpawnTransform = OffsetTransform;

	FAbleAbilityTargetTypeLocation Location = m_Location;

    if (m_UseCustomEffect)
    {
        AActor* ElementTypeOwner = m_UseInstigatorElementType ? Context->GetInstigator() : Context->GetOwner();
        UParticleSystem* CustomEffectTemplate = GetCustomEffectTemplateByElementType_Lua(ElementTypeOwner, m_CustomEffectId);
        if (CustomEffectTemplate)
        {
            EffectTemplate = CustomEffectTemplate;
        }
    }
    if (!EffectTemplate)
    {
        UE_LOG(LogAbleSP, Warning, TEXT("No Particle System set for PlayParticleEffectTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
        return;
    }
	// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskStart Effect Name = [%s]"), *EffectTemplate->GetPathName());

	float Scale = m_Scale;
    if (bSupportScale)
    {
    	Scale = Scale * GetOwnerScale_Lua(Context) * ScaleCoefficient;
    }
	float DynamicScale = m_DynamicScaleSize;
	float AdaptedBody = m_AdaptedBody;

    TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect = nullptr;

    UAblePlayParticleEffectTaskScratchPad* ScratchPad = nullptr;
    if (m_DestroyAtEnd || m_bTickChangeTransform)
    {
        ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
    	if (ScratchPad)
    	{
    		ScratchPad->SpawnedEffects.Empty();
    		ScratchPad->SpawnedEffectsMap.Empty();
    	}
    }

    TArray<TWeakObjectPtr<AActor>> TargetArray;
    GetActorsForTask(Context, TargetArray);

    for (int i = 0; i < TargetArray.Num(); ++i)
    {
        TWeakObjectPtr<AActor> Target = TargetArray[i];
        SpawnTransform.SetIdentity();

        if (Location.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
        {
            Location.GetTargetTransform(*Context, i, SpawnTransform);
        }
        else
        {
            Location.GetTransform(*Context, SpawnTransform);
        }

        if (m_UseUltimateLaserLocation)
        {
            GetUltimateLaserLocation_Lua(Context, SpawnTransform);
        }

        if (m_bUseWeaponLocation)
        {
            FString socket = m_WeaponSocketLocation.GetPlainNameString();
            GetWeaponLocation_Lua(Context, socket, SpawnTransform);
        }

        FVector EmitterScale(Scale);

        if (DynamicScale > 0.0f)
        {
            float targetRadius = Target->GetSimpleCollisionRadius();
            float scaleFactor = targetRadius / DynamicScale;
            EmitterScale = FVector(scaleFactor);
        }

        if (AdaptedBody)
        {
	        float targetShapeScale = GetShapeScale_Lua(Target.Get());
        	float scaleFactor = targetShapeScale * Scale;
        	EmitterScale = FVector(scaleFactor);
        }

        SpawnTransform.SetScale3D(EmitterScale);
        
#if !(UE_BUILD_SHIPPING)
        if (IsVerbose())
        {
            PrintVerbose(Context, FString::Printf(TEXT("Spawning Emitter %s with Transform %s, Scale [%4.2f] Dynamic Scale [%4.2f] Final Scale [%4.2f]"),
                EffectTemplate ? *EffectTemplate->GetName() : TEXT(""), *SpawnTransform.ToString(), Scale, DynamicScale, EmitterScale.X));
        }
#endif
        
        //Set Pool Method
        EPSCPoolMethod PoolMethod = EPSCPoolMethod::AutoRelease;

    	if (!m_UseFXPool)
    	{
    		PoolMethod = EPSCPoolMethod::None;
    	}
        else if (m_DestroyAtEnd)
        {
            PoolMethod = EPSCPoolMethod::ManualRelease;
        }
    	
        if (m_AttachToSocket)
        {
			USceneComponent* AttachComponent = Target->FindComponentByClass<USkeletalMeshComponent>();
        	if (!AttachComponent) AttachComponent = Target->FindComponentByClass<USceneComponent>();
        	
            if (EffectTemplate)
            {
            	FRotator AttachRotation = Location.GetRotation();
            	// world transform
            	// transform to socket bone local transform
            	FTransform SocketTransform = OffsetTransform;
            	if (Location.GetSocketTransform(*Context, SocketTransform))
            	{
            		FQuat SocketLocationQuat = SocketTransform.InverseTransformRotation(SpawnTransform.GetRotation());
            		AttachRotation = SocketLocationQuat.Rotator();
            	}

                bool bAttached = false;
                if (m_bUseWeaponLocation)
                {
                    USceneComponent* AttachWeaponComponent = GetWeaponSocketComp_Lua(Context);
                    if (AttachWeaponComponent != nullptr)
                    {
                        bAttached = true;
                        AttachComponent = AttachWeaponComponent;
                        SpawnedEffect = UGameplayStatics::SpawnEmitterAttached(EffectTemplate, AttachWeaponComponent, m_WeaponSocketLocation, FVector(0,0,0), FRotator(0,0,0), EAttachLocation::SnapToTarget, true, PoolMethod);
                    }
                }

                if (!bAttached)
                {
                    if (Location.NeedSpawnTraceToGround())
                    {
                        // transform world location to bone local
                        SocketTransform = OffsetTransform;
                        if (Location.GetSocketTransform(*Context, SocketTransform))
                        {
                            const FVector LocalPosition = SocketTransform.InverseTransformPosition(SpawnTransform.GetLocation());
                            SpawnedEffect = UGameplayStatics::SpawnEmitterAttached(EffectTemplate, AttachComponent, Location.GetSocketName(), LocalPosition, AttachRotation, SpawnTransform.GetScale3D(), EAttachLocation::SnapToTarget, true, PoolMethod);
                        }
                        else
                        {
                            SpawnedEffect = UGameplayStatics::SpawnEmitterAttached(EffectTemplate, AttachComponent, Location.GetSocketName(), Location.GetOffset(), AttachRotation, SpawnTransform.GetScale3D(), EAttachLocation::SnapToTarget, true, PoolMethod);
                        }
                    }
                    else
                    {
                        SpawnedEffect = UGameplayStatics::SpawnEmitterAttached(EffectTemplate, AttachComponent, Location.GetSocketName(), Location.GetOffset(), AttachRotation, SpawnTransform.GetScale3D(), EAttachLocation::SnapToTarget, true, PoolMethod);
                    	if (m_AbsoluteSetScale)
						{
							SpawnedEffect->SetRelativeScale3D_Direct(SpawnTransform.GetScale3D());
						}
                    }
                	if (SpawnedEffect.IsValid())
                	{
                		if (AttachComponent->bHiddenInGame)
                		{
                			SpawnedEffect->SetHiddenInGame(AttachComponent->bHiddenInGame);
                		}
                		else if (!AttachComponent->GetVisibleFlag())
                		{
                			SpawnedEffect->SetVisibility(AttachComponent->GetVisibleFlag(), true);
                		}	
                	}
                }	
            }
        }
        else
        {
            if (EffectTemplate)
            {
            	SpawnedEffect = UGameplayStatics::SpawnEmitterAtLocation(Target->GetWorld(), EffectTemplate, SpawnTransform, true, PoolMethod);
            }
        }
    	// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskStart Spawned Emitter %s"), *SpawnedEffect->GetName());

    	if (SpawnedEffect.IsValid())
    	{
    		SpawnedEffect->Activate(true);
    		// Apply translucency sort priority
    		SpawnedEffect->TranslucencySortPriority = m_TranslucencySortPriority;
    	}

        if (SpawnedEffect.IsValid() && ScratchPad)
        {
        	float PlayRate = m_ScaleWithAbilityPlayRate ? Context->GetAbility()->GetPlayRate(Context) : 1.0f;
        	SpawnedEffect->CustomTimeDilation = PlayRate;
        	if (m_DestroyAtEnd)
        	{
        		ScratchPad->SpawnedEffects.Add(SpawnedEffect);
        	}
        	if (m_bTickChangeTransform)
        	{
        		ScratchPad->SpawnedEffectsMap.Add(i, SpawnedEffect);
        	}
        }

        // Set our Parameters.
        for (UAbleParticleEffectParam* Parameter : m_Parameters)
        {
			UFXSystemComponent* SpawnedEffectComponent = Cast<UFXSystemComponent>(SpawnedEffect.Get());
			if (!SpawnedEffectComponent)
			{
				break;
			}

        	if (!Parameter) continue;

            if (Parameter->IsA<UAbleParticleEffectParamContextActor>())
            {
                UAbleParticleEffectParamContextActor* ContextActorParam = Cast<UAbleParticleEffectParamContextActor>(Parameter);
                if (ContextActorParam->GetContextActorType() == EAbleAbilityTargetType::ATT_TargetActor)
                {
					SpawnedEffectComponent->SetActorParameter(Parameter->GetParameterName(), Target.Get());
                }
                else if (AActor* FoundActor = GetSingleActorFromTargetType(Context, ContextActorParam->GetContextActorType()))
                {
					SpawnedEffectComponent->SetActorParameter(Parameter->GetParameterName(), FoundActor);
                }
            }
			else
			{
				ApplyParticleEffectParameter(Parameter, Context, SpawnedEffectComponent, SpawnedEffect);
			}
        }
    }
	FVector SFXPosition = FVector::ZeroVector;
	if(SpawnedEffect != nullptr)
	{
		SFXPosition = SpawnedEffect.Get()->GetComponentLocation();
        // UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskStart Spawned Emitter %s Location %s"), *SpawnedEffect->GetName(), *SFXPosition.ToCompactString());
	}
	
	OnPlaySFX_Lua(SFXBankName, SFXEventName, Context->GetOwner(), bSFXUsePosition, SFXPosition);
}

void UAblePlayParticleEffectTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAblePlayParticleEffectTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	if (Context)
	{
		UAblePlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!IsValid(ScratchPad)) return;

		for (TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect : ScratchPad->SpawnedEffects)
		{
			if (SpawnedEffect.IsValid())
			{
				if (UFXSystemComponent* SpawnedEffectSystem = Cast<UFXSystemComponent>(SpawnedEffect.Get()))
				{
					for (const UAbleParticleEffectParam* param : m_Parameters)
					{
						if (param->GetEnableTickUpdate())
						{
							ApplyParticleEffectParameter(param, Context, SpawnedEffectSystem, SpawnedEffect);
						}
					}
				}
			}
		}

        if (m_UseUltimateLaserLocation)
        {
            for (auto SpawnedEffect : ScratchPad->SpawnedEffects)
            {
                FTransform Transform;
                GetUltimateLaserLocation_Lua(Context, Transform);
                if (SpawnedEffect.IsValid())
                {
                    if (m_LockRotationToFirstFrame)
                    {
                        SpawnedEffect->SetWorldLocation(Transform.GetLocation());
                    }
                    else
                    {
                        SpawnedEffect->SetWorldLocationAndRotation(Transform.GetLocation(), Transform.GetRotation());
                    }
                }
            }
        }
		else if (m_bTickChangeTransform)
		{
			OnTickChangeTransform(Context, deltaTime);
		}
	}
}

void UAblePlayParticleEffectTask::ResetPcsTransform(const UAbleAbilityContext* Context) const
{
	UAblePlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!IsValid(ScratchPad)) return;
	FTransform OffsetTransform(FTransform::Identity);
	FTransform SpawnTransform = OffsetTransform;
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);
	
	FAbleAbilityTargetTypeLocation Location = m_Location;
	float Scale = m_Scale;
	if (bSupportScale)
	{
		Scale = Scale * GetOwnerScale_Lua(Context) * ScaleCoefficient;
	}
	float DynamicScale = m_DynamicScaleSize;
	float AdaptedBody = m_AdaptedBody;
	TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect = nullptr;
	for (auto Pair : ScratchPad->SpawnedEffectsMap)
	{
		const int TargetIndex = Pair.Key;
		SpawnedEffect = Pair.Value;
		if (!SpawnedEffect.IsValid())
        {
			continue;
        }
		TWeakObjectPtr<AActor> Target = TargetArray[TargetIndex];
		SpawnTransform.SetIdentity();
		if (Location.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
		{
			Location.GetTargetTransform(*Context, TargetIndex, SpawnTransform);
		}
		else
		{
			Location.GetTransform(*Context, SpawnTransform);
		}
		if (m_UseUltimateLaserLocation)
		{
			GetUltimateLaserLocation_Lua(Context, SpawnTransform);
		}

		if (m_bUseWeaponLocation)
		{
			FString socket = m_WeaponSocketLocation.GetPlainNameString();
			GetWeaponLocation_Lua(Context, socket, SpawnTransform);
		}

		FVector EmitterScale(Scale);

		if (DynamicScale > 0.0f)
		{
			float targetRadius = Target->GetSimpleCollisionRadius();
			float scaleFactor = targetRadius / DynamicScale;
			EmitterScale = FVector(scaleFactor);
		}

		if (AdaptedBody)
		{
			float targetShapeScale = GetShapeScale_Lua(Target.Get());
			float scaleFactor = targetShapeScale * Scale;
			EmitterScale = FVector(scaleFactor);
		}
		SpawnTransform.SetScale3D(EmitterScale);
		if (m_AttachToSocket)
        {
			if (Location.NeedSpawnTraceToGround())
			{
				SpawnedEffect->SetWorldLocation(SpawnTransform.GetLocation());
			}
			if (!m_LockRotationToFirstFrame)
			{
				FRotator AttachRotation = Location.GetRotation();
				// world transform
				// transform to socket bone local transform
				FTransform SocketTransform = OffsetTransform;
				if (Location.GetSocketTransform(*Context, SocketTransform))
				{
					FQuat SocketLocationQuat = SocketTransform.InverseTransformRotation(SpawnTransform.GetRotation());
					AttachRotation = SocketLocationQuat.Rotator();
				}
				SpawnedEffect->SetRelativeRotation(AttachRotation);	
			}
			SpawnedEffect->SetRelativeScale3D(SpawnTransform.GetScale3D());
        }
        else
        {
        	if (!m_LockRotationToFirstFrame)
        	{
        		SpawnedEffect->SetWorldTransform(SpawnTransform);
        	}
            else
            {
            	SpawnedEffect->SetWorldLocation(SpawnTransform.GetLocation());
            }
        	SpawnedEffect->SetWorldScale3D(SpawnTransform.GetScale3D());
        }
	}
}

void UAblePlayParticleEffectTask::OnAbilityPlayRateChanged(const UAbleAbilityContext* Context, float NewPlayRate)
{
	Super::OnAbilityPlayRateChanged(Context, NewPlayRate);

	if (!Context) return;
	if (!m_ScaleWithAbilityPlayRate) return;
	
	UAblePlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!IsValid(ScratchPad)) return;
	
	for (TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect : ScratchPad->SpawnedEffects)
	{
		if (SpawnedEffect.IsValid())
		{
			SpawnedEffect->CustomTimeDilation = NewPlayRate;
		}
	}
}

void UAblePlayParticleEffectTask::OnTickChangeTransform_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	UAblePlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!IsValid(ScratchPad)) return;
	ResetPcsTransform(Context);
}

void UAblePlayParticleEffectTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const
{
    Super::OnTaskEnd(Context, Result);
	OnTaskEndBP(Context.Get(), Result);
}

void UAblePlayParticleEffectTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult Result) const
{
	// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 1 [%d] [%s]"), Result, *GetName());
	
	if (m_DestroyAtEnd && IsValid(Context))
    {
    	UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 2 [%s] [%d] [%s]"), *Context->GetAbility()->GetAbilityName(), Context->GetActiveSegmentIndex(), *GetName());
        UAblePlayParticleEffectTaskScratchPad* ScratchPad = Cast<UAblePlayParticleEffectTaskScratchPad>(Context->GetScratchPadForTask(this));
        if (!IsValid(ScratchPad)) return;

    	// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 3 [%s], SpawnedEffects Length = [%d]"), *GetName(), ScratchPad->SpawnedEffects.Num());
    	
        for (TWeakObjectPtr<UParticleSystemComponent> SpawnedEffect : ScratchPad->SpawnedEffects)
        {
            if (SpawnedEffect.IsValid())
            {
#if !(UE_BUILD_SHIPPING)
                if (IsVerbose())
                {
                    PrintVerbose(Context, FString::Printf(TEXT("Destroying Emitter %s"), *SpawnedEffect->GetName()));
					if (SpawnedEffect.IsValid() && IsValid(SpawnedEffect->Template))
					{
						UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 6 EffectTemplate name = [%s]"), *SpawnedEffect->Template->GetPathName());
					}
                }
#endif
    			// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 4 Destroying Emitter [%s]"), *SpawnedEffect->GetName());
                SpawnedEffect->bAutoDestroy = true;
                /*SpawnedEffect->DeactivateSystem();*/
            	SpawnedEffect->DeactivaateNextTick();
            	if (SpawnedEffect.IsValid() && m_DestroyImmediately)
            	{
            		// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd 5 Destroying Emitter [%s]"), *SpawnedEffect->GetName());
            		SpawnedEffect->SetHiddenInGame(true, true);
            		SpawnedEffect->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            		SpawnedEffect->KillParticlesForced();
            	}

				if (SpawnedEffect.IsValid())
				{
            		SpawnedEffect->ReleaseToPool();
					/*if (SpawnedEffect.IsValid() && SpawnedEffect->Template)
					{
						UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd Release To Pool Effect Name = [%s]"), *SpawnedEffect->Template->GetPathName());
					}*/
				}
            	else
            	{
            		// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd SpawnedEffect Not Valid[1]"));
            	}
            }
        	else
        	{
        		// UE_LOG(LogAbleSP, Log, TEXT("UAblePlayParticleEffectTask::OnTaskEnd SpawnedEffect Not Valid[2]"));
        	}
        }
		ScratchPad->SpawnedEffects.Empty();
		ScratchPad->SpawnedEffectsMap.Empty();
    }
}

bool UAblePlayParticleEffectTask::NeedsTick() const
{
    if (m_UseUltimateLaserLocation || m_bTickChangeTransform) return true;

	return m_Parameters.Num() != 0 && m_Parameters.FindByPredicate([](const UAbleParticleEffectParam* RHS){ return RHS && RHS->GetEnableTickUpdate(); }) != nullptr;
}

UAbleAbilityTaskScratchPad* UAblePlayParticleEffectTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_DestroyAtEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAblePlayParticleEffectTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAblePlayParticleEffectTaskScratchPad>(Context.Get());
	}

    return nullptr;
}

TStatId UAblePlayParticleEffectTask::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePlayParticleEffectTask, STATGROUP_Able);
}

void UAblePlayParticleEffectTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Location, TEXT("Location"));
    ABL_BIND_DYNAMIC_PROPERTY(Ability, m_EffectTemplate, TEXT("Effect Template"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Scale, TEXT("Scale"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_DynamicScaleSize, TEXT("Dynamic Scale"));

	for (UAbleParticleEffectParam* Param : m_Parameters)
	{
		if (Param)
		{
			Param->BindDynamicDelegates(Ability);
		}
	}
}

void UAblePlayParticleEffectTask::SetEffectTemplate(UParticleSystem* Effect)
{
	m_EffectTemplate = Effect;
}

bool UAblePlayParticleEffectTask::UseWorldRotation(const UAbleAbilityContext& Context) const
{
	if (m_Location.GetRelativeDirType() == ESPRelativeDirType::Default)
	{
		return (!m_Location.IsUseSocketRotation() && m_Location.IsUseActorRotation()) || m_Location.IsUseWorldRotation();
	}
	return m_Location.NeedModifyRotation(Context);
}

void UAblePlayParticleEffectTask::ApplyParticleEffectParameter(const UAbleParticleEffectParam* Parameter, const TWeakObjectPtr<const UAbleAbilityContext>& Context, UFXSystemComponent* EffectSystem, TWeakObjectPtr<UParticleSystemComponent>& CascadeComponent) const
{
	if (!EffectSystem)
	{
		return;
	}

	if (Parameter->IsA<UAbleParticleEffectParamContextActor>())
	{
		const UAbleParticleEffectParamContextActor* ContextActorParam = Cast<UAbleParticleEffectParamContextActor>(Parameter);
		if (ContextActorParam->GetContextActorType() != EAbleAbilityTargetType::ATT_TargetActor)
		{
			if (AActor* FoundActor = GetSingleActorFromTargetType(Context, ContextActorParam->GetContextActorType()))
			{
				EffectSystem->SetActorParameter(Parameter->GetParameterName(), FoundActor);
			}
		}
	}
	else if (Parameter->IsA<UAbleParticleEffectParamLocation>())
	{
		const UAbleParticleEffectParamLocation* LocationParam = Cast<UAbleParticleEffectParamLocation>(Parameter);
		FTransform outTransform;
		LocationParam->GetLocation(Context).GetTransform(*Context.Get(), outTransform);
		EffectSystem->SetVectorParameter(Parameter->GetParameterName(), outTransform.GetTranslation());
	}
	else if (Parameter->IsA<UAbleParticleEffectParamFloat>())
	{
		const UAbleParticleEffectParamFloat* FloatParam = Cast<UAbleParticleEffectParamFloat>(Parameter);
		EffectSystem->SetFloatParameter(Parameter->GetParameterName(), FloatParam->GetFloat(Context));
	}
	else if (Parameter->IsA<UAbleParticleEffectParamColor>())
	{
		const UAbleParticleEffectParamColor* ColorParam = Cast<UAbleParticleEffectParamColor>(Parameter);
		EffectSystem->SetColorParameter(Parameter->GetParameterName(), ColorParam->GetColor(Context));
	}
	else if (Parameter->IsA<UAbleParticleEffectParamMaterial>() && CascadeComponent.IsValid())
	{
		const UAbleParticleEffectParamMaterial* MaterialParam = Cast<UAbleParticleEffectParamMaterial>(Parameter);
		CascadeComponent->SetMaterialParameter(Parameter->GetParameterName(), MaterialParam->GetMaterial(Context));
	}
	else if (Parameter->IsA<UAbleParticleEffectParamVector>())
	{
		const UAbleParticleEffectParamVector* VectorParam = Cast<UAbleParticleEffectParamVector>(Parameter);
		EffectSystem->SetVectorParameter(Parameter->GetParameterName(), VectorParam->GetVector(Context));
	}
	else if (Parameter->IsA<UAbleParticleEffectParamBool>())
	{
		const UAbleParticleEffectParamBool* BoolParam = Cast<UAbleParticleEffectParamBool>(Parameter);
		EffectSystem->SetBoolParameter(Parameter->GetParameterName(), BoolParam->GetBool(Context));
	}
}

FString UAblePlayParticleEffectTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePlayParticleEffectTask");
}

float UAblePlayParticleEffectTask::GetOwnerScale_Lua_Implementation(const UAbleAbilityContext* Context) const
{
	return 1.0f;
}

#if WITH_EDITOR

FText UAblePlayParticleEffectTask::GetDescriptiveTaskName() const
{
    const FText FormatText = LOCTEXT("AblePlayParticleEffectTaskFormat", "{0}: {1}");
    FString ParticleName = TEXT("<null>");
    if (m_EffectTemplate.LoadSynchronous())
    {
        ParticleName = m_EffectTemplate->GetName();
    }
    return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ParticleName));
}

FText UAblePlayParticleEffectTask::GetRichTextTaskSummary() const
{
    FTextBuilder StringBuilder;

    StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

    FString EffectName = TEXT("NULL");
    if (m_EffectTemplateDelegate.IsBound())
    {
        EffectName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_EffectTemplateDelegate.GetFunctionName().ToString() });
    }
    else
    {
        EffectName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_EffectTemplate\" Filter=\"ParticleSystem\">{0}</>"), { m_EffectTemplate ? m_EffectTemplate->GetName() : EffectName });
    }

    StringBuilder.AppendLineFormat(LOCTEXT("AblePlayParticleEffectTaskRichFmt1", "\t- Effect: {0}"), FText::FromString(EffectName));

    EffectName = TEXT("NULL");
    StringBuilder.AppendLineFormat(LOCTEXT("AblePlayParticleEffectTaskRichFmt2", "\t- Niagara Effect: {0}"), FText::FromString(EffectName));

    return StringBuilder.ToText();
}

EDataValidationResult UAblePlayParticleEffectTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;
    return result;
}

bool UAblePlayParticleEffectTask::FixUpObjectFlags()
{
	return false;
}

#endif

#undef LOCTEXT_NAMESPACE

