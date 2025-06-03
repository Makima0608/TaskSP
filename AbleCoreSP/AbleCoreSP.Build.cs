// Copyright (c) Extra Life Studios, LLC. All rights reserved.

namespace UnrealBuildTool.Rules
{
	public class AbleCoreSP : ModuleRules
	{
		public AbleCoreSP(ReadOnlyTargetRules Target) : base (Target)
        {
	        if (Target.bBuildEditor == true && Target.Type == TargetRules.TargetType.Editor)
	        {
		        OptimizeCode = CodeOptimization.Never;
	        }
	        
            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
                    System.IO.Path.Combine(ModuleDirectory, "Classes"),
                    System.IO.Path.Combine(ModuleDirectory, "Public"),
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
                    System.IO.Path.Combine(ModuleDirectory, "Private"),
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"AIModule",				
					"Core",
					"CoreUObject",
					"Engine",
					"EnhancedInput",
					"GameplayTags",
					"GameplayTasks", // AIModule requires this...					
					"InputCore",
					"NavigationSystem",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnLua", "MoeGameCore"
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
                PrivateDependencyModuleNames.Add("Slate");
                PrivateDependencyModuleNames.Add("SlateCore");
                PrivateDependencyModuleNames.Add("SequenceRecorder");
                PrivateDependencyModuleNames.Add("GameplayTagsEditor");
            }
        }
	}
}