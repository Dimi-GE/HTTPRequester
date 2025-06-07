# Minizip-ng Full Integration Guide with Encryption for Unreal Engine 5

This guide explains how to integrate the minizip-ng library (https://github.com/zlib-ng/minizip-ng) into an Unreal Engine 5 (UE5) C++ project for zipping and unzipping files with compression (deflate) and AES encryption. It’s designed for beginners, uses Unreal’s built-in zlib, and addresses prior issues like the `mz_stream_os_create` signature mismatch.

## Prerequisites

- A UE5 C++ project (e.g., `HTTPManager` in `D:\UE5\Projects\5.4\HTTPManager`).
- Visual Studio Code or Visual Studio with UE5 build tools.
- Minizip-ng source downloaded from https://github.com/zlib-ng/minizip-ng (click “Code” &gt; “Download ZIP” and extract).
- Platform: Windows (adjust for Linux/Mac as noted).

## Step-by-Step Integration

### 1. Prepare minizip-ng Files

- **Download**: Get the minizip-ng source from https://github.com/zlib-ng/minizip-ng (download ZIP and extract).
- **Select Files**: Include these for full functionality (zipping, unzipping, compression, AES encryption):
  - **Headers** (from `minizip-ng/`):
    - `mz.h`
    - `mz_os.h`
    - `mz_crypt.h` (for encryption)
    - `mz_strm.h`
    - `mz_strm_buf.h`
    - `mz_strm_mem.h`
    - `mz_strm_zlib.h`
    - `mz_zip.h`
    - `mz_strm_os.h`
  - **Sources** (from `minizip-ng/`):
    - `mz_zip.c`
    - `mz_os.c`
    - `mz_crypt.c` (for AES encryption)
    - `mz_strm.c`
    - `mz_strm_buf.c`
    - `mz_strm_mem.c`
    - `mz_strm_zlib.c`
    - `mz_os_win32.c` (Windows-specific OS functions)
    - `mz_strm_os_win32.c` (Windows-specific stream functions)
  - **For Linux/Mac**: Replace `mz_os_win32.c` and `mz_strm_os_win32.c` with `mz_os_posix.c` and `mz_strm_os_posix.c`.
  - **Exclude**: Test/tool files (e.g., `mztools.c`).
- **Note**: Unlike the previous store-only setup, we include `mz_crypt.c` for encryption and omit `mz_crypt_wrapper.c` (used to disable crypto).

### 2. Set Up Project Structure

- **Headers**: Copy the listed `.h` files to:

  ```
  D:\UE5\Projects\5.4\HTTPManager\Source\ThirdParty\minizip-ng\include\
  ```
- **Sources**: Copy the listed `.c` files to:

  ```
  D:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager\minizip-ng\
  ```
- **Why?** Headers go in `ThirdParty` for inclusion; sources go in the main module for UnrealBuildTool (UBT) to compile.

### 3. Update Build.cs

- Edit `D:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager\HTTPManager.Build.cs`:

```csharp
using UnrealBuildTool;
using System.IO;

public class HTTPManager : ModuleRules
{
    public HTTPManager(ReadOnlyTargetRules Target) : base(Target)
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
        PublicDefinitions.Add("MZ_ZLIB"); // Use Unreal's zlib
        PublicDefinitions.Add("ZLIB_WINAPI"); // zlib compatibility

        // Windows-specific fixes
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
            PublicDefinitions.Add("WIN32");
        }
    }
}
```

- **Changes**:
  - Removed `MZ_ZIP_NO_COMPRESSION` (to enable deflate).
  - Removed `MZ_ZIP_NO_CRYPTO` (to enable AES encryption).
  - Kept `zlib` dependency for compression and CRC32.

### 4. Verify `mz_stream_os_create`

- **Issue**: Previous `C2660` error showed a signature mismatch.
- Open `D:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager\minizip-ng\mz_strm_os_win32.c`.
- Ensure:

  ```c
  int32_t mz_stream_os_win32_create(void **stream) {
      mz_stream_win32 *win32 = (mz_stream_win32 *)calloc(1, sizeof(mz_stream_win32));
      if (win32) {
          win32->stream.vtbl = &mz_stream_os_vtbl;
          *stream = win32;
          return MZ_OK;
      }
      return MZ_MEM_ERROR;
  }
  ```
- **Note**: Your fix (`void* stream = mz_stream_os_create();`) is used in the sample code.

### 5. Create a Sample Zipping/Unzipping Class

- Create `ZipHandler.h` and `ZipHandler.cpp` in `D:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager`.
- **ZipHandler.h**:

  ```cpp
  #pragma once
  #include "CoreMinimal.h"
  #include "Components/ActorComponent.h"
  #include "ZipHandler.generated.h"
  
  UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
  class HTTPMANAGER_API UZipHandler : public UActorComponent
  {
      GENERATED_BODY()
  public:
      UZipHandler();
  
      // Create ZIP (disk/memory, with compression and optional encryption)
      UFUNCTION(BlueprintCallable, Category="Zip")
      bool CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath, TArray<uint8>& OutBuffer, bool bSaveToDisk, const FString& Password = "");
  
      // Extract ZIP (disk/memory, with optional encryption)
      UFUNCTION(BlueprintCallable, Category="Zip")
      bool ExtractZip(const FString& ZipPath, const TArray<uint8>& InBuffer, const FString& ExtractPath, TMap<FString, TArray<uint8>>& OutFiles, bool bFromDisk, const FString& Password = "");
  };
  ```
- **ZipHandler.cpp**:

  ```cpp
  #include "ZipHandler.h"
  #include "ThirdParty/minizip-ng/include/mz.h"
  #include "ThirdParty/minizip-ng/include/mz_zip.h"
  #include "ThirdParty/minizip-ng/include/mz_strm_mem.h"
  #include "ThirdParty/minizip-ng/include/mz_strm_os.h"
  #include "HAL/FileManager.h"
  #include "Misc/Paths.h"
  
  extern "C" {
      #include "ThirdParty/minizip-ng/include/mz_strm_os.h"
      #include "ThirdParty/minizip-ng/include/mz_zip.h"
      #include "ThirdParty/minizip-ng/include/mz_crypt.h"
  }
  
  UZipHandler::UZipHandler() {}
  
  bool UZipHandler::CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath, TArray<uint8>& OutBuffer, bool bSaveToDisk, const FString& Password)
  {
      void* stream = bSaveToDisk ? mz_stream_os_create() : mz_stream_mem_create();
      if (!stream)
      {
          UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create %s stream"), bSaveToDisk ? TEXT("file") : TEXT("memory"));
          return false;
      }
  
      if (bSaveToDisk)
      {
          mz_stream_os_set_filename(stream, TCHAR_TO_ANSI(*FPaths::ConvertRelativePathToFull(ZipPath)));
          if (mz_stream_open(stream, nullptr, MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to open file stream: %s"), *ZipPath);
              mz_stream_os_delete(&stream);
              return false;
          }
      }
      else
      {
          if (mz_stream_open(stream, nullptr, MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to open memory stream"));
              mz_stream_mem_delete(&stream);
              return false;
          }
      }
  
      void* zipHandle = mz_zip_create();
      if (!zipHandle || mz_zip_open(zipHandle, stream, MZ_OPEN_MODE_WRITE) != MZ_OK)
      {
          UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create/open zip handle"));
          if (bSaveToDisk) mz_stream_os_delete(&stream);
          else mz_stream_mem_delete(&stream);
          return false;
      }
  
      mz_zip_file file_info = { 0 };
      for (const FString& FilePath : FilePaths)
      {
          FString fileName = FPaths::GetCleanFilename(FilePath);
          TArray<uint8> fileData;
          if (!FFileHelper::LoadFileToArray(fileData, *FPaths::ConvertRelativePathToFull(FilePath)))
          {
              UE_LOG(LogTemp, Warning, TEXT("CreateZip: Failed to read file: %s - skipping..."), *FilePath);
              continue;
          }
  
          file_info.filename = TCHAR_TO_ANSI(*fileName);
          file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
          file_info.aes_version = Password.IsEmpty() ? 0 : MZ_AES_VERSION; // Enable AES if password provided
          if (mz_zip_entry_write_open(zipHandle, &file_info, MZ_COMPRESS_LEVEL_DEFAULT, 0, TCHAR_TO_ANSI(*Password)) != MZ_OK)
          {
              UE_LOG(LogTemp, Warning, TEXT("CreateZip: Failed to add file to ZIP: %s - skipping..."), *fileName);
              continue;
          }
  
          if (mz_zip_entry_write(zipHandle, fileData.GetData(), fileData.Num()) != MZ_OK)
          {
              UE_LOG(LogTemp, Warning, TEXT("CreateZip: Failed to write file to ZIP: %s - skipping..."), *fileName);
              mz_zip_entry_close(zipHandle);
              continue;
          }
          mz_zip_entry_close(zipHandle);
      }
  
      mz_zip_close(zipHandle);
      mz_zip_delete(&zipHandle);
  
      if (bSaveToDisk)
      {
          mz_stream_os_close(stream);
          mz_stream_os_delete(&stream);
      }
      else
      {
          const char* buf;
          int32_t buf_size;
          if (mz_stream_mem_get_buffer(stream, (const void**)&buf, &buf_size) == MZ_OK)
          {
              OutBuffer.SetNumUninitialized(buf_size);
              FMemory::Memcpy(OutBuffer.GetData(), buf, buf_size);
          }
          mz_stream_mem_delete(&stream);
      }
  
      UE_LOG(LogTemp, Log, TEXT("CreateZip: ZIP created successfully at %s"), *ZipPath);
      return true;
  }
  
  bool UZipHandler::ExtractZip(const FString& ZipPath, const TArray<uint8>& InBuffer, const FString& ExtractPath, TMap<FString, TArray<uint8>>& OutFiles, bool bFromDisk, const FString& Password)
  {
      void* stream = bFromDisk ? mz_stream_os_create() : mz_stream_mem_create();
      if (!stream)
      {
          UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to create %s stream"), bFromDisk ? TEXT("file") : TEXT("memory"));
          return false;
      }
  
      if (bFromDisk)
      {
          mz_stream_os_set_filename(stream, TCHAR_TO_ANSI(*FPaths::ConvertRelativePathToFull(ZipPath)));
          if (mz_stream_open(stream, nullptr, MZ_OPEN_MODE_READ) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to open ZIP file: %s"), *ZipPath);
              mz_stream_os_delete(&stream);
              return false;
          }
      }
      else
      {
          if (mz_stream_mem_set_buffer(stream, InBuffer.GetData(), InBuffer.Num()) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to set memory buffer"));
              mz_stream_mem_delete(&stream);
              return false;
          }
      }
  
      void* zipHandle = mz_zip_create();
      if (!zipHandle || mz_zip_open(zipHandle, stream, MZ_OPEN_MODE_READ) != MZ_OK)
      {
          UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to open zip handle"));
          if (bFromDisk) mz_stream_os_delete(&stream);
          else mz_stream_mem_delete(&stream);
          return false;
      }
  
      if (!ExtractPath.IsEmpty())
      {
          IFileManager& fileManager = IFileManager::Get();
          if (!fileManager.DirectoryExists(*ExtractPath))
          {
              fileManager.MakeDirectory(*ExtractPath, true);
          }
      }
  
      if (mz_zip_goto_first_entry(zipHandle) != MZ_OK)
      {
          UE_LOG(LogTemp, Error, TEXT("ExtractZip: No entries in ZIP"));
          mz_zip_close(zipHandle);
          mz_zip_delete(&zipHandle);
          if (bFromDisk) mz_stream_os_delete(&stream);
          else mz_stream_mem_delete(&stream);
          return false;
      }
  
      do
      {
          mz_zip_file* file_info;
          if (mz_zip_entry_get_info(zipHandle, &file_info) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to get entry info"));
              continue;
          }
  
          if (mz_zip_entry_is_dir(zipHandle) == MZ_OK)
              continue;
  
          if (mz_zip_entry_read_open(zipHandle, 0, TCHAR_TO_ANSI(*Password)) != MZ_OK)
          {
              UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to open entry: %s (check password)"), ANSI_TO_TCHAR(file_info->filename));
              continue;
          }
  
          int32_t size = file_info->uncompressed_size;
          TArray<uint8> fileData;
          fileData.SetNumUninitialized(size);
          int32_t bytesRead = mz_zip_entry_read(zipHandle, fileData.GetData(), size);
          mz_zip_entry_close(zipHandle);
  
          if (bytesRead != size)
          {
              UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to read entry: %s"), ANSI_TO_TCHAR(file_info->filename));
              continue;
          }
  
          FString fileName = ANSI_TO_TCHAR(file_info->filename);
          if (!ExtractPath.IsEmpty())
          {
              FString outFilePath = FPaths::Combine(ExtractPath, fileName);
              if (!FFileHelper::SaveArrayToFile(fileData, *FPaths::ConvertRelativePathToFull(outFilePath)))
              {
                  UE_LOG(LogTemp, Error, TEXT("ExtractZip: Failed to save file: %s"), *outFilePath);
              }
          }
          else
          {
              OutFiles.Add(fileName, fileData);
          }
      } while (mz_zip_goto_next_entry(zipHandle) == MZ_OK);
  
      mz_zip_close(zipHandle);
      mz_zip_delete(&zipHandle);
      if (bFromDisk) mz_stream_os_delete(&stream);
      else mz_stream_mem_delete(&stream);
  
      UE_LOG(LogTemp, Log, TEXT("ExtractZip: ZIP extracted successfully"));
      return true;
  }
  ```

### 6. Regenerate and Build

- **Regenerate**: Right-click `HTTPManager.uproject` &gt; “Generate Visual Studio project files”.
- **Build**: In Visual Studio Code, press `Ctrl+Shift+B`, select “HTTPManager - Development Editor”.

### 7. Test in Blueprintq

- **Add Component**: Add `ZipHandler` to an Actor in the UE5 Editor.
- **CreateZip**:
  - Inputs: `FilePaths` (e.g., `D:/UE5/Projects/5.4/HTTPManager/Content/File1.txt`), `ZipPath` (e.g., `D:/UE5/Projects/5.4/HTTPManager/MyZip.zip`), `OutBuffer`, `bSaveToDisk`, `Password` (e.g., “mysecret” for AES encryption).
  - Example: Call with `bSaveToDisk=true`, `Password="mysecret"`, check for encrypted ZIP.
- **ExtractZip**:
  - Inputs: `ZipPath`, `InBuffer`, `ExtractPath` (e.g., `D:/UE5/Projects/5.4/HTTPManager/Extracted/`), `OutFiles`, `bFromDisk`, `Password` (same as used in `CreateZip`).
  - Example: Call with `bFromDisk=true`, `Password="mysecret"`, check extracted files.

## Troubleshooting

- **Encryption Errors**:
  - **Error**: “Invalid password” or decryption failures.
  - **Fix**: Ensure the same password is used for `CreateZip` and `ExtractZip`. Minizip-ng uses AES encryption, which is case-sensitive.
- **Linker Errors**:
  - Verify `mz_crypt.c` is in `D:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager\minizip-ng`.
  - Check `zlib` in `PublicDependencyModuleNames`.
- **Stream Errors**:
  - Use `void* stream = mz_stream_os_create();` as you found.
  - Ensure `mz_strm_os_win32.c` matches the header signature.
- **Platform Notes**: For Linux/Mac, use `mz_os_posix.c` and `mz_strm_os_posix.c`, remove `WIN32` from `PublicDefinitions`.

## Notes

- **Encryption**: Uses AES via `mz_crypt.c`. Set `Password` to an empty string for no encryption.
- **Compression**: Uses `MZ_COMPRESS_METHOD_DEFLATE` with `MZ_COMPRESS_LEVEL_DEFAULT`.
- **Prompt Limit**: Fits within your 10 prompts per 2 hours (as of 07:23 AM CEST, June 7, 2025).
- **Beginner-Friendly**: Avoids complex build tools, uses Unreal’s `Build.cs`.

If errors occur, share the exact messages for quick fixes.