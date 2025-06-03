// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SPGame/Skill/Task/SPSkillPlayAnimationWithMoveTask.h"

#include "Curves/CurveVector.h"
#include "Game/SPGame/Character/SPCharacterMovementComponent.h"
#include "Game/SPGame/Character/SPGameMonsterBase.h"
#include "Game/SPGame/Character/SPRootMotionSource_MoveToForce.h"
#include "ShootGame/Character/Components/FPSCharacterMovementComponent.h"

FString USPSkillPlayAnimationWithMoveTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPSkillPlayAnimationWithMoveTask");
}

void USPSkillPlayAnimationWithMoveTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	OnTaskStartBP_Override(Context.Get());
	Super::OnTaskStart(Context);
}

void USPSkillPlayAnimationWithMoveTask::OnTaskStartBP_Override_Implementation(const UAbleAbilityContext* Context) const
{
	const UAnimationAsset* AnimationAsset = m_AnimationAsset.LoadSynchronous();

	if (!AnimationAsset)
	{
		// UE_LOG(LogSPSkillAbility, Warning, TEXT("No Animation set for PlayAnimationTask in Ability [%s]"), *Context->GetAbility()->GetDisplayName());
		return;
	}

	if (m_EnableAnimMove)
	{
		AActor* Actor = Context->GetSelfActor();
		if (IsValid(Actor))
		{
			USPCharacterMovementComponent* MovementComponent = Actor->FindComponentByClass<USPCharacterMovementComponent>();
			if (IsValid(MovementComponent))
			{
				MovementComponent->SetShouldAnimMove(true, FVector::ZeroVector, m_CloseFriction, m_OnlyZMoveByLoc, false, m_IsMeshSmoothTogether, m_IsMeshForceToCapsule);
				MovementComponent->SetAnimMoveRatio(m_AnimMoveRatio);

				const float AnimMoveRatioInc = Context->GetFloatParameter(FName(TEXT("AnimMoveRatio")));
				if (AnimMoveRatioInc > 0.0f)
				{
					MovementComponent->SetAnimMoveRatio(AnimMoveRatioInc);
				}
				UE_LOG(LogTemp, Warning, TEXT("USPSkillPlayAnimationWithMoveTask::OnTaskStart"));
				
				if (m_DirectLoadXml)
				{
					MovementComponent->StartPlayAnimMovement(AnimationAsset->GetName());
				}

				if (m_IsSetVelocityZtoZero)
				{
					MovementComponent->Velocity.Z = 0.0f;
				}
			}
		}
	}

	if (m_EnableRootMotion)
	{
		TSharedPtr<FSPRootMotionSource_MoveToForce> MoveToForce = MakeShared<FSPRootMotionSource_MoveToForce>();
		MoveToForce->InstanceName = FName("SPSkillPlayAnimationWithMoveTask");
		MoveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
		MoveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
		MoveToForce->Priority = 1000;
		MoveToForce->Duration = GetDuration();
		UCurveVector* m_PathOffsetCurve = NewObject<UCurveVector>();
	
		USPAnimMoveXmlManager* XmlManager = USPAnimMoveXmlManager::GetInstance();
		FString Name = AnimationAsset->GetName();
		if (IsValid(XmlManager))
		{
			FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Feature/StarP/Assets/Animation/AnimMove/"), Name + ".xml");
			USPAnimMovementXmlData* XmlData = XmlManager->GetLoadXmlData(Path);

			MoveToForce->XmlData = XmlData;
			MoveToForce->Ratio = m_RootMotionRatio;

			AActor* Actor = Context->GetSelfActor();
			if (IsValid(Actor))
			{
				FVector TempLoc = Actor->GetActorLocation();
				MoveToForce->StartLocation = TempLoc;
				// JumpForce->TargetLocation = FVector(TempLoc.X + XmlData->m_totalOffsetX, TempLoc.Y + XmlData->m_totalOffsetY, TempLoc.Z + XmlData->m_totalOffsetZ);
				MoveToForce->TargetLocation = TempLoc;

			}
		}

		AActor* Actor = Context->GetSelfActor();
		if (IsValid(Actor))
		{
			USPCharacterMovementComponent* MovementComponent = Actor->FindComponentByClass<USPCharacterMovementComponent>();
			if (IsValid(MovementComponent))
			{
				MovementComponent->RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToForce);
			}
		}
	}
}

void USPSkillPlayAnimationWithMoveTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	const EAbleAbilityTaskResult result) const
{
	OnTaskEndBP_Override(const_cast<UAbleAbilityContext*>(Context.Get()), result);
	Super::OnTaskEnd(Context, result);
}

void USPSkillPlayAnimationWithMoveTask::OnTaskEndBP_Override_Implementation(UAbleAbilityContext* Context,
	const EAbleAbilityTaskResult result) const
{
	if (m_IsRemainVelocity)
	{
		Context->SetIntParameter(TEXT("bShouldSetVelocity"), 1);
		AActor* Actor = Context->GetSelfActor();
		if (IsValid(Actor))
		{
			USPCharacterMovementComponent* MovementComponent = Actor->FindComponentByClass<USPCharacterMovementComponent>();
			if (IsValid(MovementComponent))
			{
				Context->SetVectorParameter(TEXT("VelocityValue"), MovementComponent->LastAnimMoveVelocity);
				UE_LOG(LogTemp, Log, TEXT("USPSkillPlayAnimationWithMoveTask::OnTaskEndBP_Override %s Velocity: x:%f y:%f z:%f"), *Context->GetOwner()->GetName(), MovementComponent->LastAnimMoveVelocity.X, MovementComponent->LastAnimMoveVelocity.Y, MovementComponent->LastAnimMoveVelocity.Z);

			}
		}
	}
	
	if (m_EnableAnimMove)
	{
		AActor* Actor = Context->GetSelfActor();
		if (IsValid(Actor))
		{
			USPCharacterMovementComponent* MovementComponent = Actor->FindComponentByClass<USPCharacterMovementComponent>();
			if (IsValid(MovementComponent))
			{
				MovementComponent->SetShouldAnimMove(false, FVector::ZeroVector, m_CloseFriction);

				UE_LOG(LogTemp, Warning, TEXT("USPSkillPlayAnimationWithMoveTask::OnTaskEnd"));
			}
		}
	}

	if (m_EnableRootMotion)
	{
		AActor* Actor = Context->GetSelfActor();
		if (IsValid(Actor))
		{
			USPCharacterMovementComponent* MovementComponent = Actor->FindComponentByClass<USPCharacterMovementComponent>();
			if (IsValid(MovementComponent))
			{
				MovementComponent->RemoveRootMotionSourceByID(MovementComponent->RootMotionSourceID);
			}
		}
	}
}
