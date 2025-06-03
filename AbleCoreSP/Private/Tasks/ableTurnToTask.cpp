// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableTurnToTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#if (!UE_BUILD_SHIPPING)
#include "ableAbilityUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleTurnToTaskScratchPad::UAbleTurnToTaskScratchPad()
{

}

UAbleTurnToTaskScratchPad::~UAbleTurnToTaskScratchPad()
{

}

UAbleTurnToTask::UAbleTurnToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_RotationTarget(EAbleAbilityTargetType::ATT_TargetActor),
	m_UseRotationVector(false),
	m_RotationVector(ForceInitToZero),
	m_TrackTarget(false),
	m_SetYaw(true),
	m_SetPitch(false),
	m_Blend(GetDuration()),
	m_UseTaskDurationAsBlendTime(true),
	m_bOpenClientTurn(false),
	m_TaskRealm(EAbleAbilityTaskRealm::ATR_ClientAndServer),
	m_UseSelfRotation(false),
	m_bUseAngularVelocity(false),
	m_AngularVelocity(0.0f),
	m_bScaleAngularVelocity(false)
{

}

UAbleTurnToTask::~UAbleTurnToTask()
{

}

FString UAbleTurnToTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleTurnToTask");
}

void UAbleTurnToTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleTurnToTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	ScratchPad->InProgressTurn.Empty();
	ScratchPad->TurningBlend = m_Blend;

	if (m_UseTaskDurationAsBlendTime)
	{
		ScratchPad->TurningBlend.SetBlendTime(GetDuration());
	}

	AActor* TargetActor = GetSingleActorFromTargetType(Context, m_RotationTarget.GetValue());

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor>& TurnTarget : TaskTargets)
	{
		if (!TargetActor) continue;
		FRotator TargetRotation = GetTargetRotation(Context, TurnTarget.Get(), TargetActor);
#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Setting up turning for Actor %s with a Target turn of %s."), *TurnTarget->GetName(), *TargetRotation.ToCompactString()));
		}
#endif
		ScratchPad->InProgressTurn.Add(FTurnToTaskEntry(TurnTarget.Get(), TargetRotation));
	}

}

void UAbleTurnToTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleTurnToTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

	UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

	ScratchPad->TurningBlend.Update(deltaTime);
	const float BlendingValue = ScratchPad->TurningBlend.GetBlendedValue();
	const AActor* TargetActor = GetSingleActorFromTargetType(Context, m_RotationTarget.GetValue());
	if (!UKismetSystemLibrary::IsServer(Context) && !m_bOpenClientTurn)
	{
		return;
	}
	for (FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
	{
		if (Entry.Actor.IsValid())
		{
			if (m_TrackTarget)
			{
				// Update our Target rotation.
				Entry.Target = GetTargetRotation(Context, Entry.Actor.Get(), TargetActor);
			}
			if (m_bUseAngularVelocity)
			{
				const float CurrentYaw = Entry.Actor->GetActorRotation().Yaw;
				float DeltaYaw = Entry.Target.Yaw - CurrentYaw;
				const float MaxTickYawValue = GetCustomAngularVelocity(Entry.Actor.Get()) * deltaTime;
				
				if (DeltaYaw > 180)
					DeltaYaw -= 360;
				else if (DeltaYaw < -180)
					DeltaYaw += 360;
				
				const bool DirectionYaw = DeltaYaw >= 0 ? true : false; 
				TurnAnimation(Entry.Actor,DeltaYaw);
				if ((DirectionYaw ? 1 : -1) * DeltaYaw <= MaxTickYawValue)
				{
					TurnToSetActorRotation(Entry.Actor, FRotator(Entry.Actor->GetActorRotation().Pitch, Entry.Target.Yaw, Entry.Actor->GetActorRotation().Roll));
				}
				else
				{
					TurnToSetActorRotation(Entry.Actor, FRotator(Entry.Actor->GetActorRotation().Pitch, CurrentYaw + (DirectionYaw ? 1 : -1) * MaxTickYawValue, Entry.Actor->GetActorRotation().Roll));
				}
			}
			else
			{
				FRotator LerpedRotation = FMath::Lerp(Entry.Actor->GetActorRotation(), Entry.Target, BlendingValue);
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Setting Actor %s rotation to %s ."), *Entry.Actor->GetName(), *LerpedRotation.ToCompactString()));
				}
#endif
				TurnAnimation(Entry.Actor,Entry.Target.Yaw - Entry.Actor->GetActorRotation().Yaw);
				TurnToSetActorRotation(Entry.Actor, LerpedRotation);
			}
		}
	}
}

void UAbleTurnToTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAbleTurnToTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{
	if (!Context)
	{
		return;
	}

	UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	
	if (!UKismetSystemLibrary::IsServer(Context) && !m_bOpenClientTurn)
	{
		return;
	}
	
	for (const FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
	{
		if (Entry.Actor.IsValid())
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Setting Actor %s rotation to %s ."), *Entry.Actor->GetName(), *Entry.Target.ToCompactString()));
			}
#endif
			if (!m_bUseAngularVelocity)
			{
				TurnToSetActorRotation(Entry.Actor, Entry.Target);
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleTurnToTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleTurnToTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleTurnToTaskScratchPad>(Context.Get());
}

TStatId UAbleTurnToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleTurnToTask, STATGROUP_Able);
}

void UAbleTurnToTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_UseRotationVector, "Use Rotation Vector");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_RotationVector, "Rotation Vector");
}

void UAbleTurnToTask::TurnToSetActorRotation(const TWeakObjectPtr<AActor> TargetActor, const FRotator& TargetRotation) const
{
	if (TargetActor.IsValid())
	{
		TargetActor->SetActorRotation(TargetRotation);
	}
}

void UAbleTurnToTask::TurnAnimation(const TWeakObjectPtr<AActor> TargetActor, const float DeltaYaw) const
{
}

float UAbleTurnToTask::GetCustomAngularVelocity_Implementation(AActor* TargetActor) const
{
	return m_AngularVelocity;
}

FRotator UAbleTurnToTask::GetTargetRotation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const
{
	float Yaw = 0.0f;
	float Pitch = 0.0f;

	FVector ToTarget = GetTargetVector(Context, Source, Destination);
	ToTarget.Normalize();

	FVector2D YawPitch = ToTarget.UnitCartesianToSpherical();

	if (m_SetYaw)
	{
		Yaw = FMath::RadiansToDegrees(YawPitch.Y);
	}

	if (m_SetPitch)
	{
		Pitch = FMath::RadiansToDegrees(YawPitch.X);
	}

	FRotator OutRotator(Pitch, Yaw, 0.0f);
	
	FRotator ActorRotationPitchRoll(0.0f,0.0f,0.0f);
	if (m_UseSelfRotation)
	{
		if(IsValid(Source))
		{
			FRotator ActorRotation = Source->GetActorRotation();
			ActorRotationPitchRoll.Pitch = ActorRotation.Pitch;
			ActorRotationPitchRoll.Roll = ActorRotation.Roll;
		}
	}
	return OutRotator + m_RotationOffset + ActorRotationPitchRoll;
}

FVector UAbleTurnToTask::GetTargetVector(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const
{
	bool useVector = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_UseRotationVector);

	if (useVector)
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_RotationVector);
	}

	if (Source && Destination)
	{
		return Destination->GetActorLocation() - Source->GetActorLocation();
	}

	return FVector::ZeroVector;
}

#if WITH_EDITOR

FText UAbleTurnToTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AbleTurnToTaskFormat", "{0}: {1}");
	FString TargetName = FAbleSPLogHelper::GetTargetTargetEnumAsString(m_RotationTarget);
	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(TargetName));
}

void UAbleTurnToTask::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (GetDuration() < m_Blend.GetBlendTime())
	{
		m_EndTime = GetStartTime() + m_Blend.GetBlendTime();
	}
}

#endif

bool UAbleTurnToTask::IsSingleFrameBP_Implementation() const { return m_Blend.GetBlendTimeRemaining() < KINDA_SMALL_NUMBER; }

EAbleAbilityTaskRealm UAbleTurnToTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE