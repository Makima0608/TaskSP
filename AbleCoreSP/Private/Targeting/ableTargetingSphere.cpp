// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Targeting/ableTargetingSphere.h"

#include "ableAbility.h"
#include "ableAbilityDebug.h"
#include "ableSettings.h"
#include "Engine/World.h"

UAbleTargetingSphere::UAbleTargetingSphere(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Radius(100.0f)
{

}

UAbleTargetingSphere::~UAbleTargetingSphere()
{

}

void UAbleTargetingSphere::FindTargets(UAbleAbilityContext& Context) const
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

	FCollisionObjectQueryParams ObjectQuery;
	GetCollisionObjectParams(&Context, ObjectQuery);

	if (IsUsingAsync() && USPAbleSettings::IsAsyncEnabled())
	{
		if (!Context.HasValidAsyncHandle()) // If we don't have a handle, create our query.
		{
			Location.GetTransform(Context, QueryTransform);

			FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

			FTraceHandle AsyncHandle = World->AsyncOverlapByObjectType(QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, SphereShape);
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

		FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

		TArray<FOverlapResult> Results;
		if (World->OverlapMultiByObjectType(Results, QueryTransform.GetTranslation(), QueryTransform.GetRotation(), ObjectQuery, SphereShape))
		{
			ProcessResults(Context, Results);
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawSphereQuery(World, QueryTransform, Radius);
	}
#endif // UE_BUILD_SHIPPING
}

void UAbleTargetingSphere::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");

	Super::BindDynamicDelegates(Ability);
}

float UAbleTargetingSphere::CalculateRange() const
{
	const float OffsetSize = m_CalculateAs2DRange ? m_Location.GetOffset().Size2D() : m_Location.GetOffset().Size();

	return m_Radius + OffsetSize;
}

void UAbleTargetingSphere::ProcessResults(UAbleAbilityContext& Context, const TArray<struct FOverlapResult>& Results) const
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
void UAbleTargetingSphere::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
	AActor* SourceActor = m_Location.GetSourceActor(Context);
	if (SourceActor != nullptr)
	{
		FTransform QueryTransform;
		m_Location.GetTransform(Context, QueryTransform);

		FAbleAbilityDebug::DrawSphereQuery(Context.GetWorld(), QueryTransform, m_Radius);
	}
}
#endif