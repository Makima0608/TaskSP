// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCollisionQueryTypes.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityDebug.h"
#include "AbleCoreSPPrivate.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tasks/ableCollisionQueryTask.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCollisionShape::UAbleCollisionShape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_UseAsyncQuery(false)
{

}

UAbleCollisionShape::~UAbleCollisionShape()
{

}

UWorld* UAbleCollisionShape::GetQueryWorld(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	if (AActor* SourceActor = m_QueryLocation.GetSourceActor(*Context.Get()))
	{
		return SourceActor->GetWorld();
	}

	return GEngine->GetWorld();
}

void UAbleCollisionShape::ProcessAsyncOverlaps(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAbleQueryResult>& OutResults) const
{
	for (const FOverlapResult& Result : Overlaps)
	{
		OutResults.Add(FAbleQueryResult(Result));
	}
}

FName UAbleCollisionShape::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Query_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAbleCollisionShape::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_QueryLocation, TEXT("Query Location"));
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionShape::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_ChannelPresent == ACP_NonePresent && m_CollisionChannels.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoCollisionChannels", "No Collision Channels: {0}"), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

bool UAbleCollisionShape::FixUpObjectFlags()
{
	EObjectFlags oldFlags = GetFlags();

	SetFlags(GetOutermost()->GetMaskedFlags(RF_PropagateToSubObjects));

	if (oldFlags != GetFlags())
	{
		Modify();

		return true;
	}

	return false;
}

#endif

UAbleCollisionShapeBox::UAbleCollisionShapeBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_HalfExtents(100.0f, 100.0f, 100.0f)
{

}

UAbleCollisionShapeBox::~UAbleCollisionShapeBox()
{

}

FTransform UAbleCollisionShapeBox::DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform, bool bOverrideTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
		return FTransform::Identity;
	}

	UWorld* World = SourceActor->GetWorld();

	FTransform QueryTransform;
	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);
	if (bOverrideTransform)
	{
		QueryTransform = ReplaceTransform;
	}
	else
	{
		QueryLocation.GetTransform(*Context.Get(), QueryTransform);

		// Push our query out by our half extents so we aren't centered in the box.
		FQuat Rotation = QueryTransform.GetRotation();

		FVector HalfExtentsOffset = Rotation.GetForwardVector() * HalfExtents.X;

		QueryTransform *= FTransform(HalfExtentsOffset);	
	}

	FCollisionShape Box = FCollisionShape::MakeBox(HalfExtents);

    FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	TArray<FOverlapResult> OverlapResults;
    if (World->OverlapMultiByObjectType(OverlapResults, QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, Box))
	{
		for (FOverlapResult& Result : OverlapResults)
		{
			OutResults.Add(FAbleQueryResult(Result));
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = QueryTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += QueryTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += QueryTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAbleAbilityDebug::DrawBoxQuery(World, QueryTransform, HalfExtents);
	}
#endif
	return QueryTransform;
}

FTraceHandle UAbleCollisionShapeBox::DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	UWorld* World = SourceActor->GetWorld();

	QueryLocation.GetTransform(*Context.Get(), OutQueryTransform);

	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);

	// Push our query out by our half extents so we aren't centered in the box.
	FQuat Rotation = OutQueryTransform.GetRotation();

	FVector HalfExtentsOffset = Rotation.GetForwardVector() * HalfExtents.X;

	OutQueryTransform *= FTransform(HalfExtentsOffset);

	FCollisionShape Box = FCollisionShape::MakeBox(HalfExtents);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = OutQueryTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += OutQueryTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += OutQueryTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAbleAbilityDebug::DrawBoxQuery(World, OutQueryTransform, AlignedBox);
	}
#endif

    FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

    return World->AsyncOverlapByObjectType(OutQueryTransform.GetLocation(), OutQueryTransform.GetRotation(), ObjectQuery, Box);
}

void UAbleCollisionShapeBox::BindDynamicDelegates(class UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_HalfExtents, "Half Extents");
}

#if WITH_EDITOR

const FString UAbleCollisionShapeBox::DescribeShape() const
{
	return FString::Printf(TEXT("Box %.1fm x %.1fm x%.1fm"), m_HalfExtents.X * 2.0f * 0.01f, m_HalfExtents.Y * 2.0f * 0.01f, m_HalfExtents.Z * 2.0f * 0.01f);
}

void UAbleCollisionShapeBox::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    AActor* SourceActor = m_QueryLocation.GetSourceActor(Context);
    if (SourceActor != nullptr)
    {
        FTransform QueryTransform;
        m_QueryLocation.GetTransform(Context, QueryTransform);

    	// Push our query out by our half extents so we aren't centered in the box.
    	const FQuat Rotation=QueryTransform.GetRotation();
    	const FVector HalfExtentsOffset=Rotation.GetForwardVector()*m_HalfExtents.X;
    	QueryTransform*=FTransform(HalfExtentsOffset);

        FVector AlignedBox = QueryTransform.GetRotation().GetForwardVector() * m_HalfExtents.X;
        AlignedBox += QueryTransform.GetRotation().GetRightVector() * m_HalfExtents.Y;
        AlignedBox += QueryTransform.GetRotation().GetUpVector() * m_HalfExtents.Z;
        FAbleAbilityDebug::DrawBoxQuery(Context.GetWorld(), QueryTransform, m_HalfExtents);
    }
}

EDataValidationResult UAbleCollisionShapeBox::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAbleCollisionShapeSphere::UAbleCollisionShapeSphere(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Radius(50.0f)
{

}

UAbleCollisionShapeSphere::~UAbleCollisionShapeSphere()
{

}

FTransform UAbleCollisionShapeSphere::DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform, bool bOverrideTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
		return FTransform::Identity;
	}

	UWorld* World = SourceActor->GetWorld();

	FTransform QueryTransform;
	if (bOverrideTransform)
	{
		QueryTransform = ReplaceTransform;
	}
	else
	{
		QueryLocation.GetTransform(*Context.Get(), QueryTransform);	
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
        ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	TArray<FOverlapResult> OverlapResults;
	if (World->OverlapMultiByObjectType(OverlapResults, QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, Sphere))
	{
		for (FOverlapResult& Result : OverlapResults)
		{
			OutResults.Add(FAbleQueryResult(Result));
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawSphereQuery(World, QueryTransform, Radius);
	}
#endif
	return QueryTransform;
}

FTraceHandle UAbleCollisionShapeSphere::DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	UWorld* World = SourceActor->GetWorld();

	QueryLocation.GetTransform(*Context.Get(), OutQueryTransform);

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawSphereQuery(World, OutQueryTransform, Radius);
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncOverlapByObjectType(OutQueryTransform.GetLocation(), OutQueryTransform.GetRotation(), ObjectQuery, Sphere);
}

void UAbleCollisionShapeSphere::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
}

#if WITH_EDITOR

const FString UAbleCollisionShapeSphere::DescribeShape() const
{
	return FString::Printf(TEXT("Sphere %.1fm"), m_Radius * 2.0f * 0.01f);
}

void UAbleCollisionShapeSphere::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    AActor* SourceActor = m_QueryLocation.GetSourceActor(Context);
    if (SourceActor != nullptr)
    {
        FTransform QueryTransform;
        m_QueryLocation.GetTransform(Context, QueryTransform);

        FAbleAbilityDebug::DrawSphereQuery(Context.GetWorld(), QueryTransform, m_Radius);
    }
}

EDataValidationResult UAbleCollisionShapeSphere::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAbleCollisionShapeCapsule::UAbleCollisionShapeCapsule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Height(100.0f),
	m_Radius(50.0f)
{

}

UAbleCollisionShapeCapsule::~UAbleCollisionShapeCapsule()
{

}

FTransform UAbleCollisionShapeCapsule::DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform, bool bOverrideTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
		return FTransform::Identity;
	}

	UWorld* World = SourceActor->GetWorld();

	FTransform QueryTransform;
	if (bOverrideTransform)
	{
		QueryTransform = ReplaceTransform;
	}
	else
	{
		QueryLocation.GetTransform(*Context.Get(), QueryTransform);
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FCollisionShape Capsule = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	TArray<FOverlapResult> OverlapResults;
	if (World->OverlapMultiByObjectType(OverlapResults, QueryTransform.GetLocation(), QueryTransform.GetRotation(), ObjectQuery, Capsule))
	{
		for (FOverlapResult& Result : OverlapResults)
		{
			OutResults.Add(FAbleQueryResult(Result));
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawCapsuleQuery(World, QueryTransform, Radius, Height);
	}
#endif
	return QueryTransform;
}

FTraceHandle UAbleCollisionShapeCapsule::DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);
	
	AActor* SourceActor = QueryLocation.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	UWorld* World = SourceActor->GetWorld();

	QueryLocation.GetTransform(*Context.Get(), OutQueryTransform);

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FCollisionShape Capsule = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawCapsuleQuery(World, OutQueryTransform, Radius, Height);
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncOverlapByObjectType(OutQueryTransform.GetLocation(), OutQueryTransform.GetRotation(), ObjectQuery, Capsule);
}

void UAbleCollisionShapeCapsule::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Height, "Height");
}

#if WITH_EDITOR

const FString UAbleCollisionShapeCapsule::DescribeShape() const
{
	return FString::Printf(TEXT("Capsule %.1fm x %.1fm"), m_Height * 0.01f, m_Radius * 2.0f * 0.01f);
}

void UAbleCollisionShapeCapsule::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    AActor* SourceActor = m_QueryLocation.GetSourceActor(Context);
    if (SourceActor != nullptr)
    {
        FTransform QueryTransform;
        m_QueryLocation.GetTransform(Context, QueryTransform);

        FAbleAbilityDebug::DrawCapsuleQuery(Context.GetWorld(), QueryTransform, m_Radius, m_Height);
    }
}

EDataValidationResult UAbleCollisionShapeCapsule::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAbleCollisionShapeCone::UAbleCollisionShapeCone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_FOV(90.0f),
	m_Length(100.0f),
	m_Height(50.0f),
	m_Is2DQuery(false),
	m_3DSlice(false)
{

}

UAbleCollisionShapeCone::~UAbleCollisionShapeCone()
{

}

FTransform UAbleCollisionShapeCone::DoQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, TArray<FAbleQueryResult>& OutResults, const FTransform& ReplaceTransform, bool bOverrideTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocationTarget = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocationTarget.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
		return FTransform::Identity;
	}

	UWorld* World = SourceActor->GetWorld();

	float FOV = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_FOV);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);
	float Length = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Length);

	const float Radius = (m_Is2DQuery ? Length : FMath::Max(Height, Length)) * 0.5f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(FOV < 180.0f ? Radius : Radius * 2.0f);
	
	FTransform QueryTransform;
	if (bOverrideTransform)
	{
		QueryTransform = ReplaceTransform;
	}
	else
	{
		QueryLocationTarget.GetTransform(*Context.Get(), QueryTransform);
	}

	const FVector QueryForward = QueryTransform.GetRotation().GetForwardVector();

	// Center our position around our Radius, unless we're great than 180, then we need center at our origin and increase our query size.
	const FVector OffsetVector = QueryForward * Radius;
	const FVector QueryLocation = FOV > 180.0f ? QueryTransform.GetTranslation() : QueryTransform.GetTranslation() + OffsetVector;

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	TArray<FOverlapResult> OverlapResults;
	if (World->OverlapMultiByObjectType(OverlapResults, QueryLocation, QueryTransform.GetRotation(), ObjectQuery, SphereShape))
	{
		// Do our actual cone logic, just use the Async method to keep the logic in one place.
		QueryTransform.SetScale3D(FVector(FOV, Height, Length)); // Store these values in the scale, this is safe because we don't use Scale anyway.
		ProcessAsyncOverlaps(Context, QueryTransform, OverlapResults, OutResults);
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		if (m_Is2DQuery)
		{
			FAbleAbilityDebug::Draw2DConeQuery(World, QueryTransform, FOV, Length);
		}
		else
		{
			if (m_3DSlice)
			{
				FAbleAbilityDebug::DrawConeSliceQuery(World, QueryTransform, FOV, Length, Height);
			}
			else
			{
				FAbleAbilityDebug::DrawConeQuery(World, QueryTransform, FOV, Length, Height);
			}
		}
	}
#endif
	return QueryTransform;
}

FTraceHandle UAbleCollisionShapeCone::DoAsyncQuery(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutQueryTransform) const
{
	FAbleAbilityTargetTypeLocation QueryLocationTarget = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_QueryLocation);

	AActor* SourceActor = QueryLocationTarget.GetSourceActor(*Context.Get());
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	UWorld* World = SourceActor->GetWorld();

	float FOV = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_FOV);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);
	float Length = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Length);

	const float Radius = (m_Is2DQuery ? Length : FMath::Max(Height, Length)) * 0.5f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(FOV < 180.0f ? Radius : Radius * 2.0f);

	QueryLocationTarget.GetTransform(*Context.Get(), OutQueryTransform);

	const FVector QueryForward = OutQueryTransform.GetRotation().GetForwardVector();

	// Center our position around our Radius, unless we're great than 180, then we need center at our origin and increase our query size.
	const FVector OffsetVector = QueryForward * Radius;
	const FVector QueryLocation = FOV > 180.0f ? OutQueryTransform.GetTranslation() : OutQueryTransform.GetTranslation() + OffsetVector;

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		if (m_Is2DQuery)
		{
			FAbleAbilityDebug::Draw2DConeQuery(World, OutQueryTransform, FOV, Length);
		}
		else
		{
			if (m_3DSlice)
			{
				FAbleAbilityDebug::DrawConeSliceQuery(World, OutQueryTransform, FOV, Length, Height);
			}
			else
			{
				FAbleAbilityDebug::DrawConeQuery(World, OutQueryTransform, FOV, Length, Height);
			}
		}
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}
	OutQueryTransform.SetScale3D(FVector(FOV, Height, Length)); // Store these values in the scale, this is safe because we don't use Scale anyway.
    return World->AsyncOverlapByObjectType(QueryLocation, OutQueryTransform.GetRotation(), ObjectQuery, SphereShape);
}

void UAbleCollisionShapeCone::ProcessAsyncOverlaps(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& QueryTransform, const TArray<FOverlapResult>& Overlaps, TArray<FAbleQueryResult>& OutResults) const
{
	/* ===== Source cone collider code ===== */
	//float FOV = QueryTransform.GetScale3D().X;
	//float Height = QueryTransform.GetScale3D().Y;
	//float Length = QueryTransform.GetScale3D().Z;

	//// Cache off various values we use in our cone test.
	//const bool GreaterThanOneEighty = FOV > 180.0f;
	//const float QueryAngle = GreaterThanOneEighty ? 360.0f - FOV : FOV;
	//const float HalfAngle = FMath::DegreesToRadians(QueryAngle * 0.5f);
	//const float LengthSqr = Length * Length;
	//const float HeightSqr = Height * Height;
	//const float HeightAngle = m_Is2DQuery ? 0.0f : FMath::Atan2(Height * 0.5f, Length);

	//FVector QueryForward = QueryTransform.GetRotation().GetForwardVector();

	//// If we're great than 180, we take the angle of the "hole" and compare against that (which requires flipping our forward around).
	//if (GreaterThanOneEighty)
	//{
	//	// Flip it around.
	//	QueryForward = -QueryForward;
	//}

	//// Store all our Vectors to test against.
	//const FVector QueryRight = QueryTransform.GetRotation().GetRightVector();
	//const FVector QueryLocation = QueryTransform.GetTranslation();
	//const FVector2D XYForward(QueryForward.X, QueryForward.Y); // Horiz Plane
	//const FVector2D XZForward(QueryForward.X, QueryForward.Z); // Vert Plane

 //   // Various Parameters that will be written each check, declared here so we don't thrash the cache. 
	//FTransform ResultTransform; // Our Overlap Result Transform.
	//FVector ResultLocation; // Our Overlap Result Location (from our Transform).

	//FVector ToTarget; // Vector from our Query Location to our Result. 
	//FVector2D ToTargetXZ; // 2D Vector (XZ Plane) from our Query Location to our Result.
	//FVector2D ToTargetXY; // 2D Vector (XY Plane) from our Query Location to our Result.

	//bool ValidEntry = false; // Whether we've passed all checks or not.
	//bool WithInAngle = false; // Whether we're within our Cone Angle 
	//bool HalfSpace = false; // Half space check.

	//float QueryToTargetDotProduct = 0.0f; // Dot product of our XYForward and ToTargetXY.
	//float VerticalAngle = 0.0f;  // Angle, in radians, of our XZForward and ToTarget XZ.

	//for (const FOverlapResult& Result : Overlaps)
	//{
	//	FAbleQueryResult TempTarget(Result);

	//	TempTarget.GetTransform(ResultTransform);

	//	ResultLocation = ResultTransform.GetTranslation();

	//	ToTarget = ResultLocation - QueryLocation;
	//	ToTarget.Normalize();

	//	ToTargetXZ.Set(ToTarget.X, ToTarget.Z);
	//	ToTargetXY.Set(ToTarget.X, ToTarget.Y);

	//	ValidEntry = true;

	//	// If we're a 3D query, we base our initial sphere query on whichever is largest (height or length),
	//	// so do a quick distance check here, if those pass - go ahead and do our vertical angle check.
	//	if (!m_Is2DQuery)
	//	{
	//		/*if (FVector::DistSquared(ResultLocation, QueryLocation) > HeightSqr ||
	//			FVector::DistSquared(ResultLocation, QueryLocation) > LengthSqr)
	//		{
	//			ValidEntry = false;
	//		}
	//		else
	//		{
	//			if (m_3DSlice)
	//			{
	//				ValidEntry = FVector::DistSquared(ResultLocation, QueryLocation) < HeightSqr;
	//			}
	//			else
	//			{
	//				VerticalAngle = FVector::DotProduct(QueryForward, ToTarget);

	//				ValidEntry = VerticalAngle < HeightAngle;
	//			}
	//		}*/

	//		ValidEntry=FVector::DotProduct(QueryForward, ToTarget) > 0 && FVector::DistSquared2D(ResultLocation, QueryLocation) <= LengthSqr;
	//	}
	//	else
	//	{
	//		ValidEntry = FVector::DistSquared2D(ResultLocation, QueryLocation) <= LengthSqr;
	//	}

	//	// Move on to our Dot product checks
	//	//if (ValidEntry)
	//	//{
	//	//	// Check our Horizontal angle.
	//	//	QueryToTargetDotProduct = FVector2D::DotProduct(XYForward, ToTargetXY);

	//	//	WithInAngle = FMath::Acos(QueryToTargetDotProduct) < HalfAngle;

	//	//	HalfSpace = QueryToTargetDotProduct > 0.0f;

	//	//	ValidEntry = WithInAngle && HalfSpace;

	//	//	if (GreaterThanOneEighty) // If our FOV > 180 degrees, we want everything not in the angle check.
	//	//	{
	//	//		ValidEntry = !ValidEntry;
	//	//	}
	//	//}

	//	// Save our success
	//	if (ValidEntry)
	//	{
	//		OutResults.Add(TempTarget);
	//	}
	//}
	/* ===== ~End Source cone collider code ===== */

	float FOV = QueryTransform.GetScale3D().X;
	float Height = QueryTransform.GetScale3D().Y;
	float Length = QueryTransform.GetScale3D().Z;

	// Cache off various values we use in our cone test.
	const bool GreaterThanOneEighty = FOV > 180.0f;
	const float QueryAngle = GreaterThanOneEighty ? 360.0f - FOV : FOV;
	const float HalfAngle = FMath::DegreesToRadians(QueryAngle * 0.5f);
	const float LengthSqr = Length * Length;
	const float HeightSqr = Height * Height;
	const float HeightAngle = m_Is2DQuery ? 0.0f : FMath::Atan2(Height * 0.5f, Length);

	FVector QueryForward = QueryTransform.GetRotation().GetForwardVector();

	// If we're great than 180, we take the angle of the "hole" and compare against that (which requires flipping our forward around).
	if (GreaterThanOneEighty)
	{
		// Flip it around.
		QueryForward = -QueryForward;
	}

	// Store all our Vectors to test against.
	const FVector QueryRight = QueryTransform.GetRotation().GetRightVector();
	const FVector QueryLocation = QueryTransform.GetTranslation();
	const FVector2D XYForward(QueryForward.X, QueryForward.Y); // Horiz Plane
	const FVector2D XZForward(QueryForward.X, QueryForward.Z); // Vert Plane

	// Various Parameters that will be written each check, declared here so we don't thrash the cache. 
	FTransform ResultTransform; // Our Overlap Result Transform.
	FVector ResultLocation; // Our Overlap Result Location (from our Transform).

	FVector ToTarget; // Vector from our Query Location to our Result. 
	FVector2D ToTargetXZ; // 2D Vector (XZ Plane) from our Query Location to our Result.
	FVector2D ToTargetXY; // 2D Vector (XY Plane) from our Query Location to our Result.

	bool ValidEntry = false; // Whether we've passed all checks or not.
	bool WithInAngle = false; // Whether we're within our Cone Angle 
	bool bInRange = false;
	//bool HalfSpace = false; // Half space check.

	float QueryToTargetDotProduct = 0.0f; // Dot product of our XYForward and ToTargetXY.
	float VerticalAngle = 0.0f;  // Angle, in radians, of our XZForward and ToTarget XZ.

	for (const FOverlapResult& Result : Overlaps)
	{
		FAbleQueryResult TempTarget(Result);

		TempTarget.GetTransform(ResultTransform);

		ResultLocation = ResultTransform.GetTranslation();

		ToTarget = ResultLocation - QueryLocation;
		ToTarget.Normalize();

		ToTargetXZ.Set(ToTarget.X, ToTarget.Z);
		ToTargetXY.Set(ToTarget.X, ToTarget.Y);

		ValidEntry = true;

		bInRange = FVector::DistSquared2D(ResultLocation, QueryLocation) <= LengthSqr;

		// If we're a 3D query, we base our initial sphere query on whichever is largest (height or length),
		// so do a quick distance check here, if those pass - go ahead and do our vertical angle check.
		if (!m_Is2DQuery)
		{
			WithInAngle = FVector2D::DotProduct(XYForward, ToTargetXY) > 0 && FVector2D::DotProduct(XZForward, ToTargetXZ) > 0;
		}
		else
		{
			WithInAngle = FVector::DotProduct(QueryForward, ToTarget) > 0;
		}
		ValidEntry = WithInAngle && bInRange;

		// Save our success
		if (ValidEntry)
		{
			OutResults.Add(TempTarget);
		}
	}
}

void UAbleCollisionShapeCone::BindDynamicDelegates(class UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_FOV, "FOV");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Height, "Height");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Length, "Length");
}

#if WITH_EDITOR

const FString UAbleCollisionShapeCone::DescribeShape() const
{
	if (m_Is2DQuery)
	{
		return FString::Printf(TEXT("Cone %.1fdeg %.1fm"), m_FOV, m_Length * 0.01f);
	}
	
	return FString::Printf(TEXT("Cone %.1fdeg %.1fm x %.1fm"), m_FOV, m_Length * 0.01f, m_Height * 0.01f);
}

void UAbleCollisionShapeCone::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    AActor* SourceActor = m_QueryLocation.GetSourceActor(Context);
    if (SourceActor != nullptr)
    {
        FTransform QueryTransform;
        m_QueryLocation.GetTransform(Context, QueryTransform);

        if (m_Is2DQuery)
        {
            FAbleAbilityDebug::Draw2DConeQuery(Context.GetWorld(), QueryTransform, m_FOV, m_Length);
        }
        else
        {
			if (m_3DSlice)
			{
				FAbleAbilityDebug::DrawConeSliceQuery(Context.GetWorld(), QueryTransform, m_FOV, m_Length, m_Height);
			}
			else
			{
				FAbleAbilityDebug::DrawConeQuery(Context.GetWorld(), QueryTransform, m_FOV, m_Length, m_Height);
			}
        }
    }
}

EDataValidationResult UAbleCollisionShapeCone::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_ChannelPresent == ACP_NonePresent && m_CollisionChannels.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoCollisionChannels", "No Collision Channels: {0}, make sure you move over your old setting to the new array."), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE