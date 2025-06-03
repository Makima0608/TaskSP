// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "Engine/EngineTypes.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ableSetCollisionChannelResponseTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UPrimitiveComponent;

/* Helper struct that keeps track of a Primitive component, Channel, and Response.*/
USTRUCT()
struct FCollisionLayerResponseEntry
{
	GENERATED_USTRUCT_BODY()
public:
	FCollisionLayerResponseEntry() 
	: Primitive(nullptr),
	Channel(ECollisionChannel::ECC_Pawn),
	Response(ECollisionResponse::ECR_Overlap) {};
	FCollisionLayerResponseEntry(UPrimitiveComponent* InPrimitive, ECollisionChannel InChannel, ECollisionResponse InResponse)
		: Primitive(InPrimitive),
		Channel(InChannel),
		Response(InResponse)
	{ }
	TWeakObjectPtr<UPrimitiveComponent> Primitive;
	ECollisionChannel Channel;
	ECollisionResponse Response;
};

USTRUCT(BlueprintType)
struct FCollisionChannelResponsePair
{
	GENERATED_USTRUCT_BODY();
public:
	FCollisionChannelResponsePair(): CollisionChannel(ECollisionChannel::ECC_Pawn), CollisionResponse(ECollisionResponse::ECR_Ignore) {};
	FCollisionChannelResponsePair(ECollisionChannel _channel, ECollisionResponse _resp) : 
	CollisionChannel(_channel), CollisionResponse(_resp) 
	{ }
	
	UPROPERTY(EditInstanceOnly, Category = "Collision")
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	UPROPERTY(EditInstanceOnly, Category = "Collision")
	TEnumAsByte<ECollisionResponse> CollisionResponse;
};

/* Scratchpad for our Task. */
UCLASS(Transient)
class UAbleSetCollisionChannelResponseTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAbleSetCollisionChannelResponseTaskScratchPad();
	virtual ~UAbleSetCollisionChannelResponseTaskScratchPad();

	/* The original values of all the channels we've changed. */
	UPROPERTY(transient)
	TArray<FCollisionLayerResponseEntry> PreviousCollisionValues;
};

UCLASS()
class UAbleSetCollisionChannelResponseTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
public:
	UAbleSetCollisionChannelResponseTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAbleSetCollisionChannelResponseTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Returns true if our Task is Async. */
	virtual bool IsAsyncFriendly() const { return true; }

	/* Returns the realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for our Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AbleSetCollisionChannelResponseTaskCategory", "Collision"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AbleSetCollisionChannelResponseTask", "Set Collision Channel Response(已废弃)"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AbleSetCollisionChannelResponseTaskDesc", "Sets the Collision Channel (or all channels) Response on any targets, can optionally restore the previous channel responses."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(184.0f / 255.0f, 237.0f / 255.0f, 58.0f / 255.0f); }

	virtual bool IsDeprecated() const override { return true; }
#endif

protected:
	/* The Collision Channel to set the response for. -- DEPRECATED */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Channel"))
	TEnumAsByte<ECollisionChannel> m_Channel;

	/* The Response to change to.*/
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Response"))
	TEnumAsByte<ECollisionResponse> m_Response;

	/* The channels and responses to change. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName= "Channels"))
	TArray<FCollisionChannelResponsePair> m_Channels;

	/* If true, all channels will be set to this response. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Set All Channels"))
	bool m_SetAllChannelsToResponse;

	/* If true, all the original values will be restored when the Task ends. */
	UPROPERTY(EditAnywhere, Category = "Collision", meta = (DisplayName = "Restore Response On End"))
	bool m_RestoreOnEnd;
};

#undef LOCTEXT_NAMESPACE