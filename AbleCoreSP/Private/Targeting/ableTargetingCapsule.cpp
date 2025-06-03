// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingCapsule.h"

#include "ableAbility.h"
#include "ableAbilityDebug.h"
#include "ableSettings.h"
#include "Engine/World.h"

UAbleTargetingCapsule::UAbleTargetingCapsule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Height(100.0f),
	m_Radius(50.0f)
{

}

UAbleTargetingCapsule::~UAbleTargetingCapsule()
{

}

void UAbleTargetingCapsule::FindTargets(UAbleAbilityContext& Context) const
{
	FAbleAbilityTargetTypeLocation Location = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Location);

	AActor* SourceActor = Location.GetSourceActor(Context);
    if (SourceActor == nullptr)
	{
        return;
	}

	UWorld* World = SourceActor->GetWorld();
	FTransform QueryTransform;
	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(&Context, m_Height);

	FCollisionObjectQueryParams ObjectQuery;
	GetCollisionObjectParams(&Context, ObjectQuery);

	if (IsUsingAsync() && USPAbleSettings::IsAsyncEnabled())
	{
		if (!Context.HasValidAsyncHandle()) // If we don't have a handle, create our query.
		{
			Location.GetTransform(Context, QueryTransform);

			FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

			FTraceHandle AsyncHandle = World->AsyncOverlapByObjectType(QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, CapsuleShape);
			Context.SetAsyncHandle(AsyncHandle);
		}
		else // Poll for completion.
		{
			FOverlapDatum Datum;
			if (World->QueryOverlapData(Context.GetAsyncHandle(), Datum))
			{
				ProcessResults(Context, Datum.OutOverlaps);
				FTraceHandle Empty;
				Context.SetAsyncHandle(Empty); // Reset our handle.
			}

			return;
		}
	}
	else // Sync Query
	{
		Location.GetTransform(Context, QueryTransform);

		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

		TArray<FOverlapResult> Results;
		if (World->OverlapMultiByObjectType(Results, QueryTransform.GetTranslation(), QueryTransform.GetRotation(), ObjectQuery, CapsuleShape))
		{
			ProcessResults(Context, Results);
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawCapsuleQuery(World, QueryTransform, Radius, Height);
	}
#endif // UE_BUILD_SHIPPING
}

void UAbleTargetingCapsule::BindDynamicDelegates(UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Height, "Height");

	Super::BindDynamicDelegates(Ability);
}

float UAbleTargetingCapsule::CalculateRange() const
{
	FVector Capsule(m_Radius * 2.0f, m_Radius * 2.0f, m_Height);
	FVector RotatedCapsule;

	FQuat Rotation = FQuat(m_Location.GetRotation());

	RotatedCapsule = Rotation.GetForwardVector() + Capsule.X;
	RotatedCapsule += Rotation.GetRightVector() + Capsule.Y;
	RotatedCapsule += Rotation.GetUpVector() + Capsule.Z;

	if (m_CalculateAs2DRange)
	{
		return m_Location.GetOffset().Size2D() + RotatedCapsule.Size2D();
	}

	return m_Location.GetOffset().Size() + RotatedCapsule.Size();
}

void UAbleTargetingCapsule::ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = Context.GetMutableTargetActors();

	for (const FOverlapResult& Result : Results)
	{
		if (AActor* ResultActor = Result.GetActor())
		{
			TargetActors.Add(ResultActor);
		}
	}

	// Run filters.
	FilterTargets(Context);
}

#if WITH_EDITOR
void UAbleTargetingCapsule::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
	AActor* SourceActor = m_Location.GetSourceActor(Context);
	if (SourceActor != nullptr)
	{
		FTransform QueryTransform;
		m_Location.GetTransform(Context, QueryTransform);

		FAbleAbilityDebug::DrawCapsuleQuery(Context.GetWorld(), QueryTransform, m_Radius, m_Height);
	}
}
#endif