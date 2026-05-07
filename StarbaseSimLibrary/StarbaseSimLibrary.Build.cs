
using UnrealBuildTool;

public class StarbaseSimLibrary : ModuleRules
{
	public StarbaseSimLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		//OptimizeCode = CodeOptimization.Never;


		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Json",
				"JsonUtilities" ,
				"LyraGame",
				"ModularGameplayActors",
				"Niagara",
				"Networking",
				"PhysicsCore",
				"Chaos",
				"ChaosSolverEngine",
				"Sockets",
				"ProceduralMeshComponent",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
				"CoreUObject",
				"Slate",
				"SlateCore",
				"CommonInput",
				"CommonUI",
				"UMG",

				
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
