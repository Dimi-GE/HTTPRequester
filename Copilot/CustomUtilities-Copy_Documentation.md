# CustomUtilities.cpp Documentation (Updated)

## Overview
CustomUtilities.cpp provides a comprehensive set of utility functions for JSON handling, file operations, material management, and system utilities within an Unreal Engine 5 project. This file serves as a central hub for common operations needed across the HTTPManager module.

## 1. Required Modules and External Dependencies

### Core Unreal Engine Modules
```cpp
// Materials and Rendering
#include "Materials/MaterialInstanceDynamic.h"

// Engine Core Systems
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"

// File and Platform Management
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"

// User Interface and Messaging
#include "Misc/MessageDialog.h"

// JSON Processing
#include "Json.h"
#include "JsonUtilities.h"
```

### External Function Dependencies
```cpp
// ZIP Handler External Functions (from ZipHandler.cpp)
extern void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath);
extern void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath);
extern void UnpackZip(const FString& PathToZip, const FString& TempDir);
extern TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder);
```

### Build.cs Dependencies
Ensure your module's Build.cs file includes:
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "UnrealEd", "InputCore",
    "Blutility", "UMG", "HTTP", "Json", "JsonUtilities",
    "Slate", "SlateCore", "DesktopPlatform", "zlib"
});
```

## 2. Function Documentation with Comments

### Material Management Functions

#### `ThrowDynamicInstance(float ScalarValue)`
**Purpose**: Creates a dynamic material instance from a hard-coded material path.
```cpp
UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue)
{
    // Step 1: Load base material from static path
    static FSoftObjectPath MatPath(TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify"));
    
    // Step 2: Attempt to load the material interface
    UMaterialInterface* BaseMat = Cast<UMaterialInterface>(MatPath.TryLoad());
    if (!BaseMat) return nullptr; // Early exit if material loading fails
    
    // Step 3: Create dynamic instance and set scalar parameter
    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, nullptr);
    if (DynMat)
    {
        // Step 4: Apply scalar value to "SyncState" parameter
        DynMat->SetScalarParameterValue(FName("SyncState"), ScalarValue);
    }
    
    return DynMat;
}
```

### User Interface Functions

#### `ThrowDialogMessage(FString Message)`
**Purpose**: Displays a warning dialog message to the user.
```cpp
void ThrowDialogMessage(FString Message)
{
    // Step 1: Prepare dialog title
    FText MsgTitle = FText::FromString(TEXT("Warning!"));
    
    // Step 2: Display modal dialog with OK button
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString((TEXT("%s"), *Message)), &MsgTitle);
}
```

### JSON Processing Functions

#### `ThrowRSSInitObject(FString RSSInitModule, FString RSSInitObject, int32 ReadWriteBinary)`
**Purpose**: Retrieves a specific JSON object from the RSSInit.json file structure.
```cpp
TSharedPtr<FJsonObject> ThrowRSSInitObject(FString RSSInitModule, FString RSSInitObject, int32 ReadWriteBinary)
{
    // Step 1: Define JSON file path (hardcoded to RSSInit.json)
    FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
    TSharedPtr<FJsonObject> JSONObject;
    
    // Step 2: Load the root module from JSON file
    TSharedPtr<FJsonObject> ModuleAsObject = ThrowRSSInitModule_RWUtil(RSSInitSubPath, ReadWriteBinary);
    
    // Step 3: Navigate JSON hierarchy to find requested module
    if (ModuleAsObject->HasField(RSSInitModule))
    {
        TSharedPtr<FJsonObject> LifecycleInit = ModuleAsObject->GetObjectField(RSSInitModule);
        
        // Step 4: Extract the specific object from the module
        if (LifecycleInit->HasField(RSSInitObject))
        {
            JSONObject = LifecycleInit->GetObjectField(RSSInitObject);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find JSONObject field."));
            return nullptr;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find %s (RSSInitModule) field."), *RSSInitModule);
        return nullptr; 
    }
    
    return JSONObject;
}
```

#### `ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWriteBinary)`
**Purpose**: Loads and parses JSON file, returning the module object.
```cpp
TSharedPtr<FJsonObject> ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWriteBinary)
{
    // Step 1: Construct full file path from project directory
    FString RSSInitPath = FPaths::ProjectDir() + JSONSubPath;
    FString JsonString;
    
    // Step 2: Load file content into string
    if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
        return nullptr;
    }
    
    // Step 3: Parse JSON based on read/write mode
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    JsonArray.Empty();
    TSharedPtr<FJsonObject> ModuleAsObject = nullptr;
    
    if (ReadWriteBinary == 0) // Read mode
    {
        // Step 4a: Deserialize JSON array from string
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        FJsonSerializer::Deserialize(Reader, JsonArray);
        ModuleAsObject = JsonArray[1]->AsObject(); // Access second element (hardcoded index)
    }
    else if (ReadWriteBinary == 1) // Write mode
    { 
        // Step 4b: Serialize JSON array to string (incomplete implementation)
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonArray, Writer);
        ModuleAsObject = JsonArray[1]->AsObject();
    }
    
    return ModuleAsObject;
}
```

### File System Utilities

#### `OpenFolderDialog_UTIL()`
**Purpose**: Opens a native folder selection dialog.
```cpp
FString OpenFolderDialog_UTIL()
{
    FString SelectedFolder;
    
    // Step 1: Get desktop platform interface
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        // Step 2: Get parent window handle for modal dialog
        void* ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
        
        // Step 3: Set default path to project directory
        FString DefaultPath = FPaths::ProjectDir();
        
        // Step 4: Open folder selection dialog
        bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
            ParentWindowHandle,
            TEXT("Select a folder"),
            DefaultPath,
            SelectedFolder
        );
    }
    
    return SelectedFolder;
}
```

### Cryptographic Utilities

#### `CalculateFileHash_UTIL(const FString& FilePath)`
**Purpose**: Generates MD5 hash for a file.
```cpp
FString CalculateFileHash_UTIL(const FString& FilePath)
{
    // Step 1: Read file into byte array
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load file: %s"), *FilePath);
        return TEXT("InvalidHash");
    }
    
    // Step 2: Initialize MD5 hash generator
    FMD5 Md5Gen;
    Md5Gen.Update(FileData.GetData(), FileData.Num());
    
    // Step 3: Finalize hash calculation
    uint8 Digest[16];
    Md5Gen.Final(Digest);
    
    // Step 4: Convert binary digest to hexadecimal string
    FString HashString;
    for (int32 i = 0; i < 16; i++)
    {
        HashString += FString::Printf(TEXT("%02x"), Digest[i]);
    }
    
    return HashString;
}
```

### Timer Management

#### `ThrowTimer_UTIL()`
**Purpose**: Creates and manages a countdown timer with world context handling.
```cpp
void ThrowTimer_UTIL()
{
    UWorld* WorldContext = nullptr;
    FTimerHandle RuntimeHandle;
    int32 Counter = 0;
    
    // Step 1: Get correct world context for timer operations
    if (GEngine)
    {
        // Step 1a: Prefer runtime world (Game or PIE)
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
            {
                WorldContext = Context.World();
            }
        }
    }
    
    // Step 1b: Fallback to editor world context
    #if WITH_EDITOR
        if (GEditor)
        {
            WorldContext = GEditor->GetEditorWorldContext().World();
        }
    #endif
    
    // Step 2: Set up repeating timer with lambda callback
    if (WorldContext)
    {
        WorldContext->GetTimerManager().SetTimer(RuntimeHandle, [WorldContext, &Counter, &RuntimeHandle]()
        {
            // Step 3: Timer callback logic with auto-stop
            if (Counter < 5)
            {
                Counter++;
                UE_LOG(LogTemp, Warning, TEXT("TimerSeconds: %d."), Counter);
            }
            else
            {
                // Step 4: Stop timer after 5 iterations
                StopTimer_UTIL(RuntimeHandle, WorldContext);
                UE_LOG(LogTemp, Warning, TEXT("TimerSeconds: %d."), Counter);
            }
        }, 1, true); // 1 second interval, repeating
    }
    else
    {
        bool bIsGameWorld = WorldContext->IsGameWorld();
        UE_LOG(LogTemp, Error, TEXT("Failed to run timer - context %s."), bIsGameWorld ? TEXT("true") : TEXT("false"));
    }
}
```

## 3. Potential Improvements

### Function-Level Improvements

#### `ThrowDynamicInstance`
- **Issue**: Hardcoded material path limits reusability
- **Improvement**: Accept material path as parameter or use asset registry
- **Code Enhancement**:
```cpp
UMaterialInstanceDynamic* CreateDynamicMaterialInstance(const FString& MaterialPath, const FName& ParameterName, float ScalarValue)
{
    FSoftObjectPath MatPath(MaterialPath);
    // ... rest of implementation
}
```

#### `ThrowRSSInitModule_RWUtil`
- **Issue**: Hardcoded array index [1] can cause crashes
- **Improvement**: Add bounds checking and make index configurable
- **Code Enhancement**:
```cpp
// Add validation before array access
if (JsonArray.Num() > 1)
{
    ModuleAsObject = JsonArray[1]->AsObject();
}
else
{
    UE_LOG(LogTemp, Error, TEXT("JSON array does not contain expected module at index 1"));
    return nullptr;
}
```

#### `CalculateFileHash_UTIL`
- **Issue**: Only supports MD5, which is cryptographically weak
- **Improvement**: Support multiple hash algorithms (SHA-256, etc.)
- **Code Enhancement**:
```cpp
enum class EHashAlgorithm { MD5, SHA256 };
FString CalculateFileHash_UTIL(const FString& FilePath, EHashAlgorithm Algorithm = EHashAlgorithm::SHA256)
```

#### `ThrowTimer_UTIL`
- **Issue**: Uses stack variables that may go out of scope
- **Improvement**: Use member variables or shared pointers
- **Code Enhancement**:
```cpp
class FTimerManager_UTIL
{
    TSharedPtr<int32> Counter;
    FTimerHandle RuntimeHandle;
    // ... implementation
};
```

### Overall Flow Improvements

#### Error Handling Standardization
- **Current**: Inconsistent error handling patterns
- **Improvement**: Implement standardized error handling with custom error codes
```cpp
enum class ECustomUtilsError
{
    Success,
    FileNotFound,
    JSONParseError,
    InvalidParameter
};

struct FCustomUtilsResult
{
    ECustomUtilsError ErrorCode;
    FString ErrorMessage;
    bool IsSuccess() const { return ErrorCode == ECustomUtilsError::Success; }
};
```

#### Resource Management
- **Current**: Manual resource cleanup
- **Improvement**: RAII patterns and smart pointers throughout
```cpp
// Use TUniquePtr for automatic cleanup
TUniquePtr<FJsonObject> LoadJSONFromFile(const FString& FilePath);
```

#### Configuration Management
- **Current**: Hardcoded paths and magic numbers
- **Improvement**: Configuration file or settings object
```cpp
struct FCustomUtilsConfig
{
    FString RSSInitPath = TEXT("\\RSS\\RSSInit.json");
    FString DefaultMaterialPath = TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify");
    int32 TimerDuration = 5;
    // ... other configurable values
};
```

#### Async Operations
- **Current**: Synchronous file operations can block main thread
- **Improvement**: Implement async variants using Unreal's async task system
```cpp
void LoadJSONFileAsync(const FString& FilePath, TFunction<void(TSharedPtr<FJsonObject>)> Callback);
```

#### Module Organization
- **Current**: Single large file with mixed responsibilities
- **Improvement**: Split into specialized classes
```cpp
// Separate classes for different responsibilities
class FJSONUtilities_UTIL;
class FFileSystemUtilities_UTIL;
class FMaterialUtilities_UTIL;
class FTimerUtilities_UTIL;
```

## 4. Usage Examples

### Basic JSON Operations
```cpp
// Load specific JSON object
auto JSONObject = ThrowRSSInitObject(TEXT("LifecycleInit"), TEXT("MacrosManager"), 0);
if (JSONObject.IsValid())
{
    bool bIsInitialized = JSONObject->GetBoolField(TEXT("bIsInitialized"));
}
```

### File Hash Verification
```cpp
// Calculate and compare file hashes
FString FileHash = CalculateFileHash_UTIL(TEXT("C:/MyFile.txt"));
if (FileHash != TEXT("InvalidHash"))
{
    // Store or compare hash
}
```

### Material Instance Creation
```cpp
// Create dynamic material with sync state
UMaterialInstanceDynamic* DynMat = ThrowDynamicInstance(1.0f);
if (DynMat)
{
    // Apply to UI component
    MyImageWidget->SetBrushFromMaterial(DynMat);
}
```

This documentation provides a comprehensive overview of the CustomUtilities-Copy.cpp functionality, including proper integration guidelines, detailed function explanations, and actionable improvement suggestions for production use.
