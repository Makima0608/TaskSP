// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Engine/DecalActor.h"
#include "Tasks/IAbleAbilityTask.h"
#include "SPSpawnDecalTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

USTRUCT()
struct FSpawnedDecalInfo {
	GENERATED_BODY()
	FSpawnedDecalInfo(TWeakObjectPtr<ADecalActor> InSpawnedDecal, TWeakObjectPtr<AActor> InDecalTarget) {
		SpawnedDecal = InSpawnedDecal;
		DecalTarget = InDecalTarget;
	}
	FSpawnedDecalInfo() {}
	TWeakObjectPtr<ADecalActor> SpawnedDecal;
	TWeakObjectPtr<AActor> DecalTarget;
};

UCLASS(Transient)
class USPSpawnDecalTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	USPSpawnDecalTaskScratchPad();
	
	virtual ~USPSpawnDecalTaskScratchPad();

	/* All the Particle effects we've spawned. */
	UPROPERTY(transient)
	TArray<FSpawnedDecalInfo> SpawnedDecalInfos;
};

USTRUCT(BlueprintType)
struct FSPSpawnDecalTaskFollowInfo {
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (DisplayName = "Follow target"))
	bool m_FollowTarget;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (DisplayName = "Time percentage to follow target", ToolTip = "跟随时长占task时长的百分比", ClampMin = "0.0", ClampMax = "100.0", EditCondition = "m_FollowTarget==true"))
	float m_FollowTargetTimePercentage;

	FSPSpawnDecalTaskFollowInfo()
	{
		m_FollowTarget = false;
		m_FollowTargetTimePercentage = 100.0f;
	}
};

/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPSpawnDecalTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	USPSpawnDecalTask(const FObjectInitializer& ObjectInitializer);

	~USPSpawnDecalTask();
	
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;
	
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	virtual TStatId GetStatId() const override;

	virtual bool NeedsTick() const override { return !IsSingleFrame(); }

	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const override;

#if WITH_EDITOR

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPSpawnDecalTask", "Spawn"); }

	virtual FText GetTaskName() const override { return LOCTEXT("USPSpawnDecalTask", "Spawn Decal"); }

#endif

protected:
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName="Decal Template"))
	TSubclassOf<ADecalActor> m_DecalActor;
	
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName="Decal Material"))
	TSoftObjectPtr<UMaterialInstance> MaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName="Decal Material Parameter Name"))
	TMap<FName, TSoftObjectPtr<UCurveFloat>> m_Parameters;
	
	UPROPERTY(EditAnywhere, Category = "Decal", meta=(DisplayName = "Location"))
	FAbleAbilityTargetTypeLocation m_Location;
    
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName = "Scale"))
	float m_Scale;

	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName = "Destroy on End"))
	bool m_DestroyAtEnd;

	UPROPERTY(EditAnywhere, Category = "Decal", meta = (DisplayName = "Attach info"))
	FSPSpawnDecalTaskFollowInfo AttachInfo;

	/* What realm, server or client, to execute this task. If your game isn't networked - this field is ignored. */
	UPROPERTY(EditAnywhere, Category = "Realm", meta = (DisplayName = "IsServer"))
	bool m_IsServer;
};

#undef LOCTEXT_NAMESPACE