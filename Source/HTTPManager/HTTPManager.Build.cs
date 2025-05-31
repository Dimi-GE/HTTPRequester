// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class HTTPManager : ModuleRules
{
	public HTTPManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
		"Core",
		"CoreUObject",
		"Engine",
		"InputCore",
		"Blutility",
		"UMG",
		"HTTP",
		"Json",
		"JsonUtilities",
		"Slate",
		"SlateCore",
		"DesktopPlatform"

		});


        // -------- ZIP SUPPORT (Minizip) --------
        // Path to ThirdParty/MiniZIPLib
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "MiniZIPLib");
        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));
        PrivateIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));
		
		
        // 🎯 Add the core MiniZIP .cpp files
        // string[] MinizipSources = new string[]
		// {
		// 	"mz_strm.cpp",
		// 	"mz_strm_buf.cpp",
		// 	"mz_strm_mem.cpp",
		// 	"mz_strm_os.cpp",
		// 	"mz_zip_rw.cpp",
		// 	"mz_strm_wzaes.cpp",
		// 	"mz_crypt.cpp",
		// 	"mz_strm_split.cpp"
        // };

		// foreach (string SourceFile in MinizipSources)
		// {
		// 	PublicAdditionalLibraries.Add(Path.Combine(MiniZipSrcPath, SourceFile));
		// }

        // // In case some .c files are not compiled without this:
        // bUseUnity = false;
        // bEnableExceptions = true;

		// ---------------------------------------

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
