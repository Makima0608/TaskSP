// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableSpawnActorTask.h"

#include "ableAbility.h"
#include "ableAbilityContext.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleSpawnActorTaskScratchPad::UAbleSpawnActorTaskScratchPad()
{

}

UAbleSpawnActorTaskScratchPad::~UAbleSpawnActorTaskScratchPad()
{

}

UAbleSpawnActorTask::UAbleSpawnActorTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_ActorClass(),
	m_AmountToSpawn(1),
	m_SpawnCollision(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn),
	m_InitialVelocity(ForceInitToZero),
	m_SetOwner(true),
	m_OwnerTargetType(EAbleAbilityTargetType::ATT_Self),
	m_AttachToOwnerSocket(false),
	m_AttachmentRule(EAttachmentRule::KeepRelative),
	m_SocketName(NAME_None),
	m_InheritOwnerLinearVelocity(false),
	m_MarkAsTransient(true),
	m_DestroyAtEnd(false),
	m_FireEvent(false),
	m_Name(NAME_None),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{

}

UAbleSpawnActorTask::~UAbleSpawnActorTask()
{

}

FString UAbleSpawnActorTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleSpawnActorTask");
}

void UAbleSpawnActorTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	check(Context.IsValid());

	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleSpawnActorTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	TArray<TWeakObjectPtr<AActor>> OutActors;
	GetActorsForTask(Context, OutActors);

	if (OutActors.Num() == 0)
	{
		// Just incase, to not break previous content.
		OutActors.Add(Context->GetSelfActor());
	}

	TSubclassOf<AActor> ActorClass = m_ActorClass;
	int NumToSpawn = m_AmountToSpawn;
	FAbleAbilityTargetTypeLocation SpawnTargetLocation = m_SpawnLocation;

	if (!ActorClass.GetDefaultObject())
	{
		UE_LOG(LogAbleSP, Warning, TEXT("SpawnActorTask for Ability [%s] does not have a class specified."), *(Context->GetAbility()->GetAbilityName()));
		return;
	}

	UAbleSpawnActorTaskScratchPad* ScratchPad = nullptr;
	if (m_DestroyAtEnd)
	{
		ScratchPad = Cast<UAbleSpawnActorTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		ScratchPad->SpawnedActors.Empty();
	}
	
	FTransform SpawnTransform;
	SpawnTargetLocation.GetTransform(*Context, SpawnTransform);

	// FVector SpawnLocation;

	for (int32 i = 0; i < OutActors.Num(); ++i)
	{
		UWorld* ActorWorld = OutActors[i]->GetWorld();
		FActorSpawnParameters SpawnParams;

		if (SpawnTargetLocation.GetSourceTargetType() == EAbleAbilityTargetType::ATT_TargetActor)
		{
			SpawnTargetLocation.GetTargetTransform(*Context, i, SpawnTransform);
		}

		if (m_SetOwner)
		{
			SpawnParams.Owner = GetSingleActorFromTargetType(Context, m_OwnerTargetType);
		}

		SpawnParams.SpawnCollisionHandlingOverride = m_SpawnCollision;

		if (m_MarkAsTransient)
		{
			SpawnParams.ObjectFlags = EObjectFlags::RF_Transient; // We don't want to save anything on this object.
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Spawning Actor %s using Transform %s."), *ActorClass->GetName(), *SpawnTransform.ToString()));
		}
#endif
		// Go through our spawns.
		for (int32 SpawnIndex = 0; SpawnIndex < NumToSpawn; ++SpawnIndex)
		{
			SpawnParams.Name = MakeUniqueObjectName(ActorWorld, ActorClass);

			AActor* SpawnedActor = ActorWorld->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
			if (!SpawnedActor)
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Failed to spawn Actor %s using Transform %s."), *ActorClass->GetName(), *SpawnTransform.ToString());
				return;
			}

			FVector InheritedVelocity;
			if (m_InheritOwnerLinearVelocity && SpawnedActor->GetOwner())
			{
				InheritedVelocity = SpawnedActor->GetOwner()->GetVelocity();
			}

			if (!m_InitialVelocity.IsNearlyZero())
			{
				// Use the Projectile Movement Component if they have one setup since this is likely used for spawning projectiles.
				if (UProjectileMovementComponent* ProjectileComponent = SpawnedActor->FindComponentByClass<UProjectileMovementComponent>())
				{
					ProjectileComponent->SetVelocityInLocalSpace(m_InitialVelocity + InheritedVelocity);
				}
				else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SpawnedActor->GetRootComponent()))
				{
					PrimitiveComponent->AddImpulse(m_InitialVelocity + InheritedVelocity);
				}
			}

			if (m_AttachToOwnerSocket && SpawnedActor->GetOwner())
			{
				if (USkeletalMeshComponent* SkeletalComponent = SpawnedActor->GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
				{
					if (USceneComponent* SceneComponent = SpawnedActor->FindComponentByClass<USceneComponent>())
					{
						FAttachmentTransformRules AttachRules(m_AttachmentRule, false);
						SceneComponent->AttachToComponent(SkeletalComponent, AttachRules, m_SocketName);
					}
				}
			}

			if (m_DestroyAtEnd)
			{
				ScratchPad->SpawnedActors.Add(SpawnedActor);
			}

			if (m_FireEvent)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Calling OnSpawnedActorEvent with event name %s, Spawned Actor %s and Spawn Index %d."), *m_Name.ToString(), *SpawnedActor->GetName(), SpawnIndex));
				}
#endif
				Context->GetAbility()->OnSpawnedActorEventBP(Context, m_Name, SpawnedActor, SpawnIndex);
			}
		}
	}

}

void UAbleSpawnActorTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleSpawnActorTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (m_DestroyAtEnd && Context)
	{
		UAbleSpawnActorTaskScratchPad* ScratchPad = Cast<UAbleSpawnActorTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;

		if (ScratchPad->SpawnedActors.Num())
		{
			for (AActor* SpawnedActor : ScratchPad->SpawnedActors)
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Destroying Spawned Actor %s."), *SpawnedActor->GetName()));
				}
#endif
				UWorld* SpawnedWorld = SpawnedActor->GetWorld();
				SpawnedWorld->DestroyActor(SpawnedActor);
			}

			ScratchPad->SpawnedActors.Empty();
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleSpawnActorTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (m_DestroyAtEnd)
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleSpawnActorTaskScratchPad::StaticClass();
			return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
		}

		return NewObject<UAbleSpawnActorTaskScratchPad>(Context.Get());
	}

	return nullptr;
}

TStatId UAbleSpawnActorTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleSpawnActorTask, STATGROUP_Able);
}

void UAbleSpawnActorTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_ActorClass, TEXT("Actor Class"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AmountToSpawn, TEXT("Amount to Spawn"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SpawnLocation, TEXT("Location"));
}

#if WITH_EDITOR

FText UAbleSpawnActorTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleSpawnActorTaskFormat", "{0}: {1} x{2}");
	FString ActorName = TEXT("<null>");
	if (*m_ActorClass)
	{
		if (AActor* Actor = Cast<AActor>(m_ActorClass->GetDefaultObject()))
		{
			ActorName = Actor->GetName();
		}
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(ActorName), m_AmountToSpawn);
}

FText UAbleSpawnActorTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString ActorName;
	if (m_ActorClassDelegate.IsBound())
	{
		ActorName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_ActorClassDelegate.GetFunctionName().ToString() });
	}
	else
	{
		ActorName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_ActorClass\" Filter=\"Actor\">{0}</>"), { m_ActorClass ? m_ActorClass->GetName() : TEXT("NULL") });
	}

	FString AmountToSpawnString;
	if (m_AmountToSpawnDelegate.IsBound())
	{
		AmountToSpawnString = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AmountToSpawnDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AmountToSpawnString = FString::Format(TEXT("<a id=\"AbleTextDecorators.IntValue\" style=\"RichText.Hyperlink\" PropertyName=\"m_AmountToSpawn\" MinValue=\"1\">{0}</>"), { m_AmountToSpawn });
	}

	StringBuilder.AppendLineFormat(LOCTEXT("AbleSpawnActorTaskRichFmt", "\t- Actor Class: {0}\n\t- Amount to Spawn: {1}"), FText::FromString(ActorName), FText::FromString(AmountToSpawnString));

	return StringBuilder.ToText();
}

EDataValidationResult UAbleSpawnActorTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_ActorClass == nullptr)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoClassDefined", "No Actor Class defined: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    if (m_FireEvent)
    {
        UFunction* function = AbilityContext->GetClass()->FindFunctionByName(TEXT("OnSpawnedActorEventBP"));
        if (function == nullptr || function->Script.Num() == 0)
        {
            ValidationErrors.Add(FText::Format(LOCTEXT("OnSpawnedActorEventBP_NotFound", "Function 'OnSpawnedActorEventBP' not found: {0}"), AssetName));
            result = EDataValidationResult::Invalid;
        }
    }

    return result;
}

#endif

bool UAbleSpawnActorTask::IsSingleFrameBP_Implementation() const { return !m_DestroyAtEnd; }

EAbleAbilityTaskRealm UAbleSpawnActorTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE