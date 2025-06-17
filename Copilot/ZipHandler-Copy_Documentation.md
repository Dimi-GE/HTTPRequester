# ZipHandler-Copy.cpp Documentation

## Overview
ZipHandler-Copy.cpp provides comprehensive ZIP file handling capabilities for Unreal Engine 5 using the minizip-ng library. This module enables creating ZIP archives from multiple files while preserving directory structure, and extracting ZIP archives to specified locations. The implementation focuses on store-only compression (no compression) for optimal performance and compatibility.

## 1. Required Modules and External Dependencies

### Core Unreal Engine Modules
```cpp
// File and Path Management
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
```

### Third-Party Library Dependencies
```cpp
// Minizip-ng Library Headers
#include "ThirdParty/minizip-ng/include/mz.h"         // Core minizip definitions
#include "ThirdParty/minizip-ng/include/mz_os.h"      // OS abstraction layer
#include "ThirdParty/minizip-ng/include/mz_zip.h"     // ZIP file operations
#include "ThirdParty/minizip-ng/include/mz_strm.h"    // Stream interface
#include "ThirdParty/minizip-ng/include/mz_strm_os.h" // OS-specific streams
```

### Build.cs Configuration Required
```csharp
// Path to minizip-ng
string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "minizip-ng");
string IncludePath = Path.Combine(ThirdPartyPath, "include");
string SourcePath = Path.Combine(ModuleDirectory, "minizip-ng");

// Add include paths
PublicIncludePaths.Add(IncludePath);
PrivateIncludePaths.Add(SourcePath);

// Essential definitions for minizip-ng
PublicDefinitions.Add("MZ_COMPAT");          // Compatibility mode
PublicDefinitions.Add("MZ_ZIP_NO_CRYPTO");   // Disable encryption
PublicDefinitions.Add("MZ_ZLIB");            // Use Unreal's zlib
PublicDefinitions.Add("ZLIB_WINAPI");        // zlib compatibility
PublicDefinitions.Add("MZ_ZIP_NO_COMPRESSION"); // Store-only mode

// Dependencies
PublicDependencyModuleNames.AddRange(new string[] { "zlib" });

// Disable unity build to prevent C file compilation issues
bUseUnity = false;
```

### Required Source Files in Module
Place these files in `YourModule/Source/YourModule/minizip-ng/`:
- `mz_zip.c` - Core ZIP functionality
- `mz_os.c` - OS abstraction
- `mz_strm.c` - Stream handling
- `mz_strm_buf.c` - Buffered streams
- `mz_strm_mem.c` - Memory streams
- `mz_os_win32.c` - Windows OS implementation
- `mz_strm_os_win32.c` - Windows stream implementation
- `mz_crypt_wrapper.c` - CRC32 wrapper for Unreal's zlib

## 2. Function Documentation with Comments

### Core ZIP Creation Functions

#### `CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath)`
**Purpose**: Creates a ZIP archive from a list of file paths, storing only filenames (no directory structure).
```cpp
void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath)
{
    // Step 1: Initialize OS stream for file operations
    void* FileStream = mz_stream_os_create();
    
    // Step 1a: Validate stream creation
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Step 2: Open the output ZIP file stream
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Create and initialize ZIP handle
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_WRITE) != MZ_OK)
    {
        // Step 3a: Cleanup on failure
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for writing - returning. . ."))
        return;
    }

    // Step 4: Process each input file
    for (const FString& FilePath : FilePaths)
    {
        // Step 4a: Create file entry stream
        void* FileEntryStream = mz_stream_os_create();

        // Step 4b: Open source file for reading
        if (mz_stream_open(FileEntryStream, TCHAR_TO_UTF8(*FilePath), MZ_OPEN_MODE_READ) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open file %s - skipping..."), *FilePath);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        // Step 4c: Extract filename for ZIP entry (no path preservation)
        FString FileName = FPaths::GetCleanFilename(FilePath);

        // Step 4d: Configure ZIP entry metadata
        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        FileMeta.filename = TCHAR_TO_ANSI(*FileName);
        FileMeta.modified_date = time(nullptr);

        // Step 4e: Begin writing ZIP entry
        if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_STORE, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open zip entry for %s - skipping..."), *FilePath);
            mz_stream_close(FileEntryStream);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        // Step 4f: Copy file content to ZIP using buffered I/O
        const int32 BufferSize = 4096;
        uint8 Buffer[BufferSize];

        int32 BytesRead = 0;
        do
        {
            BytesRead = mz_stream_read(FileEntryStream, Buffer, BufferSize);
            if (BytesRead < 0) break; // Error reading
            if (mz_zip_entry_write(ZipHandle, Buffer, BytesRead) != BytesRead) break; // Error writing
        } while (BytesRead > 0);

        // Step 4g: Finalize entry and cleanup
        mz_zip_entry_close(ZipHandle);
        mz_stream_close(FileEntryStream);
        mz_stream_delete(&FileEntryStream);
    }

    // Step 5: Finalize ZIP archive and cleanup all resources
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);
    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("ZipHandler::ZIP archive created successfully at %s"), *ZipPath);
}
```

#### `CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath)`
**Purpose**: Creates a ZIP archive preserving directory structure using full path and relative path pairs.
```cpp
void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath)
{
    // Step 1: Initialize file stream (same as CreateZip)
    void* FileStream = mz_stream_os_create();
    
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Step 2: Open ZIP file for writing (same pattern as CreateZip)
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Initialize ZIP handle (same pattern as CreateZip)
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_WRITE) != MZ_OK)
    {
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for writing - returning. . ."))
        return;
    }

    // Step 4: Process each file with structure preservation
    for (const TPair<FString, FString>& FilePair : FilesStructure)
    {
        const FString& FullPath = FilePair.Key;      // Absolute file path
        const FString& RelativePath = FilePair.Value; // Path within ZIP archive

        // Step 4a: Open source file
        void* FileEntryStream = mz_stream_os_create();
        if (mz_stream_open(FileEntryStream, TCHAR_TO_UTF8(*FullPath), MZ_OPEN_MODE_READ) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open file %s - skipping..."), *FullPath);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        // Step 4b: Convert relative path to UTF8 for ZIP entry
        FTCHARToUTF8 EntryNameUTF8(*RelativePath);

        // Step 4c: Configure ZIP entry with relative path
        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        FileMeta.filename = EntryNameUTF8.Get(); // Use relative path for structure preservation
        FileMeta.modified_date = time(nullptr);

        // Step 4d-4g: Same file processing logic as CreateZip
        if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_STORE, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open zip entry for %s - skipping..."), *FullPath);
            mz_stream_close(FileEntryStream);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        // Buffered file copy (same as CreateZip)
        const int32 BufferSize = 4096;
        uint8 Buffer[BufferSize];

        int32 BytesRead = 0;
        do
        {
            BytesRead = mz_stream_read(FileEntryStream, Buffer, BufferSize);
            if (BytesRead < 0) break;
            if (mz_zip_entry_write(ZipHandle, Buffer, BytesRead) != BytesRead) break;
        } while (BytesRead > 0);

        mz_zip_entry_close(ZipHandle);
        mz_stream_close(FileEntryStream);
        mz_stream_delete(&FileEntryStream);
    }

    // Step 5: Finalize (same as CreateZip)
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);
    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("\nZipHandler::ZIP archive created successfully at %s"), *ZipPath);
}
```

### Utility Functions

#### `CollectFilesForZip_UTIL(const FString& RootFolder)`
**Purpose**: Recursively collects all files in a directory and returns them as full path and relative path pairs.
```cpp
TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder)
{
    TArray<TPair<FString, FString>> FilesToZip;

    // Step 1: Normalize root folder path for consistent handling
    FString NormalizedRoot = RootFolder;
    FPaths::NormalizeDirectoryName(NormalizedRoot);

    // Step 2: Find all files recursively
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFilesRecursive(
        FoundFiles,
        *NormalizedRoot,
        TEXT("*.*"),
        true,   // Include files
        false   // Exclude directories
    );

    // Step 3: Process each found file to create relative paths
    for (const FString& FilePath : FoundFiles)
    {
        // Step 3a: Convert absolute path to relative path
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *NormalizedRoot);
        
        // Step 3b: Store as full path + relative path pair
        FilesToZip.Add(TPair<FString, FString>(FilePath, RelativePath));
    }

    // Step 4: Sort by relative path for consistent ZIP ordering
    FilesToZip.Sort([](const TPair<FString, FString>& A, const TPair<FString, FString>& B)
    {
        return A.Value < B.Value; // Sort by relative path
    });

    return FilesToZip;
}
```

### ZIP Extraction Functions

#### `UnpackZip(const FString& ZipPath, const FString& TempDir)`
**Purpose**: Extracts all contents of a ZIP archive to a specified directory.
```cpp
void UnpackZip(const FString& ZipPath, const FString& TempDir)
{
    // Step 1: Initialize file stream for reading
    void* FileStream = mz_stream_os_create();
    
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Step 2: Open ZIP file for reading
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_READ);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Create and open ZIP handle for reading
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_READ) != MZ_OK)
    {
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for writing - returning. . ."))
        return;
    }

    // Step 4: Normalize target directory for consistent path handling
    FString NormalizedTargetDir = FPaths::ConvertRelativePathToFull(TempDir);
    FPaths::NormalizeDirectoryName(NormalizedTargetDir);

    // Step 5: Iterate through all ZIP entries
    int32 EntryResult = mz_zip_goto_first_entry(ZipHandle);

    while (EntryResult == MZ_OK)
    {
        // Step 5a: Get current entry information
        mz_zip_file* FileInfo = nullptr;
        if (mz_zip_entry_get_info(ZipHandle, &FileInfo) == MZ_OK && FileInfo != nullptr)
        {
            // Step 5b: Convert entry name to Unreal string
            FString EntryName = UTF8_TO_TCHAR(FileInfo->filename);
            FString OutputPath = FPaths::Combine(NormalizedTargetDir, EntryName);
            
            UE_LOG(LogTemp, Display, TEXT("Found ZIP Entry: %s"), *EntryName);

            // Step 5c: Handle directory entries
            if (EntryName.EndsWith("/"))
            {
                IFileManager::Get().MakeDirectory(*OutputPath, true);
            }
            else
            {
                // Step 5d: Handle file entries
                // Ensure parent directory exists
                FString DirectoryPath = FPaths::GetPath(OutputPath);
                IFileManager::Get().MakeDirectory(*DirectoryPath, true);

                // Step 5e: Extract file content
                if (mz_zip_entry_read_open(ZipHandle, 0, nullptr) == MZ_OK)
                {
                    // Step 5f: Set up buffered extraction
                    const int32 BufferSize = 4096;
                    uint8 Buffer[BufferSize];

                    // Step 5g: Create output file
                    FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutputPath);

                    if (FileWriter)
                    {
                        // Step 5h: Read and write file content in chunks
                        int32 BytesRead = 0;
                        do
                        {
                            BytesRead = mz_zip_entry_read(ZipHandle, Buffer, BufferSize);
                            if (BytesRead < 0) break; // Error reading
                            FileWriter->Serialize(Buffer, BytesRead);
                        } while (BytesRead > 0);

                        // Step 5i: Finalize output file
                        FileWriter->Close();
                        delete FileWriter;

                        UE_LOG(LogTemp, Display, TEXT("Extracted: %s"), *EntryName);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to create file: %s"), *OutputPath);
                    }
                    mz_zip_entry_close(ZipHandle);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Could not open entry: %s"), *EntryName);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to get entry info."));
        }

        // Step 5j: Move to next entry
        EntryResult = mz_zip_goto_next_entry(ZipHandle);
    }

    // Step 6: Cleanup resources
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);
    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("\nZipHandler::ZIP archive unpacked successfully at %s"), *TempDir);
}
```

## 3. Potential Improvements

### Function-Level Improvements

#### Resource Management
**Current Issue**: Manual resource cleanup with potential memory leaks
**Improvement**: RAII wrapper classes
```cpp
// RAII wrapper for minizip streams
class FMinizipStreamRAII
{
public:
    FMinizipStreamRAII() : Stream(mz_stream_os_create()) {}
    ~FMinizipStreamRAII() { if (Stream) { mz_stream_delete(&Stream); } }
    
    void* Get() const { return Stream; }
    bool IsValid() const { return Stream != nullptr; }
    
private:
    void* Stream;
    // Non-copyable
    FMinizipStreamRAII(const FMinizipStreamRAII&) = delete;
    FMinizipStreamRAII& operator=(const FMinizipStreamRAII&) = delete;
};
```

#### Error Handling Standardization
**Current Issue**: Inconsistent error reporting
**Improvement**: Structured error handling
```cpp
enum class EZipOperationResult
{
    Success,
    StreamCreationFailed,
    FileOpenFailed,
    ZipHandleCreationFailed,
    EntryWriteFailed,
    EntryReadFailed
};

struct FZipOperationResult
{
    EZipOperationResult Result;
    FString ErrorMessage;
    int32 ProcessedFiles;
    
    bool IsSuccess() const { return Result == EZipOperationResult::Success; }
};
```

#### Buffer Size Optimization
**Current Issue**: Fixed 4KB buffer size may not be optimal
**Improvement**: Adaptive buffer sizing
```cpp
class FAdaptiveBuffer
{
    static constexpr int32 MinBufferSize = 4096;
    static constexpr int32 MaxBufferSize = 64 * 1024; // 64KB
    
public:
    int32 GetOptimalBufferSize(int64 FileSize) const
    {
        if (FileSize < MinBufferSize) return MinBufferSize;
        if (FileSize > MaxBufferSize) return MaxBufferSize;
        return FMath::RoundUpToPowerOfTwo(static_cast<int32>(FileSize / 16));
    }
};
```

#### Compression Method Configuration
**Current Issue**: Hardcoded to store-only mode
**Improvement**: Configurable compression levels
```cpp
enum class ECompressionLevel
{
    None = MZ_COMPRESS_METHOD_STORE,
    Fast = MZ_COMPRESS_LEVEL_FAST,
    Normal = MZ_COMPRESS_LEVEL_NORMAL,
    Best = MZ_COMPRESS_LEVEL_BEST
};

void CreateZipWithCompression(const TArray<FString>& FilePaths, const FString& ZipPath, ECompressionLevel CompressionLevel);
```

### Performance Improvements

#### Asynchronous Operations
**Current Issue**: Blocking operations on main thread
**Improvement**: Async task implementation
```cpp
class FAsyncZipTask : public FNonAbandonableTask
{
public:
    FAsyncZipTask(TArray<FString> InFilePaths, FString InZipPath)
        : FilePaths(MoveTemp(InFilePaths)), ZipPath(MoveTemp(InZipPath)) {}
    
    void DoWork()
    {
        CreateZip(FilePaths, ZipPath);
    }
    
    FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncZipTask, STATGROUP_ThreadPoolAsyncTasks); }
    
private:
    TArray<FString> FilePaths;
    FString ZipPath;
};

// Usage
auto Task = new FAsyncTask<FAsyncZipTask>(FilePaths, ZipPath);
Task->StartBackgroundTask();
```

#### Memory Mapping for Large Files
**Current Issue**: Large files loaded entirely into memory
**Improvement**: Memory-mapped file I/O
```cpp
void ProcessLargeFile(const FString& FilePath, void* ZipHandle)
{
    if (IFileManager::Get().FileSize(*FilePath) > LargeFileThreshold)
    {
        // Use memory mapping for large files
        ProcessFileMemoryMapped(FilePath, ZipHandle);
    }
    else
    {
        // Use standard buffered I/O for small files
        ProcessFileBuffered(FilePath, ZipHandle);
    }
}
```

### Overall Architecture Improvements

#### Callback System for Progress Reporting
**Current Issue**: No progress feedback for long operations
**Improvement**: Progress callback system
```cpp
DECLARE_DELEGATE_ThreeParams(FZipProgressDelegate, int32 /*ProcessedFiles*/, int32 /*TotalFiles*/, const FString& /*CurrentFile*/);

void CreateZipWithProgress(const TArray<FString>& FilePaths, const FString& ZipPath, FZipProgressDelegate ProgressCallback);
```

#### Configuration Object
**Current Issue**: Hardcoded settings throughout
**Improvement**: Centralized configuration
```cpp
struct FZipHandlerConfig
{
    int32 BufferSize = 4096;
    ECompressionLevel DefaultCompression = ECompressionLevel::None;
    bool bPreserveTimestamps = true;
    bool bCreateDirectories = true;
    FString TempDirectory = FPaths::ProjectSavedDir() / TEXT("Temp");
};
```

#### Validation and Sanitization
**Current Issue**: Limited input validation
**Improvement**: Comprehensive validation
```cpp
class FZipPathValidator
{
public:
    static bool IsValidZipPath(const FString& Path);
    static bool IsValidFileName(const FString& FileName);
    static FString SanitizeFileName(const FString& FileName);
    static bool IsPathSecure(const FString& Path); // Prevent path traversal attacks
};
```

#### Unit Testing Support
**Current Issue**: No testing infrastructure
**Improvement**: Testable interface design
```cpp
class IZipHandler
{
public:
    virtual ~IZipHandler() = default;
    virtual FZipOperationResult CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath) = 0;
    virtual FZipOperationResult ExtractZip(const FString& ZipPath, const FString& ExtractPath) = 0;
};

class FMinizipHandler : public IZipHandler
{
    // Implementation using minizip-ng
};

class FMockZipHandler : public IZipHandler
{
    // Mock implementation for testing
};
```

## 4. Usage Examples

### Basic ZIP Creation
```cpp
// Simple file compression
TArray<FString> FilesToCompress = {
    TEXT("C:/MyProject/File1.txt"),
    TEXT("C:/MyProject/File2.txt")
};
CreateZip(FilesToCompress, TEXT("C:/Output/MyArchive.zip"));
```

### Structured ZIP Creation
```cpp
// Preserve directory structure
TArray<TPair<FString, FString>> StructuredFiles = CollectFilesForZip_UTIL(TEXT("C:/MyProject/Source"));
CreateZip_Structured(StructuredFiles, TEXT("C:/Output/SourceCode.zip"));
```

### ZIP Extraction
```cpp
// Extract ZIP contents
UnpackZip(TEXT("C:/Input/Archive.zip"), TEXT("C:/Extract/Destination"));
```

### Directory Collection
```cpp
// Collect files from directory
auto FilesToZip = CollectFilesForZip_UTIL(TEXT("C:/MyProject"));
UE_LOG(LogTemp, Warning, TEXT("Found %d files to compress"), FilesToZip.Num());
```

This documentation provides a complete reference for the ZipHandler-Copy.cpp functionality, including integration requirements, detailed implementation explanations, and production-ready improvement suggestions.
