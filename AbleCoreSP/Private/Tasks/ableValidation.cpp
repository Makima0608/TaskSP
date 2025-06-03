// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ableValidation.h"

#include "Tasks/ableCollisionQueryTask.h"
#include "Tasks/ableRayCastQueryTask.h"
#include "Tasks/ableCollisionSweepTask.h"

bool UAbleValidation::CheckDependencies(const TArray<const UAbleAbilityTask *>& taskDependencies)
{
    bool hasValidDependency = false;

    for (const UAbleAbilityTask * taskDep : taskDependencies)
    {
		if (!taskDep)
		{
			continue;
		}

        if (const UAbleRayCastQueryTask* task = Cast<UAbleRayCastQueryTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
        if (const UAbleCollisionQueryTask* task = Cast<UAbleCollisionQueryTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
        if (const UAbleCollisionSweepTask* task = Cast<UAbleCollisionSweepTask>(taskDep))
        {
            if (task->GetCopyResultsToContext())
                hasValidDependency = true;
        }
    }

    return hasValidDependency;
}
