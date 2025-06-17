# CustomUtilities.cpp Documentation

## Overview
This module provides comprehensive utility functions for the HTTPManager project, integrating JSON handling, file operations, material management, timer utilities, and ZIP functionality. The implementation supports RSS manifest generation, file hashing, dynamic material instances, and external ZIP operations integration.

## Current Configuration
- **JSON Support**: Full JSON parsing and generation with RSS manifest capabilities
- **File Operations**: Hash calculation for files and directories using MD5
- **Material System**: Dynamic material instance creation with parameter control
- **Timer Management**: World context-aware timer utilities
- **ZIP Integration**: External ZIP handler integration for compression operations

## 1. Required Modules and External Dependencies

### Core Unreal Engine Modules
```cpp
// Materials Interfaces
#include "Materials/MaterialInstanceDynamic.h"
// Engine
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
// File management
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
// Messaging
#include "Misc/MessageDialog.h"
// JSON
#include "Json.h"
#include "JsonUtilities.h"
```

### External ZIP Handler Integration
```cpp
// ZIP Handler External Functions
extern void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath);
extern void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath);
extern void UnpackZip(const FString& PathToZip, const FString& TempDir);
extern TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder);
```

## 2. Function Documentation with Comments

### Material System Functions

#### `ThrowDynamicInstance(float ScalarValue)`
**Purpose**: Creates a dynamic material instance from a hardcoded base material with configurable scalar parameter.
**Material Path**: `/Game/Mats/UMG/M_SyncNotify.M_SyncNotify`

```cpp
UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue)
{
    // Step 1: Define static material path for M_SyncNotify material
    static FSoftObjectPath MatPath(TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify"));
    
    // Step 2: Attempt to load the base material interface
    UMaterialInterface* BaseMat = Cast<UMaterialInterface>(MatPath.TryLoad());
    if (!BaseMat) return nullptr; // Return null if material loading fails

    // Step 3: Create dynamic material instance from base material
    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, nullptr);
    if (DynMat)
    {
        // Step 4: Set scalar parameter "SyncState" with provided value
        DynMat->SetScalarParameterValue(FName("SyncState"), ScalarValue);
    }

    return DynMat; // Return configured dynamic material instance
}
```

### Dialog and UI Functions

#### `ThrowDialogMessage(FString Message)`
**Purpose**: Displays message dialogs with different types for user interaction.
**Note**: Currently investigates action binding based on user interaction.

```cpp
void ThrowDialogMessage(FString Message)
{
    // Step 1: Display informational message dialog
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
    
    // TODO: Investigate binding actions based on user interaction
    // Potential improvements:
    // - Return dialog result for action handling
    // - Support different dialog types (Yes/No, OK/Cancel)
    // - Bind callbacks for different user responses
}
```

### JSON and RSS Manifest Functions

#### `ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWrite)`
**Purpose**: Handles RSS initialization module JSON operations with read/write capabilities.
**Parameters**: 
- `JSONSubPath`: Relative path to JSON file
- `ReadWrite`: Operation mode (0 = Read, 1 = Write)

```cpp
TSharedPtr<FJsonObject> ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWrite)
{
    // Step 1: Construct full JSON file path
    FString FullPath = FPaths::ProjectDir() + JSONSubPath;
    
    if (ReadWrite == 0) // Read Mode
    {
        // Step 2a: Read existing JSON file
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *FullPath))
        {
            // Step 2b: Parse JSON string into object
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
            
            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                return JsonObject; // Return parsed JSON object
            }
        }
        
        // Step 2c: Return null if reading/parsing fails
        UE_LOG(LogTemp, Warning, TEXT("Failed to read or parse JSON file: %s"), *FullPath);
        return nullptr;
    }
    else // Write Mode
    {
        // Step 3a: Create new JSON object for RSS initialization
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        
        // Step 3b: Add RSS manifest structure
        JsonObject->SetStringField(TEXT("Version"), TEXT("1.0"));
        JsonObject->SetStringField(TEXT("ProjectName"), TEXT("HTTPManager"));
        JsonObject->SetStringField(TEXT("InitDate"), FDateTime::Now().ToString());
        
        // Step 3c: Create RSS feeds array
        TArray<TSharedPtr<FJsonValue>> RSSFeeds;
        
        // Step 3d: Add default RSS feed entry
        TSharedPtr<FJsonObject> DefaultFeed = MakeShareable(new FJsonObject);
        DefaultFeed->SetStringField(TEXT("Name"), TEXT("Default Feed"));
        DefaultFeed->SetStringField(TEXT("URL"), TEXT(""));
        DefaultFeed->SetStringField(TEXT("UpdateInterval"), TEXT("3600")); // 1 hour
        
        RSSFeeds.Add(MakeShareable(new FJsonValueObject(DefaultFeed)));
        JsonObject->SetArrayField(TEXT("RSSFeeds"), RSSFeeds);
        
        // Step 3e: Write JSON object to file
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        
        if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
        {
            if (FFileHelper::SaveStringToFile(OutputString, *FullPath))
            {
                UE_LOG(LogTemp, Log, TEXT("RSS Init JSON written to: %s"), *FullPath);
                return JsonObject;
            }
        }
        
        // Step 3f: Log error if writing fails
        UE_LOG(LogTemp, Error, TEXT("Failed to write RSS Init JSON to: %s"), *FullPath);
        return nullptr;
    }
}
```

#### `ThrowJsonArrayFromFile_UTIL(FString JSONSubPath)`
**Purpose**: Reads and parses JSON array from file, typically for RSS feed collections.

```cpp
TArray<TSharedPtr<FJsonValue>> ThrowJsonArrayFromFile_UTIL(FString JSONSubPath)
{
    // Step 1: Construct full JSON file path
    FString FullPath = FPaths::ProjectDir() + JSONSubPath;
    
    // Step 2: Initialize empty array for return
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    
    // Step 3: Load JSON file content
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load JSON file: %s"), *FullPath);
        return JsonArray; // Return empty array
    }
    
    // Step 4: Parse JSON content
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // Step 5: Extract array field (assuming root object contains array)
        const TArray<TSharedPtr<FJsonValue>>* ArrayField;
        if (JsonObject->TryGetArrayField(TEXT("Items"), ArrayField) ||
            JsonObject->TryGetArrayField(TEXT("RSSFeeds"), ArrayField) ||
            JsonObject->TryGetArrayField(TEXT("Data"), ArrayField))
        {
            JsonArray = *ArrayField;
            UE_LOG(LogTemp, Log, TEXT("Loaded JSON array with %d items"), JsonArray.Num());
        }
        else
        {
            // Step 6: If no recognized array field, try parsing as root array
            TSharedRef<TJsonReader<>> ArrayReader = TJsonReaderFactory<>::Create(JsonString);
            TArray<TSharedPtr<FJsonValue>> DirectArray;
            
            if (FJsonSerializer::Deserialize(ArrayReader, DirectArray))
            {
                JsonArray = DirectArray;
                UE_LOG(LogTemp, Log, TEXT("Loaded direct JSON array with %d items"), JsonArray.Num());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON file: %s"), *FullPath);
    }
    
    return JsonArray;
}
```

### File System and Dialog Functions

#### `OpenFolderDialog_UTIL()`
**Purpose**: Opens native folder selection dialog for user directory selection.

```cpp
FString OpenFolderDialog_UTIL()
{
    // Step 1: Get desktop platform interface
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    
    if (DesktopPlatform)
    {
        // Step 2: Setup dialog parameters
        FString SelectedFolder;
        const FString DialogTitle = TEXT("Select Folder");
        const FString DefaultPath = FPaths::ProjectDir(); // Start from project directory
        
        // Step 3: Open folder selection dialog
        const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            DialogTitle,
            DefaultPath,
            SelectedFolder
        );
        
        // Step 4: Return selected folder or empty string
        if (bFolderSelected)
        {
            UE_LOG(LogTemp, Log, TEXT("Selected folder: %s"), *SelectedFolder);
            return SelectedFolder;
        }
    }
    
    // Step 5: Return empty string if dialog cancelled or failed
    UE_LOG(LogTemp, Warning, TEXT("Folder selection cancelled or failed"));
    return FString();
}
```

### File Hashing Functions

#### `CalculateFileHash_UTIL(const FString& FilePath)`
**Purpose**: Calculates MD5 hash for a single file for integrity verification.

```cpp
FString CalculateFileHash_UTIL(const FString& FilePath)
{
    // Step 1: Validate file existence
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("File does not exist: %s"), *FilePath);
        return FString();
    }
    
    // Step 2: Load file content into byte array
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file for hashing: %s"), *FilePath);
        return FString();
    }
    
    // Step 3: Calculate MD5 hash
    FMD5 MD5Context;
    MD5Context.Update(FileData.GetData(), FileData.Num());
    
    // Step 4: Finalize hash and convert to hex string
    FMD5Hash Hash;
    Hash.Set(MD5Context);
    
    FString HashString = LexToString(Hash);
    
    UE_LOG(LogTemp, Log, TEXT("File hash calculated for %s: %s"), *FPaths::GetCleanFilename(FilePath), *HashString);
    return HashString;
}
```

#### `CalculateDirectoryHash_UTIL(const TMap<FString, FString>& FileHashes)`
**Purpose**: Calculates combined hash for directory based on individual file hashes.

```cpp
FString CalculateDirectoryHash_UTIL(const TMap<FString, FString>& FileHashes)
{
    // Step 1: Validate input
    if (FileHashes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No file hashes provided for directory hash calculation"));
        return FString();
    }
    
    // Step 2: Create sorted array of file hashes for consistent ordering
    TArray<FString> SortedPaths;
    FileHashes.GetKeys(SortedPaths);
    SortedPaths.Sort(); // Sort paths alphabetically for consistent hash
    
    // Step 3: Concatenate all file hashes in sorted order
    FString CombinedHashes;
    for (const FString& FilePath : SortedPaths)
    {
        const FString* FileHash = FileHashes.Find(FilePath);
        if (FileHash)
        {
            // Include relative path and hash for better collision resistance
            FString RelativePath = FPaths::GetCleanFilename(FilePath);
            CombinedHashes += RelativePath + TEXT(":") + *FileHash + TEXT(";");
        }
    }
    
    // Step 4: Calculate hash of combined string
    FMD5 MD5Context;
    FTCHARToUTF8 UTF8String(*CombinedHashes);
    MD5Context.Update(reinterpret_cast<const uint8*>(UTF8String.Get()), UTF8String.Length());
    
    // Step 5: Finalize directory hash
    FMD5Hash DirectoryHash;
    DirectoryHash.Set(MD5Context);
    
    FString DirectoryHashString = LexToString(DirectoryHash);
    
    UE_LOG(LogTemp, Log, TEXT("Directory hash calculated from %d files: %s"), FileHashes.Num(), *DirectoryHashString);
    return DirectoryHashString;
}
```

### Timer Utility Functions

#### `ThrowTimer_UTIL()`
**Purpose**: Starts a general-purpose timer for utility operations.
**Note**: Requires world context for proper timer management.

```cpp
void ThrowTimer_UTIL()
{
    // Step 1: Get world context for timer management
    UWorld* World = nullptr;
    
    // Step 1a: Try to get world from game engine
    if (GEngine && GEngine->GetWorldContexts().Num() > 0)
    {
        World = GEngine->GetWorldContexts()[0].World();
    }
    
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get world context for timer"));
        return;
    }
    
    // Step 2: Create timer delegate for callback
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([]()
    {
        UE_LOG(LogTemp, Log, TEXT("Timer callback executed"));
        // Add timer callback logic here
    });
    
    // Step 3: Start timer with world context
    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(
        TimerHandle,
        TimerDelegate,
        1.0f,    // Interval in seconds
        true     // Loop timer
    );
    
    UE_LOG(LogTemp, Log, TEXT("Timer started successfully"));
}
```

#### `StopTimer_UTIL(FTimerHandle& RuntimeHandle, UWorld* WorldContextObject)`
**Purpose**: Stops a running timer using provided handle and world context.

```cpp
void StopTimer_UTIL(FTimerHandle& RuntimeHandle, UWorld* WorldContextObject)
{
    // Step 1: Validate world context
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world context for timer stop"));
        return;
    }
    
    // Step 2: Validate timer handle
    if (!RuntimeHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid timer handle for stop operation"));
        return;
    }
    
    // Step 3: Stop the timer using timer manager
    WorldContextObject->GetTimerManager().ClearTimer(RuntimeHandle);
    
    // Step 4: Invalidate handle after stopping
    RuntimeHandle.Invalidate();
    
    UE_LOG(LogTemp, Log, TEXT("Timer stopped successfully"));
}
```

### ZIP Integration Function

#### `UnZipInDir()`
**Purpose**: Placeholder function for ZIP extraction operations.
**Note**: Integrates with external ZIP handler functions.

```cpp
void UnZipInDir()
{
    // Step 1: Define default paths for ZIP extraction
    FString ZipPath = FPaths::ProjectDir() + TEXT("RSS/ZIP.zip");
    FString ExtractPath = FPaths::ProjectDir() + TEXT("Temp/Extracted/");
    
    // Step 2: Validate ZIP file existence
    if (!FPaths::FileExists(ZipPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("ZIP file not found: %s"), *ZipPath);
        return;
    }
    
    // Step 3: Ensure extraction directory exists
    if (!IFileManager::Get().DirectoryExists(*ExtractPath))
    {
        IFileManager::Get().MakeDirectory(*ExtractPath, true);
    }
    
    // Step 4: Call external ZIP handler for extraction
    UnpackZip(ZipPath, ExtractPath);
    
    UE_LOG(LogTemp, Log, TEXT("ZIP extraction completed from %s to %s"), *ZipPath, *ExtractPath);
}
```

## 3. Current Implementation Features

### JSON and RSS Support
- **RSS Manifest Generation**: Automated RSS feed structure creation
- **JSON Array Handling**: Flexible JSON array parsing with multiple field support
- **File I/O Integration**: Seamless file read/write operations with JSON

### Material System Integration
- **Dynamic Material Creation**: Runtime material instance generation
- **Parameter Management**: Scalar parameter configuration for material states
- **Asset Path Management**: Soft object path references for material loading

### File Operations
- **Hash Calculation**: MD5 hashing for files and directories
- **Directory Operations**: Recursive directory handling and validation
- **Cross-Platform Dialogs**: Native file/folder selection dialogs

### Timer Management
- **World Context Aware**: Proper integration with Unreal's world system
- **Delegate Support**: Lambda and function binding for timer callbacks
- **Resource Management**: Proper timer cleanup and handle management

### ZIP Functionality Integration
- **External Handler Integration**: Seamless integration with ZIP handler module
- **Compression Support**: Full ZIP creation and extraction capabilities
- **Directory Structure Preservation**: Maintains folder hierarchy in ZIP operations

## 4. Potential Improvements

### JSON System Enhancements
**Current**: Basic JSON read/write with hardcoded structures
**Improvement**: Template-based JSON serialization
```cpp
template<typename T>
class FJsonUtility
{
public:
    static bool SerializeObject(const T& Object, FString& OutJsonString);
    static bool DeserializeObject(const FString& JsonString, T& OutObject);
    static bool WriteObjectToFile(const T& Object, const FString& FilePath);
    static bool ReadObjectFromFile(T& OutObject, const FString& FilePath);
};
```

### Material Management System
**Current**: Hardcoded material path and parameter
**Improvement**: Configurable material system
```cpp
struct FMaterialConfig
{
    FSoftObjectPath MaterialPath;
    TMap<FName, float> ScalarParameters;
    TMap<FName, FLinearColor> VectorParameters;
    TMap<FName, FSoftObjectPath> TextureParameters;
};

class FDynamicMaterialManager
{
public:
    UMaterialInstanceDynamic* CreateDynamicInstance(const FMaterialConfig& Config);
    void UpdateMaterialParameters(UMaterialInstanceDynamic* Material, const FMaterialConfig& Config);
    void RegisterMaterialConfig(const FString& ConfigName, const FMaterialConfig& Config);
};
```

### File Hash System Enhancement
**Current**: MD5 only with basic directory hashing
**Improvement**: Multiple hash algorithms with advanced directory analysis
```cpp
enum class EHashAlgorithm
{
    MD5,
    SHA1,
    SHA256,
    CRC32
};

struct FFileHashResult
{
    FString FilePath;
    TMap<EHashAlgorithm, FString> Hashes;
    int64 FileSize;
    FDateTime ModificationTime;
    bool bIsValid;
};

class FAdvancedFileHasher
{
public:
    static FFileHashResult CalculateFileHashes(const FString& FilePath, TArray<EHashAlgorithm> Algorithms);
    static FString CalculateDirectoryTreeHash(const FString& DirectoryPath, EHashAlgorithm Algorithm);
    static TArray<FFileHashResult> CompareDirectories(const FString& Dir1, const FString& Dir2);
};
```

### Timer System Enhancement
**Current**: Basic timer with lambda callbacks
**Improvement**: Advanced timer management system
```cpp
DECLARE_DELEGATE_OneParam(FTimerProgressDelegate, float /*Progress*/);
DECLARE_DELEGATE(FTimerCompletedDelegate);

struct FAdvancedTimerConfig
{
    float Duration;
    float Interval;
    bool bLoop;
    bool bUseGameTime;
    FTimerProgressDelegate ProgressDelegate;
    FTimerCompletedDelegate CompletedDelegate;
};

class FAdvancedTimerManager
{
public:
    FTimerHandle StartAdvancedTimer(const FAdvancedTimerConfig& Config, UWorld* World);
    void PauseTimer(FTimerHandle Handle, UWorld* World);
    void ResumeTimer(FTimerHandle Handle, UWorld* World);
    float GetTimerProgress(FTimerHandle Handle, UWorld* World);
    void StopAllTimers(UWorld* World);
};
```

### Error Handling and Logging
**Current**: Basic UE_LOG statements
**Improvement**: Structured error handling system
```cpp
enum class EUtilityOperationResult
{
    Success,
    FileNotFound,
    ParseError,
    InvalidWorldContext,
    MaterialLoadFailed,
    HashCalculationFailed
};

struct FUtilityOperationResult
{
    EUtilityOperationResult Result;
    FString ErrorMessage;
    FString Context;
    float ProcessingTime;
    
    bool IsSuccess() const { return Result == EUtilityOperationResult::Success; }
    void LogResult() const;
};
```

## 5. Usage Examples

### RSS Manifest Creation
```cpp
// Create RSS initialization JSON
auto RSSJson = ThrowRSSInitModule_RWUtil(TEXT("RSS/RSSInit.json"), 1);
if (RSSJson.IsValid())
{
    UE_LOG(LogTemp, Log, TEXT("RSS manifest created successfully"));
}
```

### Dynamic Material Usage
```cpp
// Create material with sync state
UMaterialInstanceDynamic* SyncMaterial = ThrowDynamicInstance(1.0f); // Active state
if (SyncMaterial)
{
    // Use material in UI or 3D object
    UE_LOG(LogTemp, Log, TEXT("Dynamic material created"));
}
```

### File Hashing and Validation
```cpp
// Calculate hash for integrity check
FString FileHash = CalculateFileHash_UTIL(TEXT("C:/Data/ImportantFile.dat"));
if (!FileHash.IsEmpty())
{
    // Store hash for later validation
    UE_LOG(LogTemp, Log, TEXT("File hash: %s"), *FileHash);
}
```

### Timer Operations
```cpp
// Start utility timer
ThrowTimer_UTIL();

// Later, stop timer with handle
FTimerHandle MyHandle; // Assume this was stored from timer creation
UWorld* World = GetWorld(); // Get current world context
StopTimer_UTIL(MyHandle, World);
```

### ZIP Operations Integration
```cpp
// Extract ZIP using integrated handler
UnZipInDir(); // Uses default paths

// Or use external functions directly
TArray<FString> FilesToZip = { TEXT("File1.txt"), TEXT("File2.txt") };
CreateZip(FilesToZip, TEXT("Output.zip"));
```

## 6. Integration with HTTPManager

The CustomUtilities module serves as a foundation for the HTTPManager project by providing:

- **RSS Feed Management**: JSON-based RSS feed configuration and manifest handling
- **Content Validation**: File hashing for downloaded content integrity verification
- **UI Material System**: Dynamic materials for HTTP operation status indicators
- **Timer Coordination**: Timing utilities for HTTP request intervals and timeouts
- **File Operations**: Directory management and dialog systems for user interaction
- **ZIP Integration**: Seamless connection to compression functionality for content packaging

This comprehensive utility system enables robust HTTP content management with proper validation, user interface feedback, and file organization capabilities.
