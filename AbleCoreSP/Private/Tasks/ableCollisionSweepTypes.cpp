// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCollisionSweepTypes.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityDebug.h"
#include "AbleCoreSPPrivate.h"
#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCollisionSweepShape::UAbleCollisionSweepShape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), m_ChannelPresent(ACP_Default), m_OnlyReturnBlockingHit(false),
	  m_UseAsyncQuery(false)
{
}

UAbleCollisionSweepShape::~UAbleCollisionSweepShape()
{

}

void UAbleCollisionSweepShape::GetAsyncResults(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTraceHandle& Handle, TArray<FAbleQueryResult>& OutResults) const
{
	check(Context.IsValid());
	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = m_SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}


	UWorld* World = SourceActor->GetWorld();
	check(World);

	FTraceDatum Datum;
	if (World->QueryTraceData(Handle, Datum))
	{
		for (const FHitResult& HitResult : Datum.OutHits)
		{
			OutResults.Add(FAbleQueryResult(HitResult));
		}
	}
}

void UAbleCollisionSweepShape::GetQueryTransform(const TWeakObjectPtr<const UAbleAbilityContext>& Context, FTransform& OutTransform) const
{
	check(Context.IsValid());
	m_SweepLocation.GetTransform(*Context.Get(), OutTransform);
}

FName UAbleCollisionSweepShape::GetDynamicDelegateName(const FString& PropertyName) const
{
	FString DelegateName = TEXT("OnGetDynamicProperty_Sweep_") + PropertyName;
	const FString& DynamicIdentifier = GetDynamicPropertyIdentifier();
	if (!DynamicIdentifier.IsEmpty())
	{
		DelegateName += TEXT("_") + DynamicIdentifier;
	}

	return FName(*DelegateName);
}

void UAbleCollisionSweepShape::BindDynamicDelegates(class UAbleAbility* Ability)
{
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SweepLocation, TEXT("Sweep Location"));
}

#if WITH_EDITOR
EDataValidationResult UAbleCollisionSweepShape::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    if (m_ChannelPresent == ACP_NonePresent && m_CollisionChannels.Num() == 0)
    {
        ValidationErrors.Add(FText::Format(LOCTEXT("NoCollisionChannels", "No Collision Channels: {0}, make sure you move over your old setting to the new array."), AssetName));
        result = EDataValidationResult::Invalid;
    }

    return result;
}

bool UAbleCollisionSweepShape::FixUpObjectFlags()
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

UAbleCollisionSweepBox::UAbleCollisionSweepBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_HalfExtents(50.0f, 50.0f, 50.0f)
{

}

UAbleCollisionSweepBox::~UAbleCollisionSweepBox()
{

}

void UAbleCollisionSweepBox::DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);

	FTransform StartTransform, EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	FQuat SourceRotation = SourceTransform.GetRotation();
	FVector StartOffset = SourceRotation.GetForwardVector() * HalfExtents.X;

	StartTransform = SourceTransform * FTransform(StartOffset);

	FQuat EndRotation = EndTransform.GetRotation();
	FVector EndOffset = EndRotation.GetForwardVector() * HalfExtents.X;

	EndTransform *= FTransform(EndOffset);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, StartTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			OutResults.Add(FAbleQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, StartTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAbleQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += StartTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += StartTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAbleAbilityDebug::DrawBoxSweep(World, StartTransform, EndTransform, AlignedBox);
	}
#endif
}

FTraceHandle UAbleCollisionSweepBox::DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	FVector HalfExtents = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_HalfExtents);

	FTransform StartTransform, EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	FQuat SourceRotation = SourceTransform.GetRotation();
	FVector StartOffset = SourceRotation.GetForwardVector() * HalfExtents.X;

	StartTransform = SourceTransform * FTransform(StartOffset);

	FQuat EndRotation = EndTransform.GetRotation();
	FVector EndOffset = EndRotation.GetForwardVector() * HalfExtents.X;

	EndTransform *= FTransform(EndOffset);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * HalfExtents.X;
		AlignedBox += StartTransform.GetRotation().GetRightVector() * HalfExtents.Y;
		AlignedBox += StartTransform.GetRotation().GetUpVector() * HalfExtents.Z;

		FAbleAbilityDebug::DrawBoxSweep(World, StartTransform, EndTransform, AlignedBox);
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, StartTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAbleCollisionSweepBox::BindDynamicDelegates(class UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_HalfExtents, "Half Extents");
}

#if WITH_EDITOR

const FString UAbleCollisionSweepBox::DescribeShape() const
{
	return FString::Printf(TEXT("Box %.1fm x %.1fm x%.1fm"), m_HalfExtents.X * 2.0f * 0.01f, m_HalfExtents.Y * 2.0f * 0.01f, m_HalfExtents.Z * 2.0f * 0.01f);
}

void UAbleCollisionSweepBox::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FVector AlignedBox = StartTransform.GetRotation().GetForwardVector() * m_HalfExtents.X;
    AlignedBox += StartTransform.GetRotation().GetRightVector() * m_HalfExtents.Y;
    AlignedBox += StartTransform.GetRotation().GetUpVector() * m_HalfExtents.Z;

    FAbleAbilityDebug::DrawBoxSweep(Context.GetWorld(), StartTransform, EndTransform, AlignedBox);
}

EDataValidationResult UAbleCollisionSweepBox::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAbleCollisionSweepSphere::UAbleCollisionSweepSphere(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_Radius(50.0f)
{

}

UAbleCollisionSweepSphere::~UAbleCollisionSweepSphere()
{

}

void UAbleCollisionSweepSphere::DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape))
		{
			OutResults.Add(FAbleQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAbleQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawSphereSweep(World, SourceTransform, EndTransform, Radius);
	}
#endif

}

FTraceHandle UAbleCollisionSweepSphere::DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawSphereSweep(World, SourceTransform, EndTransform, Radius);
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAbleCollisionSweepSphere::BindDynamicDelegates(class UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
}

#if WITH_EDITOR

const FString UAbleCollisionSweepSphere::DescribeShape() const
{
	return FString::Printf(TEXT("Sphere %.1fm"), m_Radius * 2.0f * 0.01f);
}

void UAbleCollisionSweepSphere::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FAbleAbilityDebug::DrawSphereSweep(Context.GetWorld(), StartTransform, EndTransform, m_Radius);
}

EDataValidationResult UAbleCollisionSweepSphere::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

UAbleCollisionSweepCapsule::UAbleCollisionSweepCapsule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_Radius(25.0f),
	m_Height(100.0f)
{

}

UAbleCollisionSweepCapsule::~UAbleCollisionSweepCapsule()
{

}

void UAbleCollisionSweepCapsule::DoSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform, TArray<FAbleQueryResult>& OutResults) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return;
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);
	if (m_OnlyReturnBlockingHit)
	{
		FHitResult SweepResult;
		if (World->SweepSingleByObjectType(SweepResult, SourceTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			OutResults.Add(FAbleQueryResult(SweepResult));
		}
	}
	else
	{
		TArray<FHitResult> SweepResults;
		if (World->SweepMultiByObjectType(SweepResults, SourceTransform.GetLocation(), EndTransform.GetLocation(), SourceTransform.GetRotation(), ObjectQuery, Shape))
		{
			for (const FHitResult& SweepResult : SweepResults)
			{
				OutResults.Add(FAbleQueryResult(SweepResult));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawCapsuleSweep(World, SourceTransform, EndTransform, Radius, Height);
	}
#endif

}

FTraceHandle UAbleCollisionSweepCapsule::DoAsyncSweep(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const FTransform& SourceTransform) const
{
	check(Context.IsValid());
	FAbleAbilityTargetTypeLocation SweepLocation = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SweepLocation);

	const UAbleAbilityContext& ConstContext = *Context.Get();
	AActor* SourceActor = SweepLocation.GetSourceActor(ConstContext);
	if (!SourceActor)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("Source Actor for Target was null! Skipping query."));
        return FTraceHandle();
	}

	float Radius = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Radius);
	float Height = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_Height);

	FTransform EndTransform;
	SweepLocation.GetTransform(ConstContext, EndTransform);

	UWorld* World = SourceActor->GetWorld();
	check(World);

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, Height * 0.5f);

#if !UE_BUILD_SHIPPING
	if (FAbleAbilityDebug::ShouldDrawQueries())
	{
		FAbleAbilityDebug::DrawCapsuleSweep(World, SourceTransform, EndTransform, Radius, Height);
	}
#endif

	FCollisionObjectQueryParams ObjectQuery;
	const TArray<TEnumAsByte<ECollisionChannel>> Channels = UAbleAbilityBlueprintLibrary::GetCollisionChannelPresent(
		Context.Get(), m_ChannelPresent, m_CollisionChannels);
    for (TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
    	ObjectQuery.AddObjectTypesToQuery(Channel.GetValue());
	}

	return World->AsyncSweepByObjectType(m_OnlyReturnBlockingHit ? EAsyncTraceType::Single : EAsyncTraceType::Multi, SourceTransform.GetLocation(), EndTransform.GetLocation(), FQuat::Identity, ObjectQuery, Shape);
}

void UAbleCollisionSweepCapsule::BindDynamicDelegates(class UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Radius, "Radius");
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_Height, "Height");
}

#if WITH_EDITOR

const FString UAbleCollisionSweepCapsule::DescribeShape() const
{
	return FString::Printf(TEXT("Capsule %.1fm x %.1fm"), m_Height * 0.01f, m_Radius * 2.0f * 0.01f);
}

void UAbleCollisionSweepCapsule::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	// Draw a Preview.
    FTransform StartTransform, EndTransform;
    m_SweepLocation.GetTransform(Context, StartTransform);
    m_SweepLocation.GetTransform(Context, EndTransform);

    FAbleAbilityDebug::DrawCapsuleSweep(Context.GetWorld(), StartTransform, EndTransform, m_Radius, m_Height);
}

EDataValidationResult UAbleCollisionSweepCapsule::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = Super::IsTaskDataValid(AbilityContext, AssetName, ValidationErrors);

    return result;
}

#endif

#undef LOCTEXT_NAMESPACE