// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tasks/IAbleAbilityTask.h"
#include <Core/GameFeatureSystem/GameFeatureSystem.h>

#include "ableSubSystem.generated.h"

USTRUCT()
struct ABLECORESP_API FAbleTaskScratchPadBucket
{
	GENERATED_BODY()
public:
	FAbleTaskScratchPadBucket() : ScratchPadClass(nullptr), Instances() {};

	UPROPERTY()
	TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass;

	UPROPERTY()
	TArray<UAbleAbilityTaskScratchPad*> Instances;
};

USTRUCT()
struct ABLECORESP_API FAbleAbilityScratchPadBucket
{
	GENERATED_BODY()
public:
	FAbleAbilityScratchPadBucket() : ScratchPadClass(nullptr), Instances() {};

	UPROPERTY()
	TSubclassOf<UAbleAbilityScratchPad> ScratchPadClass;

	UPROPERTY()
	TArray<UAbleAbilityScratchPad*> Instances;
};

UCLASS(BlueprintType)
class ABLECORESP_API UAbleAbilityUtilitySubsystem : public UGameFeatureSystem, public IUnLuaInterface
{
	GENERATED_BODY()
public:
	UAbleAbilityUtilitySubsystem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual ~UAbleAbilityUtilitySubsystem();

	virtual void Initialize(UObject& Outer) override;

	virtual FString GetModuleName_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAbleAbilityContext* FindOrConstructContext();

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAbleAbilityTaskScratchPad* FindOrConstructTaskScratchPad(TSubclassOf<UAbleAbilityTaskScratchPad>& Class);

	UFUNCTION(BlueprintCallable, Category = "Able")
	UAbleAbilityScratchPad* FindOrConstructAbilityScratchPad(TSubclassOf<UAbleAbilityScratchPad>& Class);
	
	UFUNCTION(BlueprintCallable, Category = "Able")
	void ClearAllCachePools();

	// Return methods
	void ReturnContext(UAbleAbilityContext* Context);
	void ReturnTaskScratchPad(UAbleAbilityTaskScratchPad* Scratchpad);
	void ReturnAbilityScratchPad(UAbleAbilityScratchPad* Scratchpad);
	
	UDataTable* TryGetChannelPresentDataTable();
private:
	// Helper methods
	FAbleTaskScratchPadBucket* GetTaskBucketByClass(TSubclassOf<UAbleAbilityTaskScratchPad>& Class);
	FAbleAbilityScratchPadBucket* GetAbilityBucketByClass(TSubclassOf<UAbleAbilityScratchPad>& Class);
	uint32 GetTotalScratchPads() const;

	UPROPERTY(Transient)
	TArray<UAbleAbilityContext*> m_AllocatedContexts;

	UPROPERTY(Transient)
	TArray<UAbleAbilityContext*> m_AvailableContexts;

	UPROPERTY(Transient)
	TArray<FAbleTaskScratchPadBucket> m_TaskBuckets;

	UPROPERTY(Transient)
	TArray<FAbleAbilityScratchPadBucket> m_AbilityBuckets;

	UPROPERTY(Transient)
	const USPAbleSettings* m_Settings;

	UPROPERTY()
	UDataTable* m_ChannelPresentDataTable;
};