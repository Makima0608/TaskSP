// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableStopAcrossTask.h"

#include "ableAbilityComponent.h"
#include "AbleCoreSPPrivate.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAbleStopAcrossTask::UAbleStopAcrossTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAbleStopAcrossTask::~UAbleStopAcrossTask()
{

}

FString UAbleStopAcrossTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AbleStopAcrossTask");
}

void UAbleStopAcrossTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAbleStopAcrossTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	Context->GetSelfAbilityComponent()->StopAcrossTask(Context, m_AcrossTasks);
}

void UAbleStopAcrossTask::AddAcrossTask(const UAbleAbilityTask* Task)
{
	m_AcrossTasks.AddUnique(Task);
	
#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_HeadTasks"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

void UAbleStopAcrossTask::RemoveAcrossTask(const UAbleAbilityTask* Task)
{
	m_AcrossTasks.Remove(Task);

#if WITH_EDITORONLY_DATA
	FPropertyChangedEvent ChangeEvent(GetClass()->FindPropertyByName(FName(TEXT("m_HeadTasks"))));
	m_OnTaskPropertyModified.Broadcast(*this, ChangeEvent);
#endif
}

TStatId UAbleStopAcrossTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAbleStopAcrossTask, STATGROUP_Able);
}

#if WITH_EDITOR
FText UAbleStopAcrossTask::GetRichTextTaskSummary() const
{
	FTextBuilder SummaryString;
	SummaryString.AppendLine(Super::GetRichTextTaskSummary());

	if (GetAcrossTasks().Num())
	{
		SummaryString.AppendLine(LOCTEXT("AbleAbilityAcrossTask",  "\t - AcrossTasks: "));
		// FText HeadTasksFmt = LOCTEXT("AbleAbilityHeadTaskFmt", "\t\t - <a id=\"AbleTextDecorators.TaskDependency\" style=\"RichText.Hyperlink\" Index=\"{0}\">{1}</>");
		const TArray<const UAbleAbilityTask*>& HeadTasks = GetAcrossTasks();
		for (int i = 0; i < HeadTasks.Num(); ++i)
		{
			if (!HeadTasks[i])
			{
				continue;
			}

			SummaryString.AppendLine(FText::FromString(FString::Printf(TEXT("\t\t - %s"), *GetAcrossTasks()[i]->GetName())));
		}
	}

	return SummaryString.ToText();
}
#endif

bool UAbleStopAcrossTask::IsSingleFrameBP_Implementation() const { return true; }

EAbleAbilityTaskRealm UAbleStopAcrossTask::GetTaskRealmBP_Implementation() const { return m_TaskRealm; } // Client for Auth client, Server for AIs/Proxies.

#undef LOCTEXT_NAMESPACE

