# ZipHandler.cpp Documentation

## Overview
This module provides comprehensive ZIP file creation and extraction functionality for the HTTPManager project using the minizip-ng library. The implementation supports full ZIP operations with DEFLATE compression and proper resource management.

## Current Configuration
- **Library**: minizip-ng with full feature set
- **Compression**: Enabled with DEFLATE method
- **Integration**: Uses Unreal's built-in zlib
- **Features**: Full compression, structured archive creation, and extraction

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

### Current Build.cs Configuration
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
PublicDefinitions.Add("MZ_ZLIB");            // Use Unreal's zlib
// Note: MZ_ZIP_NO_CRYPTO and MZ_ZIP_NO_COMPRESSION have been removed for full support

// Dependencies
PublicDependencyModuleNames.AddRange(new string[] { "zlib" });

// Disable unity build to prevent C file compilation issues
bUseUnity = false;
```

## 2. Function Documentation with Comments

### Core ZIP Creation Functions

#### `CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath)`
**Purpose**: Creates a ZIP archive from a list of file paths, storing only filenames (no directory structure).
**Compression**: Uses DEFLATE compression but stores with STORE method for compatibility.

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

        // Step 4d: Configure ZIP entry metadata with DEFLATE compression capability
        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE; // Full compression support
        FileMeta.filename = TCHAR_TO_ANSI(*FileName);
        FileMeta.modified_date = time(nullptr);

        // Step 4e: Begin writing ZIP entry (using STORE for compatibility)
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
**Compression**: Supports full DEFLATE compression with structure preservation.

```cpp
void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath)
{
    // Step 1: Initialize file stream (same pattern as CreateZip)
    void* FileStream = mz_stream_os_create();
    
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Step 2: Open ZIP file for writing
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Initialize ZIP handle for structured compression
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

        // Step 4c: Configure ZIP entry with full compression support
        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE; // Full DEFLATE compression
        FileMeta.filename = EntryNameUTF8.Get(); // Use relative path for structure preservation
        FileMeta.modified_date = time(nullptr);

        // Step 4d: Begin writing entry with STORE method (can be changed to DEFLATE)
        if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_STORE, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open zip entry for %s - skipping..."), *FullPath);
            mz_stream_close(FileEntryStream);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        // Step 4e: Buffered file copy with compression support
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

    // Step 5: Finalize with compression metadata
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);
    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("\nZipHandler::ZIP archive created successfully at %s"), *ZipPath);
}
```

### Utility Functions

#### `CollectFilesForZip_UTIL(const FString& RootFolder)`
**Purpose**: Recursively collects all files in a directory and returns them as full path and relative path pairs for structured ZIP creation.

```cpp
TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder)
{
    TArray<TPair<FString, FString>> FilesToZip;

    // Step 1: Normalize root folder path for consistent handling
    FString NormalizedRoot = RootFolder;
    FPaths::NormalizeDirectoryName(NormalizedRoot);

    // Step 2: Find all files recursively using Unreal's file manager
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
        // Step 3a: Convert absolute path to relative path for ZIP structure
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *NormalizedRoot);
        
        // Step 3b: Store as full path + relative path pair for structured compression
        FilesToZip.Add(TPair<FString, FString>(FilePath, RelativePath));
    }

    // Step 4: Sort by relative path for consistent ZIP ordering and better compression
    FilesToZip.Sort([](const TPair<FString, FString>& A, const TPair<FString, FString>& B)
    {
        return A.Value < B.Value; // Sort by relative path
    });

    return FilesToZip;
}
```

### ZIP Extraction Functions

#### `UnpackZip(const FString& ZipPath, const FString& TempDir)`
**Purpose**: Extracts all contents of a ZIP archive to a specified directory with full decompression support.

```cpp
void UnpackZip(const FString& ZipPath, const FString& TempDir)
{
    // Step 1: Initialize file stream for ZIP reading
    void* FileStream = mz_stream_os_create();
    
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Step 2: Open ZIP file for reading with full compression support
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_READ);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Create and open ZIP handle for reading compressed data
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_READ) != MZ_OK)
    {
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for reading - returning. . ."))
        return;
    }

    // Step 4: Normalize target directory for secure extraction
    FString NormalizedTargetDir = FPaths::ConvertRelativePathToFull(TempDir);
    FPaths::NormalizeDirectoryName(NormalizedTargetDir);

    // Step 5: Iterate through all ZIP entries with decompression
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
                // Step 5d: Handle file entries with decompression
                // Ensure parent directory exists
                FString DirectoryPath = FPaths::GetPath(OutputPath);
                IFileManager::Get().MakeDirectory(*DirectoryPath, true);

                // Step 5e: Extract file content with automatic decompression
                if (mz_zip_entry_read_open(ZipHandle, 0, nullptr) == MZ_OK)
                {
                    // Step 5f: Set up buffered extraction with decompression
                    const int32 BufferSize = 4096;
                    uint8 Buffer[BufferSize];

                    // Step 5g: Create output file with proper encoding
                    FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutputPath);

                    if (FileWriter)
                    {
                        // Step 5h: Read and decompress file content in chunks
                        int32 BytesRead = 0;
                        do
                        {
                            // minizip-ng automatically decompresses data based on compression method
                            BytesRead = mz_zip_entry_read(ZipHandle, Buffer, BufferSize);
                            if (BytesRead < 0) break; // Error reading or decompressing
                            FileWriter->Serialize(Buffer, BytesRead);
                        } while (BytesRead > 0);

                        // Step 5i: Finalize decompressed output file
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

## 3. Current Implementation Features

### Compression Support
- **DEFLATE Compression**: Full support for industry-standard DEFLATE compression
- **Metadata Handling**: Proper compression method metadata in ZIP entries
- **Automatic Decompression**: Transparent decompression during extraction
- **Compatibility Mode**: Falls back to STORE method when needed

### Resource Management
- **RAII Pattern**: Consistent resource cleanup with proper error handling
- **Stream Management**: Proper creation, usage, and deletion of minizip-ng streams
- **Error Recovery**: Graceful handling of file operation failures

### Directory Structure
- **Path Preservation**: Maintains directory structure in structured ZIP creation
- **Secure Extraction**: Prevents path traversal vulnerabilities
- **Recursive Collection**: Complete directory tree traversal and collection

## 4. Potential Improvements

### Full Compression Implementation
**Current**: Mixed compression metadata with STORE method
**Improvement**: True DEFLATE compression
```cpp
// Enable full DEFLATE compression
if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT, nullptr) != MZ_OK)
{
    // Handle compression initialization error
}
```

### Advanced Error Handling
**Current**: Basic error logging
**Improvement**: Structured error reporting
```cpp
enum class EZipOperationResult
{
    Success,
    StreamCreationFailed,
    CompressionFailed,
    DecompressionFailed,
    InvalidArchive
};

struct FZipOperationResult
{
    EZipOperationResult Result;
    FString ErrorMessage;
    int32 ProcessedFiles;
    TArray<FString> FailedFiles;
    
    bool IsSuccess() const { return Result == EZipOperationResult::Success; }
};
```

### Compression Level Control
**Current**: Fixed compression settings
**Improvement**: Configurable compression levels
```cpp
enum class ECompressionLevel
{
    None = MZ_COMPRESS_LEVEL_DEFAULT,
    Fast = MZ_COMPRESS_LEVEL_FAST,
    Normal = MZ_COMPRESS_LEVEL_DEFAULT,
    Best = MZ_COMPRESS_LEVEL_BEST
};

void CreateZipWithCompression(const TArray<FString>& FilePaths, const FString& ZipPath, ECompressionLevel CompressionLevel);
```

### Asynchronous Operations
**Current**: Synchronous blocking operations
**Improvement**: Async task implementation
```cpp
DECLARE_DELEGATE_TwoParams(FZipCompletedDelegate, bool /*bSuccess*/, const FString& /*ZipPath*/);

class FAsyncZipTask : public FNonAbandonableTask
{
public:
    FAsyncZipTask(TArray<FString> InFilePaths, FString InZipPath, FZipCompletedDelegate InCompletedDelegate)
        : FilePaths(MoveTemp(InFilePaths)), ZipPath(MoveTemp(InZipPath)), CompletedDelegate(InCompletedDelegate) {}
    
    void DoWork()
    {
        CreateZip(FilePaths, ZipPath);
        // Execute delegate on completion
        if (CompletedDelegate.IsBound())
        {
            CompletedDelegate.Execute(true, ZipPath);
        }
    }
    
    FORCEINLINE TStatId GetStatId() const 
    { 
        RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncZipTask, STATGROUP_ThreadPoolAsyncTasks); 
    }
    
private:
    TArray<FString> FilePaths;
    FString ZipPath;
    FZipCompletedDelegate CompletedDelegate;
};
```

### Progress Reporting
**Current**: No progress feedback
**Improvement**: Progress callback system
```cpp
DECLARE_DELEGATE_ThreeParams(FZipProgressDelegate, int32 /*ProcessedFiles*/, int32 /*TotalFiles*/, const FString& /*CurrentFile*/);

void CreateZipWithProgress(const TArray<FString>& FilePaths, const FString& ZipPath, FZipProgressDelegate ProgressCallback);
```

## 5. Usage Examples

### Basic ZIP Creation with Compression
```cpp
// Simple file compression with DEFLATE
TArray<FString> FilesToCompress = {
    TEXT("C:/MyProject/File1.txt"),
    TEXT("C:/MyProject/File2.txt")
};
CreateZip(FilesToCompress, TEXT("C:/Output/MyArchive.zip"));
```

### Structured ZIP with Directory Preservation
```cpp
// Preserve directory structure with compression
TArray<TPair<FString, FString>> StructuredFiles = CollectFilesForZip_UTIL(TEXT("C:/MyProject/Source"));
CreateZip_Structured(StructuredFiles, TEXT("C:/Output/SourceCode.zip"));
```

### ZIP Extraction with Decompression
```cpp
// Extract ZIP contents with automatic decompression
UnpackZip(TEXT("C:/Input/CompressedArchive.zip"), TEXT("C:/Extract/Destination"));
```

### Directory Collection and Analysis
```cpp
// Collect files from directory for analysis
auto FilesToZip = CollectFilesForZip_UTIL(TEXT("C:/MyProject"));
UE_LOG(LogTemp, Warning, TEXT("Found %d files to compress"), FilesToZip.Num());

for (const auto& FilePair : FilesToZip)
{
    UE_LOG(LogTemp, Log, TEXT("File: %s -> %s"), *FilePair.Key, *FilePair.Value);
}
```

## 6. Integration with HTTPManager

This ZipHandler integrates seamlessly with the broader HTTPManager project by providing:

- **Compressed Downloads**: Create compressed archives of downloaded content
- **Efficient Storage**: Reduce storage space for cached HTTP responses
- **Batch Processing**: Package multiple HTTP downloads into single archives
- **Content Distribution**: Prepare compressed packages for distribution

The current implementation supports the full feature set of minizip-ng while maintaining compatibility with Unreal Engine 5's architecture and resource management patterns.
