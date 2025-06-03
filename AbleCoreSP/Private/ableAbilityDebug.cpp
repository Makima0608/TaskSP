// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityDebug.h"

#include "ableAbilityBlueprintLibrary.h"
#include "Engine/World.h"

#include "DrawDebugHelpers.h"

#if !UE_BUILD_SHIPPING

static TAutoConsoleVariable<int32> CVarDrawDebugQueryShapes(TEXT("Able.DrawQueryDebug"), 0, TEXT("1 to enable drawing of debug query volumes for abilities."));
static TAutoConsoleVariable<float> CVarDrawDebugQueryShapesLifetime(TEXT("Able.QueryDebugShapeLifetime"), 0.25f, TEXT("How long, in seconds, a query debug shape should be visible."));
bool FAbleAbilityDebug::ForceDrawQueries = false;
bool FAbleAbilityDebug::DrawInEditor = false;
bool FAbleAbilityDebug::VerboseDebug = false;

void FAbleAbilityDebug::EnableDrawQueries(bool Enable)
{
	ForceDrawQueries = Enable;
}

void FAbleAbilityDebug::EnableDrawInEditor(bool Enable)
{
	DrawInEditor = Enable;
}

void FAbleAbilityDebug::EnableVerboseDebug(bool Enable)
{
	VerboseDebug = Enable;
}

bool FAbleAbilityDebug::ShouldDrawQueries()
{
	return ForceDrawQueries || IsVerboseDebug() || CVarDrawDebugQueryShapes.GetValueOnAnyThread() != 0;
}

bool FAbleAbilityDebug::IsVerboseDebug()
{
	return VerboseDebug;
}

bool FAbleAbilityDebug::ShouldDrawInEditor()
{
	return DrawInEditor;
}

float FAbleAbilityDebug::GetDebugQueryLifetime()
{
	return CVarDrawDebugQueryShapesLifetime.GetValueOnAnyThread();
}

void FAbleAbilityDebug::SetDebugQueryLifetime(float lifetime)
{
	CVarDrawDebugQueryShapesLifetime.AsVariable()->Set(lifetime);
}

void FAbleAbilityDebug::DrawSphereQuery(const UWorld* World, const FTransform& QueryTransform, float Radius)
{
	DrawDebugSphere(World, QueryTransform.GetLocation(), Radius, 32, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawBoxQuery(const UWorld* World, const FTransform& QueryTransform, const FVector& HalfExtents)
{
	DrawDebugBox(World, QueryTransform.GetLocation(), HalfExtents, QueryTransform.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::Draw2DConeQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length)
{
	const FVector Forward = QueryTransform.GetRotation().GetForwardVector();
	const FVector Up = QueryTransform.GetRotation().GetUpVector();

	int32 Segments = FOV > 180.0f ? 32 : 16; // Double our segments if we're doing a larger circle.
	const float FOVRadians = FMath::DegreesToRadians(FOV);
	const float RadianStep = FOVRadians / (float)Segments;
	float CurrentRadians = -FOVRadians * 0.5f;

	// First and last lines are special and close our shape.
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	FVector LineStart, LineEnd;
	for (int32 i = 0; i < Segments; ++i)
	{
		LineStart = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		LineEnd = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		CurrentRadians += RadianStep;
	}
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawConeQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length, float Height)
{
	const FVector Forward = QueryTransform.GetRotation().GetForwardVector();
	const FVector Up = QueryTransform.GetRotation().GetUpVector();

	int32 Segments = FOV > 180.0f ? 32 : 16; // Double our segments if we're doing a larger circle.
	const float FOVRadians = FMath::DegreesToRadians(FOV);
	const float RadianStep = FOVRadians / (float)Segments;
	float CurrentRadians = -FOVRadians * 0.5f;
	
	int32 HeightSegments = 6;
	const float HeightStep = Height / (float)HeightSegments;
	float CurrentHeight = -(Height * 0.5f);

	// First and last lines are special and close our shape.
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());

	// Draw our Height Segments for this position.
	FVector CurrentHorizPt = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
	for (int32 j = 0; j < HeightSegments; ++j)
	{
		DrawDebugLine(World, QueryTransform.GetLocation(), CurrentHorizPt + (Up * CurrentHeight), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		if (j < HeightSegments - 1)
		{
			// Vertical Caps.
			DrawDebugLine(World, CurrentHorizPt + (Up * CurrentHeight), CurrentHorizPt + (Up * (CurrentHeight + HeightStep)), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		}
		CurrentHeight += HeightStep;
	}
	
	FVector LineStart, LineEnd;
	for (int32 i = 0; i < Segments; ++i)
	{
		LineStart = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		LineEnd = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		CurrentRadians += RadianStep;

		CurrentHeight = -(Height * 0.5f);
		for (int32 j = 0; j < HeightSegments; ++j)
		{
			DrawDebugLine(World, QueryTransform.GetLocation(), LineStart + (Up * CurrentHeight), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
			DrawDebugLine(World, QueryTransform.GetLocation(), LineEnd + (Up * CurrentHeight), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());

			// Horizontal Cap at height
			DrawDebugLine(World, LineStart + (Up * CurrentHeight), LineEnd + (Up * CurrentHeight), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
			if (j < HeightSegments - 1)
			{
				// Vertical Caps.
				DrawDebugLine(World, LineStart + (Up * CurrentHeight), LineStart + (Up * (CurrentHeight + HeightStep)), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
				DrawDebugLine(World, LineEnd + (Up * CurrentHeight), LineEnd + (Up * (CurrentHeight + HeightStep)), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
			}
			CurrentHeight += HeightStep;
		}

	}
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	
	// Draw our Height Segments for this position.
	CurrentHorizPt = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
	CurrentHeight = -(Height * 0.5f);
	for (int32 j = 0; j < HeightSegments; ++j)
	{
		DrawDebugLine(World, QueryTransform.GetLocation(), CurrentHorizPt + (Up * CurrentHeight), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		if (j < HeightSegments - 1)
		{
			// Vertical Caps.
			DrawDebugLine(World, CurrentHorizPt + (Up * CurrentHeight), CurrentHorizPt + (Up * (CurrentHeight + HeightStep)), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
		}
		CurrentHeight += HeightStep;
	}
}

void FAbleAbilityDebug::DrawConeSliceQuery(const UWorld* World, const FTransform& QueryTransform, float FOV, float Length, float Height, float Duration, FLinearColor DebugColor)
{
	const FVector Forward = QueryTransform.GetRotation().GetForwardVector();
	const FVector Up = QueryTransform.GetRotation().GetUpVector();

	int32 Segments = FOV > 180.0f ? 32 : 16; // Double our segments if we're doing a larger circle.
	const float FOVRadians = FMath::DegreesToRadians(FOV);
	const float RadianStep = FOVRadians / (float)Segments;
	float CurrentRadians = -FOVRadians * 0.5f;

	FVector TopLocation = QueryTransform.GetLocation() + Up * Height;

	// Draw the Bottom of the shape.
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugLine(World, TopLocation, TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

	// Connections
	DrawDebugLine(World, QueryTransform.GetLocation(), TopLocation, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugLine(World, QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

	FVector LineStart, LineEnd;
	for (int32 i = 0; i < Segments; ++i)
	{
		// Bottom
		LineStart = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		LineEnd = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

		// Top
		LineStart = TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		LineEnd = TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

		// Connection A
		LineStart = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		LineEnd = TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

		// Connection B
		LineStart = QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		LineEnd = TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians + RadianStep), Up) * Length;
		DrawDebugLine(World, LineStart, LineEnd, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

		CurrentRadians += RadianStep;
	}
	DrawDebugLine(World, QueryTransform.GetLocation(), QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugLine(World, TopLocation, TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());

	DrawDebugLine(World, QueryTransform.GetLocation(), TopLocation, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugLine(World, QueryTransform.GetLocation() + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, TopLocation + Forward.RotateAngleAxis(FMath::RadiansToDegrees(CurrentRadians), Up) * Length, DebugColor.ToFColor(true), ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawConeQueryWithRadius(const UWorld* World, const FTransform& QueryTransform, float Radius, float Length)
{
	FVector QueryLocation = QueryTransform.GetLocation();
	FVector QueryForward = QueryTransform.GetRotation().GetForwardVector();
	float HalfAngle = FMath::Atan2(Radius, Length);
	float SlantHeight = FMath::Sqrt(Radius * Radius + Length * Length);

	const int32 NumSides = 32;
	const float DebugQueryLifetime = GetDebugQueryLifetime();

	DrawDebugCone(World, QueryLocation, QueryForward, SlantHeight, HalfAngle, HalfAngle, NumSides, FColor::Red, true, DebugQueryLifetime);
}

void FAbleAbilityDebug::DrawRaycastQuery(const UWorld* World, const FVector& QueryStart, const FVector& QueryEnd)
{
	DrawDebugLine(World, QueryStart, QueryEnd, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawCapsuleQuery(const UWorld* World, const FTransform& QueryTransform, float Radius, float Height)
{
	const float HalfHeight = Height * 0.5f;
	DrawDebugCapsule(World, QueryTransform.GetLocation(), HalfHeight, Radius, QueryTransform.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawBoxSweep(const UWorld* World, const FTransform& Start, const FTransform& End, const FVector& HalfExtents)
{
	DrawDebugBox(World, Start.GetLocation(), HalfExtents, Start.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugDirectionalArrow(World, Start.GetLocation(), End.GetLocation(), 50.0f, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugBox(World, End.GetLocation(), HalfExtents, Start.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawSphereSweep(const UWorld* World, const FTransform& Start, const FTransform& End, float Radius)
{
	DrawDebugSphere(World, Start.GetLocation(), Radius, 32, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugDirectionalArrow(World, Start.GetLocation(), End.GetLocation(), 50.0f, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugSphere(World, End.GetLocation(), Radius, 32, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

void FAbleAbilityDebug::DrawCapsuleSweep(const UWorld* World, const FTransform& Start, const FTransform& End, float Radius, float Height)
{
	DrawDebugCapsule(World, Start.GetLocation(), Height * 0.5f, Radius, Start.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugDirectionalArrow(World, Start.GetLocation(), End.GetLocation(), 50.0f, FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
	DrawDebugCapsule(World, End.GetLocation(), Height * 0.5f, Radius, Start.GetRotation(), FColor::Red, ShouldDrawInEditor(), GetDebugQueryLifetime());
}

#endif