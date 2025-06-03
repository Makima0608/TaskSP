// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableJumpToTask.h"

#include "ableAbility.h"
#include "ableAbilityUtilities.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleJumpToScratchPad::UAbleJumpToScratchPad()
{

}

UAbleJumpToScratchPad::~UAbleJumpToScratchPad()
{

}

UAbleJumpToTask::UAbleJumpToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_TargetType(JTT_Actor), m_TargetActor(ATT_Self), m_TrackActor(false), m_TargetActorOffset(0),
	  m_JumpHeight(0),
	  m_Speed(0),
	  m_AcceptableRadius(0),
	  m_Use2DDistanceChecks(false),
	  m_EndTaskImmediately(false),
	  m_CancelMoveOnInterrupt(false),
	  m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{
}

UAbleJumpToTask::~UAbleJumpToTask()
{

}

FString UAbleJumpToTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleJumpToTask");
}

void UAbleJumpToTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleJumpToTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleJumpToScratchPad* ScratchPad = CastChecked<UAbleJumpToScratchPad>(Context->GetScratchPadForTask(this));

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	ScratchPad->CurrentTargetLocation = GetTargetLocation(Context);
	ScratchPad->JumpingPawns.Empty();

	for (TWeakObjectPtr<AActor>& Target : TaskTargets)
	{
		if (Target.IsValid())
		{
			if (APawn* Pawn = Cast<APawn>(Target))
			{
				SetPhysicsVelocity(Context, Pawn, ScratchPad->CurrentTargetLocation, ScratchPad);
			}
			else
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Actor %s is not a Pawn. Unable to set velocity."), *Target->GetName());
			}
		}
	}
}

void UAbleJumpToTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleJumpToTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
    if (m_TrackActor)
	{
		UAbleJumpToScratchPad* ScratchPad = CastChecked<UAbleJumpToScratchPad>(Context->GetScratchPadForTask(this));
		FVector potentialNewLocation = GetTargetLocation(Context);

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, potentialNewLocation) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, potentialNewLocation)) > (m_AcceptableRadius * m_AcceptableRadius))
		{
			for (TWeakObjectPtr<APawn>& JumpingPawn : ScratchPad->JumpingPawns)
			{
				ScratchPad->CurrentTargetLocation = potentialNewLocation;
				SetPhysicsVelocity(Context, JumpingPawn.Get(), ScratchPad->CurrentTargetLocation, ScratchPad, false);
			}
		}
	}
}

void UAbleJumpToTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const
{
	OnTaskEndBP(Context.Get(), Result);
}

void UAbleJumpToTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{
    UAbleJumpToScratchPad* ScratchPad = CastChecked<UAbleJumpToScratchPad>(Context->GetScratchPadForTask(this));

	if (m_CancelMoveOnInterrupt && result == EAbleAbilityTaskResult::Interrupted)
	{
		for (TWeakObjectPtr<APawn>& JumpingPawn : ScratchPad->JumpingPawns)
		{
			if (UCharacterMovementComponent* CharacterMovementComponent = JumpingPawn->FindComponentByClass<UCharacterMovementComponent>())
			{
				CharacterMovementComponent->RequestDirectMove(FVector::ZeroVector, true);
			}
			else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(JumpingPawn->GetRootComponent()))
			{
				PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}
	}
}

bool UAbleJumpToTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleJumpToTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_EndTaskImmediately)
	{
		return true;
	}

	UAbleJumpToScratchPad* ScratchPad = CastChecked<UAbleJumpToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->JumpingPawns.RemoveAll([&](const TWeakObjectPtr<APawn>& LHS)
	{
		if (!LHS.IsValid())
		{
			return true;
		}

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, LHS->GetActorLocation()) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, LHS->GetActorLocation())) <= (m_AcceptableRadius * m_AcceptableRadius))
		{
			return true;
		}

		return false;
	});

	// We're not done as long as we have some outstanding work.
	return ScratchPad->JumpingPawns.Num() > 0;
}

UAbleAbilityTaskScratchPad* UAbleJumpToTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleJumpToScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleJumpToScratchPad>(Context.Get());
}

TStatId UAbleJumpToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleJumpToTask, STATGROUP_Able);
}

void UAbleJumpToTask::BindDynamicDelegates( UAbleAbility* Ability )
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_JumpHeight, TEXT("Jump Height"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Speed, TEXT("Speed"));
}

#if WITH_EDITOR

FText UAbleJumpToTask::GetDescriptiveTaskName() const
{
	return FText::FormatOrdered(LOCTEXT("AbleJumpToTaskDesc", "Jump to {0}"), FText::FromString(FAbleSPLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
}

#endif

FVector UAbleJumpToTask::GetTargetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (m_TargetType.GetValue() == EAbleJumpToTarget::JTT_Actor)
	{

		AActor* actor = GetSingleActorFromTargetType(Context, m_TargetActor.GetValue());

		if (!actor)
		{
#if !(UE_BUILD_SHIPPING)
			if (IsVerbose())
			{
				PrintVerbose(Context, FString::Printf(TEXT("Failed to find Actor using Target Type %s, using Self."), *FAbleSPLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
			}
#endif
			actor = Context->GetSelfActor();
		}

		return actor->GetActorLocation();
	}
	else
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TargetLocation);
	}
}

void UAbleJumpToTask::SetPhysicsVelocity(const TWeakObjectPtr<const UAbleAbilityContext>& Context, APawn* Target, const FVector& EndLocation, UAbleJumpToScratchPad* ScratchPad, bool addToScratchPad) const
{
	// Vf^2 = Vi^2 + 2 * A * D;
	FVector towardsTarget = EndLocation - Target->GetActorLocation();
	towardsTarget = towardsTarget - (towardsTarget.GetSafeNormal() * m_TargetActorOffset); // Our offset.
	float d = towardsTarget.Size2D();
	float a = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Speed);

	if (UCharacterMovementComponent* CharacterMovementComponent = Target->FindComponentByClass<UCharacterMovementComponent>())
	{
		FVector Vi = FMath::Square(CharacterMovementComponent->Velocity);
		FVector Vf = Vi + 2.0f * (a * d);
		Vf.X = FMath::Sqrt(Vf.X);
		Vf.Y = FMath::Sqrt(Vf.X);
		Vf.Z = FMath::Sqrt(Vf.Z);

		// Jump Height = Gravity * Height.
		Vf += Target->GetActorUpVector() + (Target->GetGravityDirection() * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_JumpHeight));

		CharacterMovementComponent->RequestDirectMove(Vf, false);

		if (addToScratchPad)
		{
			ScratchPad->JumpingPawns.Add(Target);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on CharacterMovementComponent to %s"), *Target->GetName(), *Vf.ToCompactString()));
		}
#endif
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		FVector Vi = FMath::Square(PrimitiveComponent->GetPhysicsLinearVelocity());
		FVector Vf = Vi + 2.0f * (a * d);
		Vf.X = FMath::Sqrt(Vf.X);
		Vf.Y = FMath::Sqrt(Vf.X);
		Vf.Z = FMath::Sqrt(Vf.Z);

		// Jump Height = Gravity * Height.
		Vf += Target->GetActorUpVector() + (Target->GetGravityDirection() * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_JumpHeight));

		PrimitiveComponent->SetPhysicsLinearVelocity(Vf);

		if (addToScratchPad)
		{
			ScratchPad->JumpingPawns.Add(Target);
		}

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on Primitive Root Component to %s"), *Target->GetName(), *Vf.ToCompactString()));
		}
#endif
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Actor %s doesn't have a Character Movement Component or Primitive Root component. Unable to set velocity."), *Target->GetName());
	}
}

bool UAbleJumpToTask::IsSingleFrameBP_Implementation() const { return false; }

EAbleAbilityTaskRealm UAbleJumpToTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE