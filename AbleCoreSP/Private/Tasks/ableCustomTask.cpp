// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableCustomTask.h"

#include "ableAbility.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleCustomTaskScratchPad::UAbleCustomTaskScratchPad()
{

}

UAbleCustomTaskScratchPad::~UAbleCustomTaskScratchPad()
{

}

UAbleCustomTask::UAbleCustomTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleCustomTask::~UAbleCustomTask()
{

}

void UAbleCustomTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	OnTaskStartBP(Context.Get());
}

void UAbleCustomTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

}

void UAbleCustomTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	OnTaskTickBP(Context.Get(), deltaTime);
}

void UAbleCustomTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float DeltaTime) const
{

}

void UAbleCustomTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	OnTaskEndBP(Context.Get(), result);
}

void UAbleCustomTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

}

bool UAbleCustomTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return IsDoneBP(Context.Get());
}

bool UAbleCustomTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	return UAbleAbilityTask::IsDone(TWeakObjectPtr<const UAbleAbilityContext>(Context));
}

TSubclassOf<UAbleAbilityTaskScratchPad> UAbleCustomTask::GetTaskScratchPadClass(const UAbleAbilityContext* Context) const
{
	return GetTaskScratchPadClassBP(Context);
}

TSubclassOf<UAbleAbilityTaskScratchPad> UAbleCustomTask::GetTaskScratchPadClassBP_Implementation(const UAbleAbilityContext* Context) const
{
	return nullptr;
}

UAbleAbilityTaskScratchPad* UAbleCustomTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = GetTaskScratchPadClass(Context.Get());
	if (UClass* ScratchPadCL = ScratchPadClass.Get())
	{
		if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
		{
			UAbleAbilityTaskScratchPad* ScratchPad = Cast<UAbleCustomTaskScratchPad>(Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
			ResetScratchPad(ScratchPad);
			return ScratchPad;
		}

		// No Subsystem available?
		return Cast<UAbleAbilityTaskScratchPad>(NewObject<UAbleCustomTaskScratchPad>(Context.Get(), ScratchPadCL));
	}

	UE_LOG(LogAbleSP, Warning, TEXT("Custom Task [%s] is using the old Scratchpad code that will be removed. Please override \"GetTaskScratchPadClass\" in your blueprint to avoid future breakage."), *GetTaskNameBP().ToString())
	if (UAbleAbilityTaskScratchPad* ScratchPad = Cast<UAbleAbilityTaskScratchPad>(CreateScratchPadBP(Context.Get())))
	{
		ResetScratchPad(ScratchPad);
		return ScratchPad;
	}

	UE_LOG(LogAbleSP, Warning, TEXT("CreateScratchPadBP returned an null scratch pad. Please check your Create Scratch Pad method in your Blueprint.\n"));

	return nullptr;
}

UAbleCustomTaskScratchPad* UAbleCustomTask::GetScratchPad(UAbleAbilityContext* Context) const
{
	return Cast<UAbleCustomTaskScratchPad>(Context->GetScratchPadForTask(this));
}

UAbleCustomTaskScratchPad* UAbleCustomTask::CreateScratchPadBP_Implementation(UAbleAbilityContext* Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAbleCustomTaskScratchPad::StaticClass();
		return Cast<UAbleCustomTaskScratchPad>(Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass));
	}

	return NewObject<UAbleCustomTaskScratchPad>(Context);
}

void UAbleCustomTask::ResetScratchPad(UAbleAbilityTaskScratchPad* ScratchPad) const
{
	ResetScratchPadBP(ScratchPad);
}

void UAbleCustomTask::ResetScratchPadBP_Implementation(UAbleAbilityTaskScratchPad* ScratchPad) const
{
	// Do nothing since we have no idea about the contents of the scratchpad.
}

bool UAbleCustomTask::IsSingleFrame() const
{
	return IsSingleFrameBP();
}

bool UAbleCustomTask::IsSingleFrameBP_Implementation() const
{
	return true;
}

EAbleAbilityTaskRealm UAbleCustomTask::GetTaskRealm() const
{
	return GetTaskRealmBP();
}

EAbleAbilityTaskRealm UAbleCustomTask::GetTaskRealmBP_Implementation() const
{
	return EAbleAbilityTaskRealm::ATR_Client;
}

TStatId UAbleCustomTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleCustomTask, STATGROUP_Able);
}

FText UAbleCustomTask::GetTaskCategoryBP_Implementation() const
{
	return LOCTEXT("AbleCustomTaskCategory", "Blueprint|Custom");
}

FText UAbleCustomTask::GetTaskNameBP_Implementation() const
{
#if WITH_EDITOR 
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FText UAbleCustomTask::GetDescriptiveTaskNameBP_Implementation() const
{
	return LOCTEXT("AbleCustomTaskDisplayName", "Custom");
}

FText UAbleCustomTask::GetTaskDescriptionBP_Implementation() const
{
#if WITH_EDITOR 
	return GetClass()->GetDisplayNameText();
#else
	return FText::FromString(GetClass()->GetName());
#endif
}

FLinearColor UAbleCustomTask::GetTaskColorBP_Implementation() const
{
	return FLinearColor::White;
}
#if WITH_EDITOR

FText UAbleCustomTask::GetTaskCategory() const
{
	return GetTaskCategoryBP();
}

FText UAbleCustomTask::GetTaskName() const
{
	return GetTaskNameBP();
}

FText UAbleCustomTask::GetDescriptiveTaskName() const
{
	return GetDescriptiveTaskNameBP();
}

FText UAbleCustomTask::GetTaskDescription() const
{
	return GetTaskDescriptionBP();
}

FLinearColor UAbleCustomTask::GetTaskColor() const
{
	return GetTaskColorBP();
}

EDataValidationResult UAbleCustomTask::IsDataValid(TArray<FText>& ValidationErrors)
{
    const FText AssetName = GetTaskName();
    return IsTaskDataValid(nullptr, AssetName, ValidationErrors);
}

#endif

#undef LOCTEXT_NAMESPACE