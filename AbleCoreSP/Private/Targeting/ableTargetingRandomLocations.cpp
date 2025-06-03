// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingRandomLocations.h"

#include "ableAbility.h"
#include "ableAbilityDebug.h"
#include "ableSettings.h"
#include "Engine/World.h"

UAbleTargetingRandomLocations::UAbleTargetingRandomLocations(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbleTargetingRandomLocations::~UAbleTargetingRandomLocations()
{
}

void UAbleTargetingRandomLocations::FindTargets(UAbleAbilityContext& Context) const
{
	const FAbleAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Location);

	const AActor* SourceActor = Location.GetSourceActor(Context);
	if (SourceActor == nullptr)
	{
		return;
	}

	const FVector OriginSpot = SourceActor->GetActorLocation();
	TArray<FVector> NewLocations;

	const int32 LocationNum = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_LocationNum);
	const int32 TargetingShape = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_TargetingShape);
	const bool bIgnoreZAxis = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_IgnoreZAxis);

	switch (TargetingShape)
	{
	case 0: // Sphere
		{
			const float InnerRadius = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_InnerRadius);
			const float OuterRadius = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_OuterRadius);

			for (int32 i = 0; i < LocationNum; i++)
			{
				FVector ResultLocation = FMath::VRand();
				ResultLocation *= FMath::RandRange(InnerRadius, OuterRadius);
				ResultLocation += OriginSpot;

				ResultLocation.Z = bIgnoreZAxis ? OriginSpot.Z : ResultLocation.Z;


				NewLocations.Add(ResultLocation);
			}
			break;
		}
	case 1: // Box
		{
			const FVector InnerExtent = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_InnerExtent);
			const FVector OuterExtent = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_OuterExtent);
			for (int32 i = 0; i < LocationNum; i++)
			{
				FVector ResultLocation = FMath::VRand();

				ResultLocation.X = ResultLocation.X * FMath::RandRange(InnerExtent.X, OuterExtent.X);
				ResultLocation.Y = ResultLocation.Y * FMath::RandRange(InnerExtent.Y, OuterExtent.Y);
				ResultLocation += OriginSpot;

				ResultLocation.Z = bIgnoreZAxis ? OriginSpot.Z : ResultLocation.Z * FMath::RandRange(InnerExtent.Z, OuterExtent.Z);

				NewLocations.Add(ResultLocation);
			}
			break;
		}
	default:
		checkNoEntry();
		break;
	}

	if (NewLocations.Num() <= 0)
	{
		return;
	}

	Context.SetTargetLocations(NewLocations);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
	}
#endif // UE_BUILD_SHIPPING
}

void UAbleTargetingRandomLocations::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TargetingShape, "Targeting Shape");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_LocationNum, "Location Numbers");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_IgnoreZAxis, "Ignore Z Axis");

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_InnerRadius, "Inner Radius");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OuterRadius, "Outer Radius");

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_InnerExtent, "Inner Extent");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OuterExtent, "Outer Extent");

	Super::BindDynamicDelegates(Ability);
}

float UAbleTargetingRandomLocations::CalculateRange() const
{
	return m_OuterRadius;
}

#if WITH_EDITOR
void UAbleTargetingRandomLocations::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
	// const int32 LocationNum = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_LocationNum);
	FTransform TargetTransform = FTransform();
	for (const FVector& Location : Context.GetTargetLocations())
	{
		TargetTransform.SetLocation(Location);
		FAbleAbilityDebug::DrawSphereQuery(Context.GetWorld(), TargetTransform, 100);
	}
}
#endif
