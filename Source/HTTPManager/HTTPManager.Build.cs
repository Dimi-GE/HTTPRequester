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
		// Path to headers
		// Path to ThirdParty/MiniZIPLib
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "MiniZIPLib");
		string MiniZipIncludePath = Path.Combine(ThirdPartyPath, "include");
		string MiniZipSourcePath = Path.Combine(ModuleDirectory, "Private", "MiniZIPLib");

		PublicIncludePaths.Add(MiniZipIncludePath);
		PrivateIncludePaths.Add(MiniZipIncludePath);

		// Make sure Unreal knows to compile these .cpp files
		string[] MinizipFiles =
		{
			"mz_os.cpp",
			"mz_strm.cpp",
			"mz_strm_buf.cpp",
			"mz_strm_mem.cpp",
			"mz_strm_os.cpp",
			"mz_strm_pkcrypt.cpp",
			"mz_strm_split.cpp",
			"mz_strm_wzaes.cpp",
			"mz_zip_rw.cpp",
			"mz_crypt.cpp",
			"mz_zip.cpp"
		};

		foreach (string file in MinizipFiles)
		{
			string FullPath = Path.Combine(MiniZipSourcePath, file);
			RuntimeDependencies.Add(FullPath); // Optional, but doesn't hurt
			// PrivateDependencyModuleNames.Add("zlib"); // If you're using mz_strm_zlib
		}

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
