// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityUtilities.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UnrealEngine.h"

#define LOCTEXT_NAMESPACE "AbleLogHelper"

template<typename T>
const FString GetEnumAsString(T value)
{
	if (UEnum* EnumClass = StaticEnum<T>())
	{
#if WITH_EDITOR
		return EnumClass->GetDisplayNameTextByValue(static_cast<int32>(value)).ToString();
#else
		return EnumClass->GetNameStringByValue(static_cast<int32>(value));
#endif
	}

	return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();
}

const FString FAbleSPLogHelper::GetResultEnumAsString(EAbleAbilityStartResult Result)
{
	return GetEnumAsString<EAbleAbilityStartResult>(Result);
}

const FString FAbleSPLogHelper::GetTaskResultEnumAsString(EAbleAbilityTaskResult Result)
{
	return GetEnumAsString<EAbleAbilityTaskResult>(Result);
}

const FString FAbleSPLogHelper::GetTargetTargetEnumAsString(EAbleAbilityTargetType Result)
{
	return GetEnumAsString<EAbleAbilityTargetType>(Result);
}

const FString FAbleSPLogHelper::GetCollisionResponseEnumAsString(ECollisionResponse Response)
{
	return GetEnumAsString<ECollisionResponse>(Response);
}

const FString FAbleSPLogHelper::GetCollisionChannelEnumAsString(ECollisionChannel Channel)
{
	return GetEnumAsString<ECollisionChannel>(Channel);
}

const FString FAbleSPLogHelper::GetWorldName(const UWorld* World)
{
    // YANKED from SceneOutliner::GetWorldDescription

    FText PostFix;

    const FWorldContext* WorldContext = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.World() == World)
        {
            WorldContext = &Context;
            break;
        }
    }

	if (!World)
	{
		return TEXT("None");
	}

    if (World->WorldType == EWorldType::PIE)
    {
        switch (World->GetNetMode())
        {
        case NM_Client:
            if (World)
            {
                PostFix = FText::Format(LOCTEXT("ClientPostfixFormat", "(Client {0})"), FText::AsNumber(WorldContext->PIEInstance - 1));
            }
            else
            {
                PostFix = LOCTEXT("ClientPostfix", "(Client)");
            }
            break;
        case NM_DedicatedServer:
        case NM_ListenServer:
            PostFix = LOCTEXT("ServerPostfix", "(Server)");
            break;
        case NM_Standalone:
            PostFix = LOCTEXT("PlayInEditorPostfix", "(Play In Editor)");
            break;
        }
    }
    else if (World->WorldType == EWorldType::Editor)
    {
        PostFix = LOCTEXT("EditorPostfix", "(Editor)");
    }

    return FText::Format(LOCTEXT("WorldFormat", "{0} {1}"), FText::FromString(World->GetFName().GetPlainNameString()), PostFix).ToString();
}

const TArray<FKey> FAbleAbilityUtilities::GetKeysForInputAction(const FName& InputAction)
{
	TArray<FKey> ReturnValue;
	const UInputSettings* InputSettings = GetDefault<UInputSettings>();

	if (InputSettings)
	{
		const FName LocalCopy = InputAction;
		for (const FInputActionKeyMapping& ActionMapping : InputSettings->GetActionMappings())
		{
			if (ActionMapping.ActionName == InputAction)
			{
				ReturnValue.Add(ActionMapping.Key);
			}
		}
	}

	return ReturnValue;
}

UBlackboardComponent* FAbleAbilityUtilities::GetBlackboard(AActor* Target)
{
	UBlackboardComponent* BlackboardComp = nullptr;

	if (Target != nullptr)
	{
		APawn* TargetPawn = Cast<APawn>(Target);
		if (TargetPawn && TargetPawn->GetController())
		{
			BlackboardComp = TargetPawn->GetController()->FindComponentByClass<UBlackboardComponent>();
		}

		if (BlackboardComp == nullptr)
		{
			BlackboardComp = Target->FindComponentByClass<UBlackboardComponent>();
		}
	}

	return BlackboardComp;
}

#undef LOCTEXT_NAMESPACE