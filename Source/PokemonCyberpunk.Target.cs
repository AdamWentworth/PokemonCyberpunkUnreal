using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonCyberpunkTarget : TargetRules
{
	public PokemonCyberpunkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("PokemonCyberpunk");
	}
}
