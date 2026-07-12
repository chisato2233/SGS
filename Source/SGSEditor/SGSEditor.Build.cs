using UnrealBuildTool;

public class SGSEditor : ModuleRules
{
	public SGSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"MessageLog",
			"UnrealEd"
		});
	}
}
