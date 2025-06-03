// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/SPAbilityTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

USPAbilityTaskScratchPad::USPAbilityTaskScratchPad()
{
}

USPAbilityTaskScratchPad::~USPAbilityTaskScratchPad()
{
}

bool USPAbilityTaskScratchPad::SkipObjectReferencer_Implementation() const
{
	return true;
}

USPAbilityTask::USPAbilityTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USPAbilityTask::~USPAbilityTask()
{
}

bool USPAbilityTask::SkipObjectReferencer_Implementation() const
{
	return true;
}

void USPAbilityTask::Clear()
{
	Super::Clear();
	ClearBP();
}

void USPAbilityTask::ClearBP_Implementation()
{
}

void USPAbilityTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPAbilityTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
}

void USPAbilityTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void USPAbilityTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float DeltaTime) const
{
}

void USPAbilityTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                               const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPAbilityTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                const EAbleAbilityTaskResult result) const
{
}

bool USPAbilityTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool USPAbilityTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	return UAbleAbilityTask::IsDone(TWeakObjectPtr<const UAbleAbilityContext>(Context));
}

TSubclassOf<UAbleAbilityTaskScratchPad> USPAbilityTask::GetTaskScratchPadClass(const UAbleAbilityContext* Context) const
{
	return GetTaskScratchPadClassBP(Context);
}

TSubclassOf<UAbleAbilityTaskScratchPad> USPAbilityTask::GetTaskScratchPadClassBP_Implementation(
	const UAbleAbilityContext* Context) const
{
	return nullptr;
}

UAbleAbilityTaskScratchPad* USPAbilityTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = GetTaskScratchPadClass(Context.Get());
	if (UClass* ScratchPadCL = ScratchPadClass.Get())
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			UAbleAbilityTaskScratchPad* ScratchPad = Cast<USPAbilityTaskScratchPad>(
				Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
			//Pop From Pool No Need Reset ScratchPad，有下层业务把初始化放在了这个函数了没法直接去掉
			ResetScratchPad(ScratchPad);
			return ScratchPad;
		}

		// No Subsystem available?
		return Cast<UAbleAbilityTaskScratchPad>(NewObject<USPAbilityTaskScratchPad>(Context.Get(), ScratchPadCL));
	}

	UE_LOG(LogAbleSP, Warning,
	       TEXT(
		       "Custom Task [%s] is using the old Scratchpad code that will be removed. Please override \"GetTaskScratchPadClass\" in your blueprint to avoid future breakage."
	       ), *GetTaskNameBP().ToString())
	if (UAbleAbilityTaskScratchPad* ScratchPad = Cast<UAbleAbilityTaskScratchPad>(CreateScratchPadBP(Context.Get())))
	{
		ResetScratchPad(ScratchPad);
		return ScratchPad;
	}

	UE_LOG(LogAbleSP, Warning,
	       TEXT(
		       "CreateScratchPadBP returned an null scratch pad. Please check your Create Scratch Pad method in your Blueprint.\n"
	       ));

	return nullptr;
}

USPAbilityTaskScratchPad* USPAbilityTask::GetScratchPad(UAbleAbilityContext* Context) const
{
	return Cast<USPAbilityTaskScratchPad>(Context->GetScratchPadForTask(this));
}

USPAbilityTaskScratchPad* USPAbilityTask::CreateScratchPadBP_Implementation(UAbleAbilityContext* Context) const
{
	static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = GetTaskScratchPadClass(Context);
	if (!ScratchPadClass.Get())
	{
		return nullptr;
	}
	
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		return Cast<USPAbilityTaskScratchPad>(Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
	}

	return NewObject<USPAbilityTaskScratchPad>(Context);
}

void USPAbilityTask::ResetScratchPad(UAbleAbilityTaskScratchPad* ScratchPad) const
{
	ResetScratchPadBP(ScratchPad);
}

void USPAbilityTask::ResetScratchPadBP_Implementation(UAbleAbilityTaskScratchPad* ScratchPad) const
{
	// Do nothing since we have no idea about the contents of the scratchpad.
}

bool USPAbilityTask::IsSingleFrame() const
{
	return IsSingleFrameBP();
}

bool USPAbilityTask::IsSingleFrameBP_Implementation() const
{
	return true;
}

EAbleAbilityTaskRealm USPAbilityTask::GetTaskRealm() const
{
	return GetTaskRealmBP();
}

EAbleAbilityTaskRealm USPAbilityTask::GetTaskRealmBP_Implementation() const
{
	return EAbleAbilityTaskRealm::ATR_Client;
}

TStatId USPAbilityTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCustomTask, STATGROUP_Able);
}

FText USPAbilityTask::GetTaskCategoryBP_Implementation() const
{
	return LOCTEXT("AbleCustomTaskCategory", "Blueprint|Custom");
}

FText USPAbilityTask::GetTaskNameBP_Implementation() const
{
#if WITH_EDITOR
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FText USPAbilityTask::GetDescriptiveTaskNameBP_Implementation() const
{
	return LOCTEXT("AbleCustomTaskDisplayName", "Custom");
}

FText USPAbilityTask::GetTaskDescriptionBP_Implementation() const
{
#if WITH_EDITOR
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FLinearColor USPAbilityTask::GetTaskColorBP_Implementation() const
{
	return FLinearColor::White;
}

float USPAbilityTask::GetEndTime() const
{
	return GetEndTimeBP();
}

float USPAbilityTask::GetEndTimeBP_Implementation() const
{
	return Super::GetEndTime();
}

#if WITH_EDITOR

FText USPAbilityTask::GetTaskCategory() const
{
	return GetTaskCategoryBP();
}

FText USPAbilityTask::GetTaskName() const
{
	return GetTaskNameBP();
}

FText USPAbilityTask::GetDescriptiveTaskName() const
{
	return GetDescriptiveTaskNameBP();
}

FText USPAbilityTask::GetTaskDescription() const
{
	return GetTaskDescriptionBP();
}

FLinearColor USPAbilityTask::GetTaskColor() const
{
	return GetTaskColorBP();
}

EDataValidationResult USPAbilityTask::IsDataValid(TArray<FText>& ValidationErrors)
{
	const FText AssetName = GetTaskName();
	return IsTaskDataValid(nullptr, AssetName, ValidationErrors);
}

void USPAbilityTask::OnAbilityEditorTick(const UAbleAbilityContext& Context, float DeltaTime) const
{
	OnAbilityEditorTickBP(&Context, DeltaTime);
}

#endif

void USPAbilityTask::OnAbilityEditorTickBP_Implementation(const UAbleAbilityContext* Context, float DeltaTime) const
{
}

#undef LOCTEXT_NAMESPACE
