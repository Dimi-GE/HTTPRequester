# Minizip-ng Integration Guide for Unreal Engine 5

This guide provides a step-by-step process to integrate the minizip-ng library (from https://github.com/zlib-ng/minizip-ng) into an Unreal Engine 5 (UE5) C++ project for basic zip/unzip functionality (in memory or on disk, without compression or encryption). It’s tailored for beginners and uses only the essential source files, leveraging Unreal’s built-in zlib to avoid external dependencies.

## Prerequisites
- A UE5 C++ project (e.g., `MyProject`).
- Visual Studio Code or Visual Studio with UE5 build tools.
- Minizip-ng source files downloaded from https://github.com/zlib-ng/minizip-ng (download the ZIP and extract it).
- Platform: Windows (adjust for Linux/Mac as noted).

## Step-by-Step Integration

### 1. Prepare minizip-ng Files
- **Download**: Get the minizip-ng source from https://github.com/zlib-ng/minizip-ng (click “Code” > “Download ZIP” and extract).
- **Select Files**: Use only these files for basic zip/unzip without compression or encryption:
  - **Headers** (from `minizip-ng/`):
    - `mz.h`
    - `mz_os.h`
    - `mz_crypt.h` (used but disabled for crypto)
    - `mz_strm.h`
    - `mz_strm_buf.h`
    - `mz_strm_mem.h`
    - `mz_strm_zlib.h`
    - `mz_zip.h`
  - **Sources** (from `minizip-ng/`):
    - `mz_zip.c`
    - `mz_os.c`
    - `mz_strm.c`
    - `mz_strm_buf.c`
    - `mz_strm_mem.c`
    - `mz_strm_zlib.c`
    - `mz_os_win32.c` (Windows-specific OS functions)
    - `mz_strm_os_win32.c` (Windows-specific stream functions)
  - **For Linux/Mac**: Replace `mz_os_win32.c` and `mz_strm_os_win32.c` with `mz_os_posix.c` and `mz_strm_os_posix.c`.

### 2. Set Up Project Structure
- **Headers**: Copy the listed `.h` files to:
  ```
  MyProject/Source/ThirdParty/minizip-ng/include/
  ```
- **Sources**: Copy the listed `.c` files to:
  ```
  MyProject/Source/MyProject/minizip-ng/
  ```
- **CRC32 Wrapper**: Create `mz_crypt_wrapper.c` in `MyProject/Source/MyProject/minizip-ng` to provide `mz_crypt_crc32_update` using Unreal’s zlib:
  ```c
  #include "mz.h"
  #include "zlib.h"

  uint32_t mz_crypt_crc32_update(uint32_t crc, const uint8_t *buf, int32_t size) {
      return crc32(crc, buf, size);
  }
  ```
- **Why?** Placing sources in the main module (`MyProject/Source/MyProject`) ensures UnrealBuildTool (UBT) compiles them. The wrapper avoids encryption dependencies.

### 3. Update Build.cs
- Edit `MyProject/Source/MyProject/MyProject.Build.cs` to configure include paths and settings:
  ```csharp
  using UnrealBuildTool;
  using System.IO;

  public class MyProject : ModuleRules
  {
      public MyProject(ReadOnlyTargetRules Target) : base(Target)
      {
          PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

          PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "zlib" });
          PrivateDependencyModuleNames.AddRange(new string[] { });

          // Path to minizip-ng
          string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "minizip-ng");
          string IncludePath = Path.Combine(ThirdPartyPath, "include");
          string SourcePath = Path.Combine(ModuleDirectory, "minizip-ng");

          // Add include paths
          PublicIncludePaths.Add(IncludePath);
          PrivateIncludePaths.Add(SourcePath);

          // Disable unity build
          bUseUnityBuild = false;

          // Definitions for minizip-ng
          PublicDefinitions.Add("MZ_COMPAT"); // Compatibility mode
          PublicDefinitions.Add("MZ_ZIP_NO_CRYPTO"); // Disable encryption
          PublicDefinitions.Add("MZ_ZLIB"); // Use Unreal's zlib
          PublicDefinitions.Add("ZLIB_WINAPI"); // zlib compatibility
          PublicDefinitions.Add("MZ_ZIP_NO_COMPRESSION"); // Store-only mode

          // Windows-specific fixes
          if (Target.Platform == UnrealTargetPlatform.Win64)
          {
              PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
              PublicDefinitions.Add("WIN32");
          }
      }
  }
  ```
- **Key Settings**:
  - `zlib`: Links Unreal’s zlib for CRC32 and basic compression support.
  - `MZ_ZIP_NO_CRYPTO`: Disables encryption.
  - `MZ_ZIP_NO_COMPRESSION`: Ensures store-only mode.
  - `MZ_ZLIB`: Uses Unreal’s `zlib.h` instead of `zlib-ng.h`.
  - `bUseUnityBuild = false`: Prevents C file compilation issues.

### 4. Regenerate Project Files
- Right-click `MyProject.uproject` in File Explorer and select “Generate Visual Studio project files” (or in Unreal Editor, File > Refresh Visual Studio Code Project).

### 5. Build the Project
- In Visual Studio Code, press `Ctrl+Shift+B` and select “MyProject - Development Editor”.
- Check the Output panel. A successful build means minizip-ng is integrated.

### 6. Verify
- Open the UE5 Editor and ensure the project loads without errors.
- The library is now ready for use in C++ or Blueprints (e.g., for zipping/unzipping files).

## Knowledge Base: Errors Encountered and Fixes

### Error 1: PublicAdditionalSourceFiles Not Found
- **Error**: `Build.cs(43,9): error CS0103: The name 'PublicAdditionalSourceFiles' does not exist in the current context`
- **Cause**: `PublicAdditionalSourceFiles` is not a valid property in UnrealBuildTool.
- **Fix**: Moved `.c` files to `MyProject/Source/MyProject/minizip-ng` so UBT automatically compiles them as part of the main module.

### Error 2: Missing mz.h
- **Error**: `Source\HTTPManager\MiniZIPLib\mz_strm_mem.c(19): fatal error C1083: Cannot open include file: 'mz.h': No such file or directory`
- **Cause**: Incorrect include path or missing `mz.h` in the project.
- **Fix**: Placed headers in `MyProject/Source/ThirdParty/minizip-ng/include` and added `PublicIncludePaths.Add(IncludePath)` in `Build.cs`.

### Error 3: Missing zlib-ng.h
- **Error**: `Source\HTTPManager\minizip-ng\mz_strm_zlib.c: fatal error C1083: Cannot open include file: 'zlib-ng.h': No such file or directory`
- **Cause**: `mz_strm_zlib.c` expected zlib-ng, but we use Unreal’s zlib.
- **Fix**: Added `PublicDefinitions.Add("MZ_ZLIB")` to use `zlib.h`. Excluded `mz_crypt.c` (which also required `zlib-ng.h`) with `MZ_ZIP_NO_CRYPTO`.

### Error 4: Unresolved Externals
- **Error**:
  ```
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_crypt_crc32_update referenced in function mz_file_get_crc
  mz_zip.c.obj : error LNK2001: unresolved external symbol mz_crypt_crc32_update
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_os_make_dir referenced in function mz_dir_make
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_os_is_dir_separator referenced in function mz_dir_make
  mz_zip.c.obj : error LNK2001: unresolved external symbol mz_os_is_dir_separator
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_stream_os_open referenced in function mz_file_get_crc
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_stream_os_read referenced in function mz_file_get_crc
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_stream_os_close referenced in function mz_file_get_crc
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_stream_os_create referenced in function mz_file_get_crc
  mz_os.c.obj : error LNK2019: unresolved external symbol mz_stream_os_delete referenced in function mz_file_get_crc
  ```
- **Cause**: Missing platform-specific files (`mz_os_win32.c`, `mz_strm_os_win32.c`) and `mz_crypt_crc32_update` (from `mz_crypt.c`, which was excluded).
- **Fix**:
  - Added `mz_os_win32.c` and `mz_strm_os_win32.c` for Windows-specific functions.
  - Created `mz_crypt_wrapper.c` to implement `mz_crypt_crc32_update` using Unreal’s `crc32`.

## Notes
- **Platform**: For Linux/Mac, use `mz_os_posix.c` and `mz_strm_os_posix.c` instead of Windows files and remove `WIN32` from `PublicDefinitions`.
- **Prompt Limit**: This document fits within your 10-prompt-per-2-hour limit (as of 05:36 PM CEST, June 1, 2025).
- **Next Steps**: Add a C++ class to use minizip-ng for zipping/unzipping. Request a separate guide if needed.

If you encounter new errors or need further clarification, share details, and I’ll assist promptly.