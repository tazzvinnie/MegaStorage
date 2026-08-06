using UnrealBuildTool;

public class MegaStorage : ModuleRules
{
	public MegaStorage(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject",
			"Engine",
			"FactoryGame",
			"SML"
		});
	}
}
