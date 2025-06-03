// Copyright (c) Extra Life Studios, LLC. All rights reserved.
#include "ableAbilityTypes.h"

#include "AbleCoreSPPrivate.h"
#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityContext.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

FAbleAbilityTargetTypeLocation::FAbleAbilityTargetTypeLocation()
	: m_Source(EAbleAbilityTargetType::ATT_Self),
	  m_TargetIndex(0),
	  m_SpawnTraceToGround(false),
	  m_IgnoreSelfWhenTraceToGround(false),
	  m_ZOffsetAfterTraceToGround(0.0f),
	  m_AllowRotateTraceToGround(false),
	  m_Offset(ForceInitToZero),
	  m_bWorldOffset(false),
	  m_Rotation(ForceInitToZero),
	  m_Socket(NAME_None),
	  m_UseSocketRotation(false),
	  m_UseActorRotation(false),
	  m_UseWorldRotation(false),
	  m_UseActorMeshRotation(false),
	  m_bCanAimingByTargetBehind(false),
	  m_bModifyBehindTarget(true),
	  m_RotationMin(ForceInitToZero),
	  m_RotationhMax(ForceInitToZero),
	  m_limitRotationRoll(false),
	  m_limitRotationPitch(false),
	  m_limitRotationYaw(false),
	  m_bAutoAdjustRotationPitch(false),
	  m_RandomLocationIndex(0),
	  m_NavigationExtend(ForceInitToZero)
{
}

bool FAbleAbilityTargetTypeLocation::TryLocationToGround(const UAbleAbilityContext& Context, FTransform& OutTransform) const
{
	bool bSucessTrace = false;
	if (m_SpawnTraceToGround)
	{
#if WITH_EDITOR
		if(!FApp::IsGame())
		{
			const FVector InLocation = OutTransform.GetLocation();
			OutTransform.SetLocation(FVector(InLocation.X, InLocation.Y, 0));
			return true;
		}
#endif
		
		UWorld* CurrentWorld = Context.GetWorld();
		if (CurrentWorld)
		{
			const FVector InLocation = OutTransform.GetLocation();
			/*UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(CurrentWorld);
			if (NavSystem)
			{
				FNavLocation NavLocation;
				//Project To Ground
				bool bSuccess = NavSystem->ProjectPointToNavigation(InLocation, NavLocation, m_NavigationExtend.Equals(INVALID_NAVEXTENT) ? INVALID_NAVEXTENT : m_NavigationExtend);
				if (bSuccess)
				{
					OutTransform.SetLocation(FVector(InLocation.X, InLocation.Y, NavLocation.Location.Z));
					bSucessTrace = true;
				}
				else
				{
					UE_LOG(LogAbleSP, Warning, TEXT("TryLocationToGround Project Point To Navigation Fail!"));
				}
			}*/
			//If NavSystem Fail, Try Trace To Ground
			if (!bSucessTrace)
			{
				FHitResult HitResult;
				FHitResult FinalResult;
				FCollisionQueryParams CollisionParams;
				if (m_IgnoreSelfWhenTraceToGround)
				{
					CollisionParams.AddIgnoredActor(Context.GetOwner());
				}
				const float TraceDistance = m_NavigationExtend.Equals(INVALID_NAVEXTENT) ? 500 : m_NavigationExtend.Z;
				FVector EndPos = InLocation + FVector(0, 0, -TraceDistance);
				FCollisionObjectQueryParams CheckTrace(UAbleAbilityBlueprintLibrary::GetCollisionChannelFromTraceType(&Context, ESPAbleTraceType::AT_WorldStatic));
				CheckTrace.AddObjectTypesToQuery(UAbleAbilityBlueprintLibrary::GetCollisionChannelFromTraceType(&Context, ESPAbleTraceType::AT_GameBuilding));
				CheckTrace.AddObjectTypesToQuery(UAbleAbilityBlueprintLibrary::GetCollisionChannelFromTraceType(&Context, ESPAbleTraceType::AT_InterWater));
				bSucessTrace = CurrentWorld->LineTraceSingleByObjectType(HitResult, InLocation, EndPos, CheckTrace, CollisionParams);
				float HitDistance = 0.0f;
				FVector HitLocation = FVector(0, 0, 0);
				if (bSucessTrace)
				{
					HitDistance = UKismetMathLibrary::Vector_Distance(InLocation, HitResult.ImpactPoint);
					HitLocation = FVector(InLocation.X, InLocation.Y, HitResult.ImpactPoint.Z);
					FinalResult = HitResult;
				}
				EndPos = InLocation + FVector(0, 0, TraceDistance);
				bool bChangeTrace = false;
				if (CurrentWorld->LineTraceSingleByObjectType(HitResult, InLocation, EndPos, CheckTrace, CollisionParams))
				{
					if (!bSucessTrace | (HitDistance > UKismetMathLibrary::Vector_Distance(InLocation, HitResult.ImpactPoint)))
					{
						HitLocation = FVector(InLocation.X, InLocation.Y, HitResult.ImpactPoint.Z);
						FinalResult = HitResult;
						bChangeTrace = true;
					}
					bSucessTrace = true;
				}
				if (bSucessTrace)
				{
					HitLocation.Z += m_ZOffsetAfterTraceToGround;
					OutTransform.SetLocation(HitLocation);
					if (m_AllowRotateTraceToGround)
					{
						//如果向上检测，法线乘以-1，使其始终保证生成物旋转朝上
						if (bChangeTrace)
						{
							FinalResult.ImpactNormal = -FinalResult.ImpactNormal;
						}
						FVector ResultVector = UKismetMathLibrary::ProjectVectorOnToPlane(OutTransform.GetRotation().Vector(), FinalResult.ImpactNormal);
						OutTransform.SetRotation(FRotationMatrix::MakeFromXZ(ResultVector, FinalResult.ImpactNormal).ToQuat());
					}
				}
				else
				{
					UE_LOG(LogAbleSP, Warning, TEXT("TryLocationToGround Trace To Ground Fail!"));
				}
			}
		}
	}
	return bSucessTrace;
}

bool FAbleAbilityTargetTypeLocation::GetSocketTransform(const UAbleAbilityContext& Context, FTransform& OutTransform) const
{
	AActor* BaseActor = GetSourceActor(Context);
	if (!BaseActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));
		return false;
	}
	else
	{
		if (!m_Socket.IsNone())
		{
			USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, BaseActor, m_Socket);
			if (!SkeletalMesh)
			{
				SkeletalMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
			}
			if (SkeletalMesh)
			{
				OutTransform = SkeletalMesh->GetSocketTransform(m_Socket);
				return true;
			}
		}	
	}
	return false;
}

bool FAbleAbilityTargetTypeLocation::NeedModifyRotation(const UAbleAbilityContext& Context) const
{
#if WITH_EDITOR
	if (!FApp::IsGame())
	{
		// Filter out self in editor browsing mode
		if (Context.GetTargetActors().Num() > 0 && Context.GetTargetActors()[0] == Context.GetOwner())
		{
			return false;
		}
	}
#endif
	return (m_RelativeDirType == ESPRelativeDirType::SpawnToTargetDir || m_RelativeDirType == ESPRelativeDirType::SpawnToTargetPitchDirButForward || m_RelativeDirType == ESPRelativeDirType::SpawnToTargetSnapDir || m_RelativeDirType == ESPRelativeDirType::InitialOrientation)  && Context.GetTargetActors().Num() > 0 && m_Source != ATT_TargetActor;
}

bool FAbleAbilityTargetTypeLocation::GetModifyRotation(const UAbleAbilityContext& Context, FRotator& Rotator) const
{
	if (NeedModifyRotation(Context))
	{
		TWeakObjectPtr<AActor> TargetActor = nullptr;
		// 先判断目标数组长度合法性
		if (Context.GetTargetActorsWeakPtr().Num())
		{
			// 配置的目标Index，并Clamp到0和目标数组长度之间，保证当配置错误时能返回数组的最后一个单位
			int32 TargetActorIndex = FMath::Clamp(m_TargetIndex, 0, Context.GetTargetActorsWeakPtr().Num() - 1);
			if (Context.GetTargetActorsWeakPtr()[TargetActorIndex].IsValid())
			{
				TargetActor = Context.GetTargetActorsWeakPtr()[TargetActorIndex];
			}
		}
#if WITH_EDITOR
		if (!FApp::IsGame())
		{
			// Filter out self in editor browsing mode
			if (TargetActor.Get() == Context.GetOwner())
			{
				return false;
			}
		}
#endif
		if (TargetActor.IsValid())
		{
			AActor* BaseActor = GetSourceActor(Context);
			if (!BaseActor)
			{
				UE_LOG(LogAbleSP, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));
				return false;
			}
			else
			{
				FVector SourceLocation = BaseActor->GetActorLocation();
				if (!m_Socket.IsNone())
				{
					USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, BaseActor, m_Socket);
					if (!SkeletalMesh)
					{
						SkeletalMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
					}

					if (SkeletalMesh)
					{
						SourceLocation = SkeletalMesh->GetSocketLocation(m_Socket);
					}
				}
				FTransform InTransform = FTransform(Rotator, SourceLocation);
				if (m_bWorldOffset)
				{
					InTransform.AddToTranslation(m_Offset);
				}
				const bool bSucessTrace = TryLocationToGround(Context, InTransform);
				SourceLocation = InTransform.GetLocation();
				
				FVector TargetLocation = TargetActor->GetActorLocation();
				if (m_RelativeDirType == ESPRelativeDirType::SpawnToTargetPitchDirButForward)
				{
					//Project
					TargetLocation = UKismetMathLibrary::ProjectPointOnToPlane(TargetLocation, SourceLocation, BaseActor->GetActorRightVector());
					// check use snap to ground adjustment
					if(m_bAutoAdjustRotationPitch)
					{
						AutoAdjustPitchTargetLocation(TargetLocation,SourceLocation, Context);
					}
				}
				else if (m_RelativeDirType == ESPRelativeDirType::SpawnToTargetSnapDir)
				{
					//Project
					const FVector TargetActorLocationSnap = Context.GetTargetActorLocationSnap();
					TargetLocation = UKismetMathLibrary::ProjectPointOnToPlane(TargetActorLocationSnap, SourceLocation, BaseActor->GetActorRightVector());
				}else if (m_RelativeDirType == ESPRelativeDirType::InitialOrientation)
				{
					Rotator = Context.GetTargetActorRotationSnap();
					return true;
				}
				const FVector ForwardVector = BaseActor->GetActorForwardVector();
				const FVector DirectionToCheck = TargetLocation - SourceLocation; // 获取角色到点的向量	
				float DotProduct = FVector::DotProduct(ForwardVector, DirectionToCheck);
				if (DotProduct <= 0 && !m_bCanAimingByTargetBehind)
				{
					//目标点在角色后方忽视
					return false;
				}
				if (DotProduct <= 0 && m_bModifyBehindTarget)
				{
					//求TargetLocation对于ForwardVector向量的垂直对称目标点
					const FVector ProjectionPoint = UKismetMathLibrary::ProjectPointOnToPlane(TargetLocation, SourceLocation, ForwardVector);
					TargetLocation = 2 * ProjectionPoint - TargetLocation;
				}
				FRotator Rot = (m_AllowRotateTraceToGround && bSucessTrace) ? InTransform.Rotator() : UKismetMathLibrary::FindLookAtRotation(SourceLocation, TargetLocation);

				UE_LOG(LogAbleSP, Log, TEXT("GetModifyRotation AbilityId:%d AbilityUniqueID:%d SourceLocation:%s TargetLocation%s"), Context.GetAbilityId(), Context.GetAbilityUniqueID(), *SourceLocation.ToString(), *TargetLocation.ToString());
				
				Rotator = FRotator(
					m_limitRotationPitch? FMath::Clamp(Rot.Pitch, m_RotationMin.Pitch, m_RotationhMax.Pitch) : Rot.Pitch,
					m_limitRotationYaw? FMath::Clamp(Rot.Yaw, m_RotationMin.Yaw, m_RotationhMax.Yaw) : Rot.Yaw,
					m_limitRotationRoll? FMath::Clamp(Rot.Roll, m_RotationMin.Roll, m_RotationhMax.Roll): Rot.Roll
					);
				return true;
			}
		}
	}
	return false;
}

void FAbleAbilityTargetTypeLocation::AutoAdjustPitchTargetLocation(FVector& OutLocation,const FVector& SourceLocation, const UAbleAbilityContext& Context) const
{
	if(!IsValid(&Context) || Context.GetTargetActorsWeakPtr().Num() < 1) return;
	TWeakObjectPtr<AActor> TargetActor = nullptr;
	// 配置的目标Index，并Clamp到0和目标数组长度之间，保证当配置错误时能返回数组的最后一个单位
	int32 TargetActorIndex = FMath::Clamp(m_TargetIndex, 0, Context.GetTargetActorsWeakPtr().Num() - 1);
	if (Context.GetTargetActorsWeakPtr()[TargetActorIndex].IsValid())
	{
		TargetActor = Context.GetTargetActorsWeakPtr()[TargetActorIndex];
	}
	AActor* BaseActor = GetSourceActor(Context);

	if(!TargetActor.IsValid() || !IsValid(BaseActor)) return;
	// check if target is flying
	if(auto Character = Cast<ACharacter>(TargetActor))
	{
		UCharacterMovementComponent const *  MoveComp = Character->GetCharacterMovement();
		if (!MoveComp || MoveComp->MovementMode == EMovementMode::MOVE_Flying) return;

		// check if self is flying
		if(auto BaseCharacter = Cast<ACharacter>(BaseActor))
		{
			UCharacterMovementComponent const * BaseActorMoveComp = BaseCharacter->GetCharacterMovement();
			if (!MoveComp || BaseActorMoveComp->MovementMode == EMovementMode::MOVE_Flying) return;
			FVector TargetLocation = TargetActor->GetActorLocation();
			float halfHeight = Character->GetDefaultHalfHeight();
			float capsuleScaleZ = 1.0;
			if(IsValid(Character->GetCapsuleComponent()))
				capsuleScaleZ = Character->GetCapsuleComponent()->GetComponentScale().Z;
			float TopZ = TargetLocation.Z + Character->GetDefaultHalfHeight();
			float BottomZ = TargetLocation.Z - Character->GetDefaultHalfHeight();
			
			if(SourceLocation.Z >= BottomZ && SourceLocation.Z <= TopZ)
			{
				OutLocation.Z = SourceLocation.Z;
			}
			else
			{
				OutLocation.Z = TargetLocation.Z;
			}

#if WITH_EDITOR
			bool bDebug = BaseActorMoveComp->GetWorld() && BaseActorMoveComp->GetWorld()->WorldType == EWorldType::EditorPreview;
			if(bDebug)
			{
				DrawDebugDirectionalArrow(BaseActor->GetWorld(),FVector(TargetLocation.X,TargetLocation.Y,TopZ),FVector(TargetLocation.X,TargetLocation.Y,BottomZ), 20.0f,FColor::Red, false,30.0,0,2.0f);
				DrawDebugDirectionalArrow(BaseActor->GetWorld(),OutLocation,SourceLocation, 20.0f,FColor::Red, false,30.0,0,2.0f); 
			}
#endif
		}
	}
}


void FAbleAbilityTargetTypeLocation::GetTargetTransform(const UAbleAbilityContext& Context, int32 TargetIndex, FTransform& OutTransform) const
{
	if (TargetIndex >= Context.GetTargetActors().Num())
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Ability [%s] tried to get a target transform at time [%2.3f], but none were available. Using Self Actor's transform instead.\n"), *Context.GetAbility()->GetAbilityName(), Context.GetCurrentTime());
		OutTransform = Context.GetSelfActor()->GetActorTransform();
		if (m_UseActorMeshRotation)
		{
			const USkeletalMeshComponent* SkeletalMesh = Context.GetSelfActor()->FindComponentByClass<USkeletalMeshComponent>();
			if (SkeletalMesh)
			{
				OutTransform.SetRotation(SkeletalMesh->GetComponentQuat() * FQuat(FRotator(0, 90, 0)));
			}
		}
		TryLocationToGround(Context, OutTransform);
		return;
	}

	if (m_Source == EAbleAbilityTargetType::ATT_World)
	{
		OutTransform = FTransform(FQuat(m_Rotation), m_Offset);
		TryLocationToGround(Context, OutTransform);
		return;
	}

	TWeakObjectPtr<AActor> TargetActor = Context.GetTargetActorsWeakPtr()[TargetIndex];

	if (TargetActor.IsValid())
	{
		if (!m_Socket.IsNone())
		{
			USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, TargetActor.Get(), m_Socket);
			if (!SkeletalMesh)
			{
				SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
			}

			if (SkeletalMesh)
			{
				if (m_UseSocketRotation)
				{
					OutTransform = SkeletalMesh->GetSocketTransform(m_Socket);
				}
				else
				{
					if (m_UseActorMeshRotation)
					{
						OutTransform.SetRotation(SkeletalMesh->GetComponentQuat() * FQuat(FRotator(0, 90, 0)));
					}
                    else if (m_UseWorldRotation)
                    {
                        OutTransform.SetRotation(m_WorldRotator.Quaternion());
                    }
					OutTransform.SetTranslation(SkeletalMesh->GetSocketLocation(m_Socket));
				}
			}
		}
		else
		{
			if (m_Source == EAbleAbilityTargetType::ATT_Camera && !TargetActor->IsA<ACameraActor>())
			{
				FVector ActorEyes;
				FRotator ActorEyesRot;
				TargetActor->GetActorEyesViewPoint(ActorEyes, ActorEyesRot);
				OutTransform = FTransform(FQuat(ActorEyesRot), ActorEyes);
			}
			else if (m_Source == EAbleAbilityTargetType::ATT_Location)
			{
				OutTransform.SetTranslation(Context.GetTargetLocation());
			}
			else if(m_Source==EAbleAbilityTargetType::ATT_RandomLocations)
			{
				const TArray<FVector> Locations= Context.GetTargetLocations();
				if (Locations.IsValidIndex(m_RandomLocationIndex))
				{
					OutTransform.SetTranslation(Locations[m_RandomLocationIndex]);
				}
			}
			else
			{
				OutTransform = TargetActor->GetActorTransform();
			}
		}

		OutTransform.ConcatenateRotation(m_Rotation.Quaternion());

		//FQuat Rotator = OutTransform.GetRotation();
		//FVector LocalSpaceOffset = Rotator.GetForwardVector() * m_Offset.X;
		//LocalSpaceOffset += Rotator.GetRightVector() * m_Offset.Y;
		//LocalSpaceOffset += Rotator.GetUpVector() * m_Offset.Z;

		OutTransform.AddToTranslation(m_Offset);
	}
	else
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));

		OutTransform = FTransform(m_Rotation, m_Offset);
	}
	TryLocationToGround(Context, OutTransform);
}

void FAbleAbilityTargetTypeLocation::GetTransform(const UAbleAbilityContext& Context, FTransform& OutTransform) const
{
	FRotator Rotation = FRotator::ZeroRotator;
	bool bModifyRotation = GetModifyRotation(Context, Rotation);
	OutTransform = FTransform::Identity;
	
	if (m_Source == EAbleAbilityTargetType::ATT_World)
	{
		OutTransform = FTransform(FQuat(m_Rotation), m_Offset);
		TryLocationToGround(Context, OutTransform);
		return;
	}
	else if(m_Source==EAbleAbilityTargetType::ATT_RandomLocations)
	{
		const TArray<FVector> Locations= Context.GetTargetLocations();
		if (Locations.IsValidIndex(m_RandomLocationIndex))
		{
			OutTransform.SetTranslation(Locations[m_RandomLocationIndex]);
			return;
		}
	}

	AActor* BaseActor = GetSourceActor(Context);

	if (!BaseActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Unable to find an Actor for our Targeting source, Query will be from the origin. Is this intended?"));

		OutTransform = FTransform(m_Rotation, m_Offset);
	}
	else
	{
		if (!m_Socket.IsNone())
		{
			USkeletalMeshComponent* SkeletalMesh = Context.GetAbility()->GetSkeletalMeshComponentForActorBP(&Context, BaseActor, m_Socket);
			if (!SkeletalMesh)
			{
				SkeletalMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
			}

			if (SkeletalMesh)
			{
				if (bModifyRotation)
				{
					OutTransform.SetTranslation(SkeletalMesh->GetSocketLocation(m_Socket));
					OutTransform.SetRotation(Rotation.Quaternion());
				}
				else if (m_UseSocketRotation)
				{
					OutTransform = SkeletalMesh->GetSocketTransform(m_Socket);
				}
				else
				{
					OutTransform.SetTranslation(SkeletalMesh->GetSocketLocation(m_Socket));
					if(m_UseWorldRotation)
                    {
                        OutTransform.SetRotation(m_WorldRotator.Quaternion());
                    }
					else if (m_UseActorMeshRotation)
					{
						OutTransform.SetRotation(SkeletalMesh->GetComponentQuat() * FQuat(FRotator(0, 90, 0)));
					}
					else
					{
						OutTransform.SetRotation(BaseActor->GetActorRotation().Quaternion());
					}
				}
			}
		}
		else
		{
			// If we're not a camera actor, we get our "Eyes point" which is just the camera by another name.
			// Otherwise, it's safe just to grab our actor location.
			if (m_Source == EAbleAbilityTargetType::ATT_Camera && !BaseActor->IsA<ACameraActor>())
			{
				FVector ActorEyes;
				FRotator ActorEyesRot;
				BaseActor->GetActorEyesViewPoint(ActorEyes, ActorEyesRot);
				OutTransform = FTransform(FQuat(ActorEyesRot), ActorEyes);
			}
			else if (m_Source == EAbleAbilityTargetType::ATT_Location)
			{
				OutTransform.SetTranslation(Context.GetTargetLocation());
			}
			else
			{
				OutTransform = BaseActor->GetActorTransform();
				if (m_UseActorMeshRotation)
				{
					const USkeletalMeshComponent* SkeletalMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
					if (SkeletalMesh)
					{
						OutTransform.SetRotation(SkeletalMesh->GetComponentQuat() * FQuat(FRotator(0, 90, 0)));
					}
				}
			}
			if (bModifyRotation)
			{
				OutTransform.SetRotation(Rotation.Quaternion());
			}
		}

		OutTransform.ConcatenateRotation(m_Rotation.Quaternion());

		FQuat Rotator = OutTransform.GetRotation();
		if (!m_bWorldOffset)
		{
			FVector LocalSpaceOffset = Rotator.GetForwardVector() * m_Offset.X;
        	LocalSpaceOffset += Rotator.GetRightVector() * m_Offset.Y;
        	LocalSpaceOffset += Rotator.GetUpVector() * m_Offset.Z;
			OutTransform.AddToTranslation(LocalSpaceOffset);	
		}
		else
		{
			OutTransform.AddToTranslation(m_Offset);
		}
	
		TryLocationToGround(Context, OutTransform);
		//OutTransform.AddToTranslation(m_Offset);
	}
}

AActor* FAbleAbilityTargetTypeLocation::GetSourceActor(const UAbleAbilityContext& Context) const
{
	switch (m_Source)
	{
		case EAbleAbilityTargetType::ATT_World:
		case EAbleAbilityTargetType::ATT_Location:
		case EAbleAbilityTargetType::ATT_Self:
		case EAbleAbilityTargetType::ATT_Camera: // Camera we just return self.
		{
			return Context.GetSelfActor();
		}
		break;
		case EAbleAbilityTargetType::ATT_Instigator:
		{
			return Context.GetInstigator();
		}
		break;
		case EAbleAbilityTargetType::ATT_Owner:
		{
			return Context.GetOwner();
		}
		break;
		case EAbleAbilityTargetType::ATT_TargetActor:
		{
			// 先判断目标数组长度合法性
			if (Context.GetTargetActorsWeakPtr().Num())
			{
				// 配置的目标Index，并Clamp到0和目标数组长度之间，保证当配置错误时能返回数组的最后一个单位
				int32 TargetActorIndex = FMath::Clamp(m_TargetIndex, 0, Context.GetTargetActorsWeakPtr().Num() - 1);
				if (Context.GetTargetActorsWeakPtr()[TargetActorIndex].IsValid())
				{
					return Context.GetTargetActorsWeakPtr()[TargetActorIndex].Get();
				}
			}
			else 
			{
				return nullptr;
			}
		}
		break;
		case EAbleAbilityTargetType::ATT_RandomLocations:
			{
				return Context.GetOwner();
			}
		break;
		default:
		{
		}
		break;
	}

	return nullptr;
}