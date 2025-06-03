// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "ableAbility.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "ableAbilityBlueprintLibrary.generated.h"

#define LOCTEXT_NAMESPACE "AbleCore"

class USPAbilityTaskScratchPad;
class UAbleAbility;
class UAbleAbilityInstance;
class UAbleAbilityContext;
class UAbleCustomTaskScratchPad;

UCLASS()
class ABLECORESP_API UAbleAbilityBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	* Returns an instance (CDO) of the provided Ability Class.
	*
	* @param Ability SubClass to grab the CDO from.
	*
	* @return the default class instance.
	*/
	UFUNCTION(BlueprintPure, Category="Able|Ability")
	static UAbleAbility* GetAbilityObjectFromClass(TSubclassOf<UAbleAbility> Class);

	/**
	* Activates the Ability using the provided context and returns the result.
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @return the start result returned by the Ability.
	* 
	* @deprecated **function will be private, please use USPAbilityComponent::TryActivateAbilityContext() replaced**
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability")
	static EAbleAbilityStartResult ActivateAbility(UAbleAbilityContext* Context);

	/**
	* Creates an Ability Context with the provided information.
	*
	* @param Ability The Ability to execute.
	* @param AbilityComponent The Ability Component that will be running this Context.
	* @param Owner The "owner" to set as the Owner Target Type.
	* @param Instigator The "Instigator" to set as the Instigator Target Type.
	*
	* @return the Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	static UAbleAbilityContext* CreateAbilityContext(const UAbleAbility* Ability, UAbleAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator);

	/**
	* Creates an Ability Context with the provided information.
	*
	* @param Ability The Ability to execute.
	* @param AbilityComponent The Ability Component that will be running this Context.
	* @param Owner The "owner" to set as the Owner Target Type.
	* @param Instigator The "Instigator" to set as the Instigator Target Type.
	* @param Target Sets the "Target Location" to the provided value.
	*
	* @return the Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Scratchpad).
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Context")
	static UAbleAbilityContext* CreateAbilityContextWithLocation(const UAbleAbility* Ability, UAbleAbilityComponent* AbilityComponent, AActor* Owner, AActor* Instigator, FVector Location);

	/**
	* Returns true if the provided start result is considered successful.
	*
	* @param Result The result enum returned by another Ability method (CanActivate, Activate, etc).
	*
	* @return true if the result was a success, false if it failed for whatever reason.
	*/
	UFUNCTION(BlueprintPure, Category = "Able|Ability")
	static bool IsSuccessfulStartResult(EAbleAbilityStartResult Result);

	/**
	* Returns whether debugging drawing of collision queries is enabled or not.
	*
	* @return true if debug drawing is enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Debug")
	static bool GetDrawCollisionQueries();

	/**
	* Toggles the viewing of collision queries within Able. Returns the new value.
	*
	* @param Enable whether to enable or disable debug drawing.
	*
	* @return whether or not debug drawing is now enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Ability|Debug")
	static bool SetDrawCollisionQueries(bool Enable);

	/**
	* Creates a ScratchPad based on the UAbleCustomScratchPad, for use by Custom Tasks.
	*
	* @param Context The Ability Context (https://able.extralifestudios.com/wiki/index.php/Ability_Context)
	* @param ScratchPadClass The Task Scratchpad Subclass to instantiate.
	*
	* @return the created Custom Scratchpad Object.
	*/
	UFUNCTION(BlueprintCallable, Category = "Able|Custom Task")
	static USPAbilityTaskScratchPad* CreateCustomScratchPad(UAbleAbilityContext* Context, TSubclassOf<USPAbilityTaskScratchPad> ScratchPadClass);

	UFUNCTION(BlueprintCallable)
	static TEnumAsByte<ETraceTypeQuery> GetQueryType(const UAbleAbilityContext* Context, ESPAbleTraceType TraceType);

	UFUNCTION(BlueprintCallable)
	static TArray<TEnumAsByte<ETraceTypeQuery>> GetQueryTypePresent(const UAbleAbilityContext* Context, TEnumAsByte<EAbleChannelPresent> Present, TArray<TEnumAsByte<ESPAbleTraceType>>& TraceTypes);

	UFUNCTION(BlueprintCallable)
	static TEnumAsByte<ECollisionChannel> GetCollisionChannelFromTraceType(const UAbleAbilityContext* Context, const ESPAbleTraceType TraceType);

	UFUNCTION(BlueprintCallable)
	static TEnumAsByte<EObjectTypeQuery> GetObjectTypeFromTraceType(const UAbleAbilityContext* Context, const ESPAbleTraceType TraceType);
	
	UFUNCTION(BlueprintCallable)
	static TArray<TEnumAsByte<ECollisionChannel>> GetCollisionChannelPresent(const UAbleAbilityContext* Context, TEnumAsByte<EAbleChannelPresent> Present, const TArray<TEnumAsByte<ESPAbleTraceType>> TraceTypes);

	UFUNCTION(BlueprintCallable)
	static TArray<TEnumAsByte<EObjectTypeQuery>> GetObjectTypesPresent(const UAbleAbilityContext* Context, TEnumAsByte<EAbleChannelPresent> Present, const TArray<TEnumAsByte<ESPAbleTraceType>> TraceTypes);
	
	UFUNCTION(BlueprintCallable)
	static TArray<TEnumAsByte<ESPAbleTraceType>> GetTraceTypeDataTable(const UObject* WorldContext, const TEnumAsByte<EAbleChannelPresent> Present);

	UFUNCTION(BlueprintCallable)
	static bool CheckBranchCond(UAbleAbilityContext* Context, const TArray<UAbleChannelingBase*>& Conditions, const bool bMustPassAllConditions);

	UFUNCTION(BlueprintCallable)
	static bool CheckBranchCondWithBranchData(UAbleAbilityContext* Context, const FAbilitySegmentBranchData& BranchData);

	UFUNCTION(BlueprintPure)
	static bool IsValidForNetwork(const AActor* RefActor, const EAbleAbilityTaskRealm Realm);

	UFUNCTION(BlueprintCallable)
	static AActor* GetSingleActorFromTargetType(const UAbleAbilityContext* Context, EAbleAbilityTargetType TargetType, int32 TargetIndex = 0);

	UFUNCTION(BlueprintCallable)
	static void SetComponentTickEnableImplicitRef(const FName ReferenceKey, UActorComponent* Component, const UAbleAbilityContext* Context);

	UFUNCTION(BlueprintCallable)
	static void SetComponentTickDisableImplicitRef(const FName ReferenceKey, UActorComponent* Component, const UAbleAbilityContext* Context);
};

#undef LOCTEXT_NAMESPACE
