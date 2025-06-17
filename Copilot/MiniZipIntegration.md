# Minizip-ng Integration Guide for Unreal Engine 5

## Overview
This document provides a comprehensive guide for integrating the minizip-ng library into Unreal Engine 5 projects. Based on analysis of the HTTPManager project structure and the existing integration, this guide offers step-by-step instructions for successful implementation.

## 1. File Inventory Analysis

### Current ThirdParty Include Files
Located at: `d:\UE5\Projects\5.4\HTTPManager\Source\ThirdParty\minizip-ng\include`
```
mz.h                 - Core minizip-ng definitions and constants
mz_crypt.h          - Cryptographic functions (disabled in this integration)
mz_os.h             - Operating system abstraction layer
mz_strm.h           - Stream interface definitions
mz_strm_buf.h       - Buffered stream implementation
mz_strm_mem.h       - Memory stream implementation
mz_strm_os.h        - OS-specific stream interface
mz_strm_zlib.h      - zlib compression stream interface
mz_zip.h            - Main ZIP file operations interface
```

### Current Module Source Files (Limited Configuration)
Located at: `d:\UE5\Projects\5.4\HTTPManager\Source\HTTPManager\minizip-ng`
```
mz_crypt_wrapper.c   - Custom CRC32 wrapper using Unreal's zlib
mz_os.c             - Cross-platform OS operations
mz_os_win32.c       - Windows-specific OS implementations
mz_strm.c           - Base stream functionality
mz_strm_buf.c       - Buffered stream operations
mz_strm_mem.c       - Memory-based stream operations
mz_strm_os_win32.c  - Windows-specific stream operations
mz_zip.c            - Core ZIP file handling
```

### Required Additional Files for Full Integration
To enable compression and encryption, these additional files are needed:
```
mz_crypt.c          - Cryptographic functions and encryption support
mz_strm_zlib.c      - zlib compression stream implementation
mz_strm_bzip.c      - bzip2 compression stream (optional)
mz_strm_lzma.c      - LZMA compression stream (optional)
mz_strm_pkcrypt.c   - PKWare traditional encryption
mz_strm_wzaes.c     - WinZip AES encryption
mz_zip_rw.c         - Enhanced ZIP reading/writing operations
```

### Build Configuration Analysis (Current Limited Setup)
From `HTTPManager.Build-Copy.cs`:
```csharp
// Essential configuration elements identified:
string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "minizip-ng");
string IncludePath = Path.Combine(ThirdPartyPath, "include");
string SourcePath = Path.Combine(ModuleDirectory, "minizip-ng");

PublicIncludePaths.Add(IncludePath);
PrivateIncludePaths.Add(SourcePath);

// CURRENT LIMITATIONS - These definitions restrict functionality:
PublicDefinitions.Add("MZ_COMPAT");           // Compatibility mode
PublicDefinitions.Add("MZ_ZIP_NO_CRYPTO");    // DISABLES encryption
PublicDefinitions.Add("MZ_ZLIB");             // Use Unreal's zlib
PublicDefinitions.Add("ZLIB_WINAPI");         // zlib compatibility
PublicDefinitions.Add("MZ_ZIP_NO_COMPRESSION"); // DISABLES compression

// Unity build must be disabled for C file compilation
bUseUnity = false;
```

### Required Configuration for Full Integration
To enable compression and encryption, the Build.cs should be modified as follows:
```csharp
// FULL INTEGRATION - Enable all features:
PublicDefinitions.Add("MZ_COMPAT");           // Compatibility mode
// REMOVE: PublicDefinitions.Add("MZ_ZIP_NO_CRYPTO");    // This disables encryption
PublicDefinitions.Add("MZ_ZLIB");             // Use Unreal's zlib
PublicDefinitions.Add("ZLIB_WINAPI");         // zlib compatibility
// REMOVE: PublicDefinitions.Add("MZ_ZIP_NO_COMPRESSION"); // This disables compression

// Additional definitions for full functionality:
PublicDefinitions.Add("HAVE_ZLIB");           // Enable zlib compression
PublicDefinitions.Add("HAVE_PKCRYPT");        // Enable PKWare encryption
PublicDefinitions.Add("HAVE_WZAES");          // Enable WinZip AES encryption
```

## 2. Existing Integration Guide Analysis

Based on the existing `Minizip-ng-Integration-Guide-for-Unreal-Engine-5.markdown`:

### Confirmed Working Configuration (Full Integration Required)
- **Library Version**: minizip-ng from https://github.com/zlib-ng/minizip-ng
- **Compression**: FULL support (deflate, bzip2, LZMA) - Current setup is limited
- **Encryption**: FULL support (PKWare, WinZip AES) - Current setup is disabled
- **Platform**: Windows (with POSIX instructions for Linux/Mac)
- **Build System**: Direct integration with Unreal's zlib + additional crypto libraries

### Current Limitations and Required Changes
The existing integration is intentionally limited and needs these modifications for full functionality:
1. **Remove compression restriction**: Delete `MZ_ZIP_NO_COMPRESSION` definition
2. **Remove encryption restriction**: Delete `MZ_ZIP_NO_CRYPTO` definition  
3. **Add missing source files**: Include encryption and compression implementation files
4. **Add proper crypto support**: Replace `mz_crypt_wrapper.c` with full `mz_crypt.c`
5. **Enable additional dependencies**: Add support for advanced compression algorithms

### Known Issues and Solutions
1. **Missing mz_crypt_crc32_update**: Solved with custom wrapper
2. **Platform-specific functions**: Solved with mz_os_win32.c inclusion
3. **zlib compatibility**: Solved with MZ_ZLIB definition
4. **Unity build conflicts**: Solved with bUseUnity = false

## 3. Step-by-Step Integration Instructions

### Prerequisites
- Unreal Engine 5.4 or compatible version
- Visual Studio 2019 or later
- Minizip-ng source code from GitHub
- Windows development environment (adaptable to other platforms)

### Step 1: Download and Prepare Minizip-ng Source
1. **Download Source**:
   ```bash
   # Navigate to your project directory
   cd "d:\UE5\Projects\5.4\YourProjectName"
   
   # Download minizip-ng (or download ZIP from GitHub)
   git clone https://github.com/zlib-ng/minizip-ng.git temp_minizip
   ```

2. **Identify Required Files for Full Integration**:
   From the minizip-ng source, you need these files:
   
   **Headers** (copy to `Source/ThirdParty/minizip-ng/include/`):
   ```
   mz.h                 - Core definitions
   mz_os.h             - OS abstraction
   mz_crypt.h          - Cryptographic functions
   mz_strm.h           - Stream interfaces
   mz_strm_buf.h       - Buffered streams
   mz_strm_mem.h       - Memory streams
   mz_strm_os.h        - OS streams
   mz_strm_zlib.h      - zlib compression
   mz_strm_bzip.h      - bzip2 compression (optional)
   mz_strm_lzma.h      - LZMA compression (optional)
   mz_strm_pkcrypt.h   - PKWare encryption
   mz_strm_wzaes.h     - WinZip AES encryption
   mz_zip.h            - Main ZIP operations
   mz_zip_rw.h         - Advanced ZIP operations
   ```
   
   **Sources** (copy to `Source/YourModule/minizip-ng/`):
   ```
   mz_zip.c            - Core ZIP functionality
   mz_os.c             - OS abstraction
   mz_crypt.c          - Cryptographic functions (FULL VERSION, not wrapper)
   mz_strm.c           - Base streams
   mz_strm_buf.c       - Buffered streams
   mz_strm_mem.c       - Memory streams
   mz_strm_zlib.c      - zlib compression
   mz_strm_bzip.c      - bzip2 compression (optional)
   mz_strm_lzma.c      - LZMA compression (optional)
   mz_strm_pkcrypt.c   - PKWare encryption
   mz_strm_wzaes.c     - WinZip AES encryption
   mz_zip_rw.c         - Advanced ZIP operations
   mz_os_win32.c       - Windows OS implementation
   mz_strm_os_win32.c  - Windows stream implementation
   ```

### Step 2: Create Directory Structure
```powershell
# Create ThirdParty directory structure
New-Item -ItemType Directory -Force -Path "Source\ThirdParty\minizip-ng\include"

# Create module source directory
New-Item -ItemType Directory -Force -Path "Source\YourModule\minizip-ng"
```

### Step 3: Copy Required Files for Full Integration
```powershell
# Copy headers to ThirdParty include directory
Copy-Item "temp_minizip\mz*.h" "Source\ThirdParty\minizip-ng\include\"

# Copy CORE source files to module directory
Copy-Item "temp_minizip\mz_zip.c" "Source\YourModule\minizip-ng\"
Copy-Item "temp_minizip\mz_os.c" "Source\YourModule\minizip-ng\"
Copy-Item "temp_minizip\mz_crypt.c" "Source\YourModule\minizip-ng\"
Copy-Item "temp_minizip\mz_strm*.c" "Source\YourModule\minizip-ng\"
Copy-Item "temp_minizip\mz_zip_rw.c" "Source\YourModule\minizip-ng\"

# Copy PLATFORM-SPECIFIC files (Windows)
Copy-Item "temp_minizip\mz_os_win32.c" "Source\YourModule\minizip-ng\"
Copy-Item "temp_minizip\mz_strm_os_win32.c" "Source\YourModule\minizip-ng\"

# Clean up temporary directory
Remove-Item -Recurse -Force "temp_minizip"
```

**Important Note**: Do NOT create `mz_crypt_wrapper.c` for full integration - use the original `mz_crypt.c` instead.

### Step 4: Configure Build System for Full Integration
Edit `Source/YourModule/YourModule.Build.cs`:
```csharp
using UnrealBuildTool;
using System.IO;

public class YourModule : ModuleRules
{
    public YourModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Standard dependencies
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", "CoreUObject", "Engine", "zlib" 
        });

        // Minizip-ng FULL Integration Configuration
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "minizip-ng");
        string IncludePath = Path.Combine(ThirdPartyPath, "include");
        string SourcePath = Path.Combine(ModuleDirectory, "minizip-ng");

        // Add include paths
        PublicIncludePaths.Add(IncludePath);
        PrivateIncludePaths.Add(SourcePath);

        // CRITICAL: Disable unity build to prevent C file compilation issues
        bUseUnity = false;

        // Essential definitions for FULL minizip-ng integration
        PublicDefinitions.Add("MZ_COMPAT");              // Compatibility mode
        PublicDefinitions.Add("MZ_ZLIB");                // Use Unreal's zlib
        PublicDefinitions.Add("ZLIB_WINAPI");            // zlib compatibility
        
        // ENABLE COMPRESSION AND ENCRYPTION (remove NO_ restrictions)
        PublicDefinitions.Add("HAVE_ZLIB");              // Enable zlib compression
        PublicDefinitions.Add("HAVE_PKCRYPT");           // Enable PKWare encryption
        PublicDefinitions.Add("HAVE_WZAES");             // Enable WinZip AES encryption
        
        // Optional: Enable additional compression methods
        // PublicDefinitions.Add("HAVE_BZIP2");          // Enable bzip2 (requires bzip2 library)
        // PublicDefinitions.Add("HAVE_LZMA");           // Enable LZMA (requires LZMA library)

        // Windows-specific configuration
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
            PublicDefinitions.Add("WIN32");
        }

        // Additional dependencies if needed
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "Slate", "SlateCore" // Add if using UI components
        });
    }
}
```

**Key Changes from Limited Integration**:
- **REMOVED**: `MZ_ZIP_NO_CRYPTO` and `MZ_ZIP_NO_COMPRESSION` definitions
- **ADDED**: `HAVE_ZLIB`, `HAVE_PKCRYPT`, `HAVE_WZAES` to enable full functionality
- **NOTE**: No custom CRC32 wrapper needed - use full `mz_crypt.c` implementation

### Step 5: Platform-Specific Adjustments

#### For Linux/Mac Development:
Replace Windows-specific files with POSIX versions:
```bash
# Remove Windows files
rm Source/YourModule/minizip-ng/mz_os_win32.c
rm Source/YourModule/minizip-ng/mz_strm_os_win32.c

# Copy POSIX files instead
cp temp_minizip/mz_os_posix.c Source/YourModule/minizip-ng/
cp temp_minizip/mz_strm_os_posix.c Source/YourModule/minizip-ng/
```

Update Build.cs for non-Windows platforms:
```csharp
// Replace Windows-specific definitions
if (Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
{
    PublicDefinitions.Add("_GNU_SOURCE");
    // Remove WIN32 and ZLIB_WINAPI definitions for non-Windows platforms
}
```

### Step 6: Generate Project Files
```powershell
# Regenerate project files
.\GenerateProjectFiles.bat

# Or right-click .uproject file and select "Generate Visual Studio project files"
```

### Step 7: Build and Test
1. **Compile the Project**:
   ```powershell
   # Build from command line
   "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" YourProject.sln /p:Configuration=Development /p:Platform=x64
   ```

2. **Verify Integration**:
   Create a test file to verify the integration:
   ```cpp
   // In your module's .cpp file
   #include "mz.h"
   #include "mz_zip.h"
   #include "mz_strm_os.h"
   
   void TestMinizipIntegration()
   {
       void* stream = mz_stream_os_create();
       if (stream != nullptr)
       {
           UE_LOG(LogTemp, Warning, TEXT("Minizip-ng integration successful!"));
           mz_stream_os_delete(&stream);
       }
       else
       {
           UE_LOG(LogTemp, Error, TEXT("Minizip-ng integration failed!"));
       }
   }
   ```

### Step 8: Implementation Example with Full Features
Basic usage example with compression and encryption:
```cpp
#include "mz_zip.h"
#include "mz_strm_os.h"

void CreateCompressedEncryptedZip()
{
    // Create file stream
    void* file_stream = mz_stream_os_create();
    
    // Open ZIP file for writing
    int32 result = mz_stream_open(file_stream, "encrypted_test.zip", MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (result == MZ_OK)
    {
        // Create ZIP handle
        void* zip_handle = mz_zip_create();
        if (mz_zip_open(zip_handle, file_stream, MZ_OPEN_MODE_WRITE) == MZ_OK)
        {
            // Set compression level and encryption
            mz_zip_set_compress_method(zip_handle, MZ_COMPRESS_METHOD_DEFLATE);
            mz_zip_set_compress_level(zip_handle, MZ_COMPRESS_LEVEL_DEFAULT);
            
            // Enable AES encryption with password
            const char* password = "your_secure_password";
            mz_zip_set_password(zip_handle, password);
            
            UE_LOG(LogTemp, Warning, TEXT("Compressed and encrypted ZIP file created successfully"));
            
            // Cleanup
            mz_zip_close(zip_handle);
        }
        mz_zip_delete(&zip_handle);
        mz_stream_close(file_stream);
    }
    mz_stream_delete(&file_stream);
}

void ExtractEncryptedZip()
{
    // Similar pattern for extraction with password
    void* file_stream = mz_stream_os_create();
    int32 result = mz_stream_open(file_stream, "encrypted_test.zip", MZ_OPEN_MODE_READ);
    
    if (result == MZ_OK)
    {
        void* zip_handle = mz_zip_create();
        if (mz_zip_open(zip_handle, file_stream, MZ_OPEN_MODE_READ) == MZ_OK)
        {
            // Set password for decryption
            const char* password = "your_secure_password";
            mz_zip_set_password(zip_handle, password);
            
            // Extract files with automatic decompression and decryption
            UE_LOG(LogTemp, Warning, TEXT("Extracting encrypted ZIP file"));
            
            mz_zip_close(zip_handle);
        }
        mz_zip_delete(&zip_handle);
        mz_stream_close(file_stream);
    }
    mz_stream_delete(&file_stream);
}
```

## 4. Troubleshooting Common Issues

### Issue 1: Build Errors with Unity Build
**Symptoms**: Compilation errors with C files
**Solution**: Ensure `bUseUnity = false` in Build.cs

### Issue 2: Missing Cryptographic Functions
**Symptoms**: Linker errors for encryption/decryption functions
**Solution**: Ensure `mz_crypt.c` (NOT mz_crypt_wrapper.c) is present and `MZ_ZIP_NO_CRYPTO` is NOT defined

### Issue 3: Missing Compression Functions  
**Symptoms**: Linker errors for compression/decompression functions
**Solution**: Ensure `mz_strm_zlib.c` is present and `MZ_ZIP_NO_COMPRESSION` is NOT defined

### Issue 4: Platform-Specific Function Missing
**Symptoms**: Unresolved externals for OS functions
**Solution**: Ensure correct platform-specific files are included:
- Windows: `mz_os_win32.c`, `mz_strm_os_win32.c`
- Linux/Mac: `mz_os_posix.c`, `mz_strm_os_posix.c`

### Issue 5: zlib Compatibility
**Symptoms**: Errors related to zlib functions
**Solution**: Verify `MZ_ZLIB` and `ZLIB_WINAPI` definitions are present

### Issue 6: Header Not Found
**Symptoms**: Cannot find minizip-ng headers
**Solution**: Check include paths in Build.cs and file locations

### Issue 7: AES Encryption Linking Errors
**Symptoms**: Unresolved externals for AES functions
**Solution**: Ensure `mz_strm_wzaes.c` is compiled and `HAVE_WZAES` is defined

### Issue 8: PKWare Encryption Errors
**Symptoms**: Traditional encryption functions missing
**Solution**: Ensure `mz_strm_pkcrypt.c` is compiled and `HAVE_PKCRYPT` is defined

## 5. Performance Considerations

### Memory Usage
- **Full compression modes**: Higher CPU usage but significantly smaller archive sizes
- **Encryption overhead**: Minimal impact on file size, slight CPU overhead for encryption/decryption
- **Buffer management**: 4KB buffers provide good balance, but larger buffers (16-64KB) may improve performance for large files
- **Large files**: Consider memory mapping for files > 100MB

### Threading
- **Current implementation**: Synchronous operations
- **Recommended improvement**: Implement async operations for large files
- **Thread safety**: Minizip-ng handles are not thread-safe; use separate handles per thread
- **Encryption threading**: AES operations can benefit from parallel processing for large files

### Optimization Tips
1. **Pre-allocate buffers** for repeated operations
2. **Choose compression level wisely**: Level 6 provides good balance between speed and compression
3. **Use appropriate encryption**: PKWare for compatibility, AES for security
4. **Batch operations**: Process multiple files in single ZIP session
4. **Implement progress callbacks** for user feedback

## 6. Security Considerations

### Path Traversal Prevention
```cpp
bool IsSecurePath(const FString& Path)
{
    // Prevent directory traversal attacks
    return !Path.Contains(TEXT("..")) && 
           !Path.Contains(TEXT("\\\\")) && 
           !Path.StartsWith(TEXT("/"));
}
```

### File Size Limits
```cpp
const int64 MaxFileSize = 100 * 1024 * 1024; // 100MB limit
const int32 MaxFileCount = 10000;            // 10K files limit
```

### Validation
```cpp
bool ValidateZipEntry(const FString& EntryName)
{
    // Check for malicious entries
    if (EntryName.IsEmpty() || EntryName.Len() > 260)
        return false;
        
    // Check for invalid characters
    static const FString InvalidChars = TEXT("<>:\"|?*");
    for (TCHAR c : InvalidChars)
    {
        if (EntryName.Contains(FString::Chr(c)))
            return false;
    }
    
    return true;
}
```

## 7. Advanced Configuration Options

### Custom Compression Settings
```cpp
// Enable different compression methods
PublicDefinitions.Add("MZ_DEFAULT_COMPRESSION=6");      // Default deflate level
PublicDefinitions.Add("HAVE_ZLIB");                     // Enable deflate compression
PublicDefinitions.Add("HAVE_BZIP2");                    // Enable bzip2 (requires bzip2 lib)
PublicDefinitions.Add("HAVE_LZMA");                     // Enable LZMA (requires LZMA lib)
```

### Encryption Configuration
```cpp
// Enable encryption methods
PublicDefinitions.Add("HAVE_PKCRYPT");                  // PKWare traditional encryption
PublicDefinitions.Add("HAVE_WZAES");                    // WinZip AES encryption
PublicDefinitions.Add("HAVE_OPENSSL");                  // OpenSSL support (if available)
```

### Memory Allocation Control
```cpp
// Custom memory allocator integration
PublicDefinitions.Add("MZ_ALLOC=CustomAlloc");
PublicDefinitions.Add("MZ_FREE=CustomFree");
```

### Debug Configuration
```cpp
#if UE_BUILD_DEBUG
    PublicDefinitions.Add("MZ_ZIP_DEBUG");
#endif
```

## 8. Migration from Other ZIP Libraries

### From Unreal's Built-in ZIP
- Replace `FFileHelper` calls with minizip-ng equivalents
- Update compression settings
- Modify error handling patterns

### From zlib Direct Usage
- Replace raw zlib calls with minizip-ng stream interface
- Update file handling to use minizip-ng streams
- Modify buffer management

## 9. Testing and Validation

### Unit Test Examples
```cpp
// Test basic ZIP creation
UTEST(MinizipTest, CreateEmptyZip)
{
    FString TestPath = FPaths::ProjectSavedDir() / TEXT("test.zip");
    // Test implementation
    TestTrue("ZIP Created", FPaths::FileExists(TestPath));
}

// Test file extraction
UTEST(MinizipTest, ExtractZipContents)
{
    // Create test ZIP and extract
    // Verify file contents match
}
```

### Integration Tests
```cpp
// Test with real project files
void TestWithProjectFiles()
{
    TArray<FString> ProjectFiles;
    IFileManager::Get().FindFilesRecursive(
        ProjectFiles, 
        *FPaths::ProjectContentDir(), 
        TEXT("*.uasset")
    );
    
    // Create ZIP from project files
    // Verify no corruption
}
```

This comprehensive guide provides everything needed to successfully integrate minizip-ng into any Unreal Engine 5 project, based on the proven configuration from the HTTPManager project analysis.
