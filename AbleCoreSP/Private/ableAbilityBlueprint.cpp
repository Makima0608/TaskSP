// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "ableAbilityBlueprint.h"
#include "ableAbilityBlueprintGeneratedClass.h"

UAbleAbilityBlueprint::UAbleAbilityBlueprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR

UClass* UAbleAbilityBlueprint::GetBlueprintClass() const
{
	return UAbleAbilityBlueprintGeneratedClass::StaticClass();
}


/** Returns the most base ability blueprint for a given blueprint (if it is inherited from another ability blueprint, returning null if only native / non-ability BP classes are it's parent) */
UAbleAbilityBlueprint* UAbleAbilityBlueprint::FindRootGameplayAbilityBlueprint(UAbleAbilityBlueprint* DerivedBlueprint)
{
	UAbleAbilityBlueprint* ParentBP = NULL;

	// Determine if there is a ability blueprint in the ancestry of this class
	for (UClass* ParentClass = DerivedBlueprint->ParentClass; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
	{
		if (UAbleAbilityBlueprint* TestBP = Cast<UAbleAbilityBlueprint>(ParentClass->ClassGeneratedBy))
		{
			ParentBP = TestBP;
		}
	}

	return ParentBP;
}

bool UAbleAbilityBlueprint::AlwaysCompileOnLoad() const
{
	return true;
}

#endif