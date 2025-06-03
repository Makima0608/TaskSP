// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableMoveToTask.h"

#include "ableAbility.h"
#include "ableAbilityUtilities.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"


#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleMoveToScratchPad::UAbleMoveToScratchPad()
{

}

UAbleMoveToScratchPad::~UAbleMoveToScratchPad()
{

}

UAbleMoveToTask::UAbleMoveToTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_TargetType(MTT_Actor), m_TargetActor(ATT_Self), m_UpdateTargetPerFrame(false), m_AcceptableRadius(0),
	  m_Use2DDistanceChecks(false),
	  m_UseNavPathing(false),
	  m_CancelOnNoPathAvailable(false),
	  m_NavPathFindingType(),
	  m_UseAsyncNavPathFinding(false), m_Speed(0), m_TimeOut(0),
	  m_CancelMoveOnInterrupt(false),
	  m_TaskRealm(EAbleAbilityTaskRealm::ATR_Server)
{
}

UAbleMoveToTask::~UAbleMoveToTask()
{

}

FString UAbleMoveToTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleMoveToTask");
}

void UAbleMoveToTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleMoveToTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	UAbleMoveToScratchPad* ScratchPad = CastChecked<UAbleMoveToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->CurrentTargetLocation = GetTargetLocation(Context);
	ScratchPad->ActiveMoveRequests.Empty();
	ScratchPad->ActivePhysicsMoves.Empty();
	ScratchPad->AsyncQueryIdArray.Empty();
	ScratchPad->CompletedAsyncQueries.Empty();
	ScratchPad->NavPathDelegate.Unbind();

	TArray<TWeakObjectPtr<AActor>> TaskTargets;
	GetActorsForTask(Context, TaskTargets);

	for (TWeakObjectPtr<AActor>& Target : TaskTargets)
	{
		if (Target.IsValid())
		{
			if (m_UseNavPathing)
			{
				StartPathFinding(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
			}
			else
			{
				SetPhysicsVelocity(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
			}

		}
	}
}

void UAbleMoveToTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleMoveToTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{

	UAbleMoveToScratchPad* ScratchPad = CastChecked<UAbleMoveToScratchPad>(Context->GetScratchPadForTask(this));

	// Handle any completed Async pathing queries.
	TArray<TPair<uint32, FNavPathSharedPtr>>::TIterator itProcess = ScratchPad->CompletedAsyncQueries.CreateIterator();
	for(; itProcess; ++itProcess)
	{

		TPair<uint32, TWeakObjectPtr<APawn>>* foundRecord = ScratchPad->AsyncQueryIdArray.FindByPredicate([&](const TPair<uint32, TWeakObjectPtr<APawn>>& LHS)
		{
			return LHS.Key == itProcess->Key;
		});

		if (!foundRecord)
		{
			// No record found? We may have replaced it with a more recent request. Ignore it.
			itProcess.RemoveCurrent();
			continue;
		}

		// We have a path and our actor is still valid. Start the move.
		if (foundRecord->Value.IsValid() && itProcess->Value.IsValid())
		{
			if (UPathFollowingComponent* PathFindingComponent = foundRecord->Value->FindComponentByClass<UPathFollowingComponent>())
			{
				// Start the move.
				FAIRequestID MoveRequest = PathFindingComponent->RequestMove(FAIMoveRequest(ScratchPad->CurrentTargetLocation), itProcess->Value);
				ScratchPad->ActiveMoveRequests.Add(TPair<FAIRequestID, TWeakObjectPtr<APawn>>(MoveRequest, foundRecord->Value));

#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Requested Move for Target %s."), *foundRecord->Value->GetName()));
				}
#endif
			}
		}

		ScratchPad->AsyncQueryIdArray.Remove(*foundRecord);
		itProcess.RemoveCurrent();
	}

	// If we need to update our path, do so.
	if (m_UpdateTargetPerFrame)
	{
		FVector newEndPoint = GetTargetLocation(Context);

		if ((m_Use2DDistanceChecks ?
			FVector::DistSquared2D(ScratchPad->CurrentTargetLocation, newEndPoint) :
			FVector::DistSquared(ScratchPad->CurrentTargetLocation, newEndPoint)) > (m_AcceptableRadius * m_AcceptableRadius))
		{
			// New distance, redo our pathing logic.
			ScratchPad->CurrentTargetLocation = newEndPoint;

			ScratchPad->ActiveMoveRequests.Empty(ScratchPad->ActiveMoveRequests.Num());
			ScratchPad->ActivePhysicsMoves.Empty(ScratchPad->ActivePhysicsMoves.Num());
			ScratchPad->AsyncQueryIdArray.Empty();

			ScratchPad->CurrentTargetLocation = newEndPoint;

			TArray<TWeakObjectPtr<AActor>> TaskTargets;
			GetActorsForTask(Context, TaskTargets);

			for (TWeakObjectPtr<AActor>& Target : TaskTargets)
			{
				if (Target.IsValid())
				{
					if (m_UseNavPathing)
					{
						StartPathFinding(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
					}
					else
					{
						SetPhysicsVelocity(Context, Target.Get(), ScratchPad->CurrentTargetLocation, ScratchPad);
					}
				}
			}
		}
	}

}

void UAbleMoveToTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult Result) const
{
	Super::OnTaskEnd(Context, Result);
	OnTaskEndBP(Context.Get(), Result);
}

void UAbleMoveToTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (!Context)
	{
		return;
	}

	if (result == EAbleAbilityTaskResult::Interrupted && m_CancelMoveOnInterrupt)
	{
		UAbleMoveToScratchPad* ScratchPad = CastChecked<UAbleMoveToScratchPad>(Context->GetScratchPadForTask(this));

		for (TPair<FAIRequestID, TWeakObjectPtr<APawn>>& RequestPawnPair : ScratchPad->ActiveMoveRequests)
		{
			if (RequestPawnPair.Value.IsValid())
			{
				if (UPathFollowingComponent* PathComponent = RequestPawnPair.Value->FindComponentByClass<UPathFollowingComponent>())
				{
					PathComponent->AbortMove(*this, FPathFollowingResultFlags::UserAbort, RequestPawnPair.Key);
				}
			}
		}

		for (TWeakObjectPtr<AActor>& PhysicsActor : ScratchPad->ActivePhysicsMoves)
		{
			if (UCharacterMovementComponent* CharacterMovementComponent = PhysicsActor->FindComponentByClass<UCharacterMovementComponent>())
			{
				CharacterMovementComponent->RequestDirectMove(FVector::ZeroVector, true);
			}
			else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(PhysicsActor->GetRootComponent()))
			{
				PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}
	}
}

UAbleAbilityTaskScratchPad* UAbleMoveToTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleMoveToScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAbleMoveToScratchPad>(Context.Get());
}

TStatId UAbleMoveToTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleMoveToTask, STATGROUP_Able);
}

void UAbleMoveToTask::BindDynamicDelegates( UAbleAbility* Ability )
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetLocation, TEXT("Target Location"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Speed, TEXT("Speed"));
}

bool UAbleMoveToTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleMoveToTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_TimeOut > 0.0f && Context->GetCurrentTime() >= GetEndTime())
	{
		return true;
	}

	UAbleMoveToScratchPad* ScratchPad = CastChecked<UAbleMoveToScratchPad>(Context->GetScratchPadForTask(this));

	ScratchPad->ActiveMoveRequests.RemoveAll([](const TPair<FAIRequestID, TWeakObjectPtr<APawn>>& LHS)
	{
		if (LHS.Value.IsValid())
		{
			return true;
		}

		if (UPathFollowingComponent* PathComponent = LHS.Value->FindComponentByClass<UPathFollowingComponent>())
		{
			return PathComponent->DidMoveReachGoal();
		}

		return false;
	});

	ScratchPad->ActivePhysicsMoves.RemoveAll([&](const TWeakObjectPtr<AActor>& LHS)
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
	return ScratchPad->ActiveMoveRequests.Num() > 0 || ScratchPad->ActivePhysicsMoves.Num() > 0 || ScratchPad->AsyncQueryIdArray.Num() > 0;
}

FVector UAbleMoveToTask::GetTargetLocation(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (m_TargetType.GetValue() == EAbleMoveToTarget::MTT_Actor)
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
			return Context->GetSelfActor()->GetActorLocation();
		}

		return actor->GetActorLocation();
	}
	else
	{
		return ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TargetLocation);
	}
}

void UAbleMoveToTask::StartPathFinding(const TWeakObjectPtr<const UAbleAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAbleMoveToScratchPad* ScratchPad) const
{
	if (UPathFollowingComponent* PathFindingComponent = Target->FindComponentByClass<UPathFollowingComponent>())
	{
		if (APawn* Pawn = Cast<APawn>(Target))
		{
			if (AController* Controller = Pawn->GetController())
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld());
				const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef());
				if (NavData)
				{
					FPathFindingQuery Query(Controller, *NavData, Controller->GetNavAgentLocation(), ScratchPad->CurrentTargetLocation);
					if (m_UseAsyncNavPathFinding)
					{
						if (!ScratchPad->NavPathDelegate.IsBound())
						{
							ScratchPad->NavPathDelegate.BindUObject(ScratchPad, &UAbleMoveToScratchPad::OnNavPathQueryFinished);
						}

						// Async Query, queue it up and wait for results.
						int Id = NavSys->FindPathAsync(FNavAgentProperties(Controller->GetNavAgentPropertiesRef().AgentRadius, Controller->GetNavAgentPropertiesRef().AgentHeight), Query, ScratchPad->NavPathDelegate, m_NavPathFindingType.GetValue() == EAblePathFindingType::Regular ? EPathFindingMode::Regular : EPathFindingMode::Hierarchical);
						ScratchPad->AsyncQueryIdArray.Add(TPair<uint32, TWeakObjectPtr<APawn>>(Id, Pawn));
					}
					else
					{
						FPathFindingResult result = NavSys->FindPathSync(Query, m_NavPathFindingType.GetValue() == EAblePathFindingType::Regular ? EPathFindingMode::Regular : EPathFindingMode::Hierarchical);
						if (result.IsPartial() && m_CancelOnNoPathAvailable)
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Target %s was only able to find a partial path. Skipping."), *Pawn->GetName()));
							}
#endif
						}
						else
						{
							// Start the move.
							FAIRequestID MoveRequest = PathFindingComponent->RequestMove(FAIMoveRequest(ScratchPad->CurrentTargetLocation), result.Path);
							ScratchPad->ActiveMoveRequests.Add(TPair<FAIRequestID, TWeakObjectPtr<APawn>>(MoveRequest, Pawn));

#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Requested Move for Target %s."), *Pawn->GetName()));
							}
#endif
						}
					}
				}
			}
			else
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Actor %s wants to use Nav Pathing but has no controller? "), *Target->GetName());
			}
		}
		else
		{
			UE_LOG(LogAbleSP, Warning, TEXT("Actor %s wants to use Nav Pathing but is not a Pawn. Please set the Actor to inherit from APawn. "), *Target->GetName());
		}
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Actor %s wants to use Nav Pathing but has no PathFollowingComponent! Please add one."), *Target->GetName());
	}
}

void UAbleMoveToTask::SetPhysicsVelocity(const TWeakObjectPtr<const UAbleAbilityContext>& Context, AActor* Target, const FVector& EndLocation, UAbleMoveToScratchPad* ScratchPad) const
{
	FVector towardsTarget = EndLocation - Target->GetActorLocation();
	float towardsLengthSqr = towardsTarget.Size2D();
	towardsTarget.Normalize();
	FVector reqVelocity = towardsTarget * ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Speed) * FMath::Min(1.0f, towardsLengthSqr);

	if (UCharacterMovementComponent* CharacterMovementComponent = Target->FindComponentByClass<UCharacterMovementComponent>())
	{
		CharacterMovementComponent->RequestDirectMove(reqVelocity, false);
		ScratchPad->ActivePhysicsMoves.Add(Target);

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on CharacterMovementComponent to %s"), *Target->GetName(), *reqVelocity.ToCompactString()));
		}
#endif
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		PrimitiveComponent->SetPhysicsLinearVelocity(reqVelocity);
		ScratchPad->ActivePhysicsMoves.Add(Target);

#if !(UE_BUILD_SHIPPING)
		if (IsVerbose())
		{
			PrintVerbose(Context, FString::Printf(TEXT("Set Linear Velocity on Target %s on Primitive Root Component to %s"), *Target->GetName(), *reqVelocity.ToCompactString()));
		}
#endif
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Actor %s doesn't have a Character Movement Component or Primitive Root component. Unable to set velocity."), *Target->GetName());
	}
}

void UAbleMoveToScratchPad::OnNavPathQueryFinished(uint32 Id, ENavigationQueryResult::Type typeData, FNavPathSharedPtr PathPtr)
{
	CompletedAsyncQueries.Add(TPair<uint32, FNavPathSharedPtr>(Id, PathPtr));
}

#if WITH_EDITOR

FText UAbleMoveToTask::GetDescriptiveTaskName() const
{
	return FText::FormatOrdered(LOCTEXT("AbleMoveToTaskDesc", "Move to {0}"), FText::FromString(FAbleSPLogHelper::GetTargetTargetEnumAsString(m_TargetActor.GetValue())));
}

#endif 

EAbleAbilityTaskRealm UAbleMoveToTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; }

#undef LOCTEXT_NAMESPACE