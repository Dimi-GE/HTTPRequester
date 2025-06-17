# Plugin Separation Guide: GitHub Sync + ZIP Utilities

## Overview

This document provides a comprehensive guide for separating the current HTTPRequester project into two independent plugins:

1. **GitHubSync Plugin** - GitHub repository integration for Unreal Engine
2. **ZipUtilities Plugin** - Advanced ZIP file management for Unreal Engine

This separation is **mandatory** for publishing both plugins independently on GitHub and Fab marketplaces.

---

## 🎯 Separation Strategy

### **Why Separate?**
- **Independent functionality**: ZIP operations are useful beyond GitHub sync
- **Marketplace categories**: Different target audiences and use cases
- **Licensing flexibility**: Different licensing models for each
- **Maintenance**: Easier to update and maintain separately
- **Distribution**: Separate download and installation

### **Dependency Relationship**
- **GitHubSync Plugin** → **depends on** → **ZipUtilities Plugin**
- **ZipUtilities Plugin** → **standalone** (no external dependencies)

---

## 📁 Current File Analysis

### **Files for ZipUtilities Plugin:**

#### **Core ZIP Functionality:**
```
Source/HTTPManager/Private/ZipHandler.cpp
Source/HTTPManager/Public/ZipHandler.h (if exists)
```

#### **Related Utility Functions:**
```
// From CustomUtilities.cpp - ZIP-related functions:
- CollectFilesForZip_UTIL()
- Any other ZIP manipulation utilities
```

#### **Dependencies to Include:**
```
// Third-party ZIP library files (minizip-ng):
- All minizip-ng source files
- Headers and implementation
- Platform-specific configurations
```

### **Files for GitHubSync Plugin:**

#### **GitHub Integration:**
```
Source/HTTPManager/Private/RSSSyncGuidance.cpp
Source/HTTPManager/Private/MacrosManager.cpp (GitHub-related functions)
Source/HTTPManager/Public/MacrosManager.h (GitHub-related declarations)
```

#### **HTTP and Networking:**
```
// HTTP request handling for GitHub API
// JSON parsing for GitHub responses
// Authentication and token management
```

#### **File System Operations:**
```
// From CustomUtilities.cpp - Non-ZIP functions:
- CalculateFileHash_UTIL()
- CalculateDirectoryHash_UTIL()
- File comparison utilities
- Manifest creation functions
```

---

## 🔧 Plugin 1: ZipUtilities Plugin

### **Plugin Structure:**
```
ZipUtilities/
├── ZipUtilities.uplugin
├── Source/
│   ├── ZipUtilities/
│   │   ├── ZipUtilities.Build.cs
│   │   ├── Private/
│   │   │   ├── ZipUtilitiesModule.cpp
│   │   │   ├── ZipHandler.cpp
│   │   │   ├── ZipFileOperations.cpp
│   │   │   └── ZipValidation.cpp
│   │   └── Public/
│   │       ├── ZipUtilitiesModule.h
│   │       ├── ZipHandler.h
│   │       ├── ZipTypes.h
│   │       └── IZipUtilities.h
│   └── ThirdParty/
│       └── minizip-ng/
│           ├── Include/
│           └── Lib/
├── Config/
│   └── FilterPlugin.ini
└── Resources/
    └── Icon128.png
```

### **ZipUtilities.uplugin:**
```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "ZIP Utilities",
    "Description": "Advanced ZIP file creation, extraction, and management for Unreal Engine projects. Supports structured ZIP creation, password protection, and large file handling.",
    "Category": "File Management",
    "CreatedBy": "YourStudio",
    "CreatedByURL": "https://github.com/yourusername/ZipUtilities",
    "DocsURL": "https://github.com/yourusername/ZipUtilities/wiki",
    "MarketplaceURL": "",
    "SupportURL": "https://github.com/yourusername/ZipUtilities/issues",
    "CanContainContent": false,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "Modules": [
        {
            "Name": "ZipUtilities",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "PlatformAllowList": ["Win64", "Mac", "Linux"]
        }
    ],
    "Plugins": []
}
```

### **Core Functions to Include:**

#### **Basic ZIP Operations:**
```cpp
// Create ZIP from file list
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities")
bool CreateZipFromFiles(
    const TArray<FString>& FilePaths,
    const FString& ZipPath,
    FString& ErrorMessage
);

// Create structured ZIP (with relative paths)
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities")
bool CreateStructuredZip(
    const TArray<FZipFileEntry>& FileEntries,
    const FString& ZipPath,
    FString& ErrorMessage
);

// Extract ZIP to folder
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities")
bool ExtractZipToFolder(
    const FString& ZipPath,
    const FString& DestinationPath,
    FString& ErrorMessage
);

// List ZIP contents
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities")
bool ListZipContents(
    const FString& ZipPath,
    TArray<FZipFileInfo>& FileList,
    FString& ErrorMessage
);
```

#### **Advanced Features:**
```cpp
// Extract specific files
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities|Advanced")
bool ExtractSpecificFiles(
    const FString& ZipPath,
    const TArray<FString>& FilesToExtract,
    const FString& DestinationPath,
    FString& ErrorMessage
);

// Validate ZIP integrity
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities|Advanced")
bool ValidateZipIntegrity(
    const FString& ZipPath,
    FString& ValidationResult
);

// Get ZIP information
UFUNCTION(BlueprintCallable, Category = "ZIP Utilities|Advanced")
bool GetZipInformation(
    const FString& ZipPath,
    FZipInformation& ZipInfo,
    FString& ErrorMessage
);
```

### **Data Structures:**
```cpp
USTRUCT(BlueprintType)
struct FZipFileEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString SourceFilePath;     // Full path to source file

    UPROPERTY(BlueprintReadWrite)
    FString ZipEntryPath;       // Path inside ZIP file

    UPROPERTY(BlueprintReadWrite)
    bool bCompress = true;      // Whether to compress this file
};

USTRUCT(BlueprintType)
struct FZipFileInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString FileName;

    UPROPERTY(BlueprintReadOnly)
    int64 UncompressedSize;

    UPROPERTY(BlueprintReadOnly)
    int64 CompressedSize;

    UPROPERTY(BlueprintReadOnly)
    FDateTime ModificationTime;

    UPROPERTY(BlueprintReadOnly)
    bool bIsDirectory;
};
```

---

## 🌐 Plugin 2: GitHubSync Plugin

### **Plugin Structure:**
```
GitHubSync/
├── GitHubSync.uplugin
├── Source/
│   ├── GitHubSync/
│   │   ├── GitHubSync.Build.cs
│   │   ├── Private/
│   │   │   ├── GitHubSyncModule.cpp
│   │   │   ├── GitHubAPI.cpp
│   │   │   ├── GitHubRepository.cpp
│   │   │   ├── GitHubAuth.cpp
│   │   │   ├── FileManifest.cpp
│   │   │   ├── SyncOperations.cpp
│   │   │   └── GitHubSyncSettings.cpp
│   │   └── Public/
│   │       ├── GitHubSyncModule.h
│   │       ├── IGitHubSync.h
│   │       ├── GitHubTypes.h
│   │       ├── GitHubRepository.h
│   │       └── GitHubSyncSettings.h
│   └── Editor/
│       ├── Private/
│       │   ├── GitHubSyncEditor.cpp
│       │   ├── GitHubSyncToolWidget.cpp
│       │   └── GitHubSyncSettingsCustomization.cpp
│       └── Public/
│           ├── GitHubSyncEditor.h
│           └── GitHubSyncToolWidget.h
├── Config/
│   ├── FilterPlugin.ini
│   └── DefaultGitHubSync.ini
├── Content/
│   └── EditorUtilityWidgets/
│       └── GitHubSyncTool.uasset
└── Resources/
    └── Icon128.png
```

### **GitHubSync.uplugin:**
```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "GitHub Sync",
    "Description": "Easy GitHub repository synchronization for Unreal Engine. Download, upload, and sync projects with GitHub repositories without Git knowledge required.",
    "Category": "Version Control",
    "CreatedBy": "YourStudio",
    "CreatedByURL": "https://github.com/yourusername/GitHubSync",
    "DocsURL": "https://github.com/yourusername/GitHubSync/wiki",
    "MarketplaceURL": "",
    "SupportURL": "https://github.com/yourusername/GitHubSync/issues",
    "CanContainContent": true,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "Modules": [
        {
            "Name": "GitHubSync",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "PlatformAllowList": ["Win64", "Mac", "Linux"]
        },
        {
            "Name": "GitHubSyncEditor",
            "Type": "Editor",
            "LoadingPhase": "Default",
            "PlatformAllowList": ["Win64", "Mac", "Linux"]
        }
    ],
    "Plugins": [
        {
            "Name": "ZipUtilities",
            "Enabled": true,
            "MarketplaceURL": "com.epicgames.launcher://ue/marketplace/product/[ZIP_UTILITIES_ID]"
        }
    ]
}
```

---

## 🔄 Separation Process

### **Phase 1: Prepare ZipUtilities Plugin**

#### **Step 1: Create Plugin Structure**
1. Create new plugin folder: `ZipUtilities/`
2. Set up basic plugin files (`uplugin`, `Build.cs`, module files)
3. Copy ZIP-related source files from current project

#### **Step 2: Extract ZIP Functionality**
1. Copy `ZipHandler.cpp` to `ZipUtilities/Private/`
2. Extract ZIP-related functions from `CustomUtilities.cpp`
3. Create clean public interface in `ZipUtilities/Public/`
4. Remove any GitHub-specific dependencies

#### **Step 3: Setup Third-Party Dependencies**
1. Copy minizip-ng source to `ThirdParty/minizip-ng/`
2. Update `Build.cs` to include third-party library
3. Configure platform-specific builds
4. Test compilation on target platforms

#### **Step 4: Create Public API**
```cpp
// Clean, simple API for other plugins to use
class ZIPUTILITIES_API IZipUtilities
{
public:
    virtual bool CreateZip(const TArray<FString>& Files, const FString& ZipPath) = 0;
    virtual bool ExtractZip(const FString& ZipPath, const FString& Destination) = 0;
    virtual bool ValidateZip(const FString& ZipPath) = 0;
};
```

### **Phase 2: Prepare GitHubSync Plugin**

#### **Step 1: Create Plugin Structure**
1. Create new plugin folder: `GitHubSync/`
2. Set up plugin files with ZipUtilities dependency
3. Copy GitHub-related source files

#### **Step 2: Extract GitHub Functionality**
1. Copy `RSSSyncGuidance.cpp` to `GitHubSync/Private/`
2. Extract GitHub-related functions from `MacrosManager.cpp`
3. Remove ZIP creation code (use ZipUtilities plugin instead)
4. Update includes and dependencies

#### **Step 3: Update to Use ZipUtilities Plugin**
```cpp
// Instead of direct ZIP operations:
CreateZip_Structured(FilesToZip, ZipPath);

// Use ZipUtilities plugin interface:
IZipUtilities& ZipUtilities = IZipUtilities::Get();
ZipUtilities.CreateZip(FilesToZip, ZipPath);
```

#### **Step 4: Create Editor Integration**
1. Create Editor Utility Widget for GitHub operations
2. Add plugin settings panel
3. Create toolbar buttons/menu items
4. Setup user-friendly interface

### **Phase 3: Testing and Validation**

#### **Step 1: Test ZipUtilities Plugin Independently**
```cpp
// Test basic operations
bool TestBasicZipOperations();
bool TestStructuredZip();
bool TestExtractionOperations();
bool TestErrorHandling();
```

#### **Step 2: Test GitHubSync Plugin with ZipUtilities**
```cpp
// Test GitHub operations using ZipUtilities
bool TestDownloadWorkflow();
bool TestUploadWorkflow();
bool TestSyncOperations();
bool TestPluginDependency();
```

#### **Step 3: Cross-Platform Testing**
- Test on Windows (primary)
- Test on Mac (secondary)
- Test on Linux (if applicable)
- Verify third-party library compilation

---

## 📝 Build System Updates

### **ZipUtilities.Build.cs:**
```csharp
using UnrealBuildTool;
using System.IO;

public class ZipUtilities : ModuleRules
{
    public ZipUtilities(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "ToolMenus"
        });

        // Add third-party library
        AddThirdPartyDependencies(Target);
    }

    private void AddThirdPartyDependencies(ReadOnlyTargetRules Target)
    {
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
        string MinizipPath = Path.Combine(ThirdPartyPath, "minizip-ng");

        PublicIncludePaths.Add(Path.Combine(MinizipPath, "Include"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(MinizipPath, "Lib/Win64/minizip.lib"));
        }
        // Add other platforms as needed
    }
}
```

### **GitHubSync.Build.cs:**
```csharp
using UnrealBuildTool;

public class GitHubSync : ModuleRules
{
    public GitHubSync(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "HTTP",
            "Json",
            "ZipUtilities"  // Dependency on ZipUtilities plugin
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "ToolMenus",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "EditorWidgets",
            "ToolWidgets"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "EditorSubsystem"
            });
        }
    }
}
```

---

## 🚀 Migration Strategy

### **For Existing Users:**

#### **Migration Package**
Create a migration guide and helper tools:
1. **Detection script** - Finds current HTTPRequester installation
2. **Backup creation** - Saves current settings and configurations
3. **Plugin installation** - Installs both new plugins
4. **Settings migration** - Transfers existing settings
5. **Validation** - Tests that everything works

#### **Migration Steps:**
1. **Backup current project** and settings
2. **Disable HTTPRequester** plugin
3. **Install ZipUtilities** plugin first
4. **Install GitHubSync** plugin second  
5. **Run migration tool** to transfer settings
6. **Test basic operations** to ensure functionality
7. **Remove old HTTPRequester** plugin when confirmed working

### **Backward Compatibility:**

#### **API Compatibility Layer**
```cpp
// In GitHubSync plugin, provide compatibility functions
namespace HTTPRequesterCompatibility
{
    // Wrapper functions that call new plugin APIs
    DEPRECATED("Use GitHubSync plugin functions instead")
    bool OldFunctionName(/* old parameters */)
    {
        // Call new function with parameter mapping
        return NewPluginFunction(/* mapped parameters */);
    }
}
```

---

## 📋 Validation Checklist

### **ZipUtilities Plugin Validation:**
- [ ] Compiles independently without errors
- [ ] All ZIP operations work correctly
- [ ] Third-party library integrates properly
- [ ] No dependencies on GitHub functionality
- [ ] API is clean and well-documented
- [ ] Cross-platform compatibility tested
- [ ] Performance meets requirements

### **GitHubSync Plugin Validation:**
- [ ] Successfully depends on ZipUtilities plugin
- [ ] All GitHub operations work correctly
- [ ] No direct ZIP library dependencies
- [ ] Editor integration works properly
- [ ] Settings system functions correctly
- [ ] Error handling is comprehensive
- [ ] User interface is intuitive

### **Integration Testing:**
- [ ] Both plugins install correctly together
- [ ] ZipUtilities functions are accessible from GitHubSync
- [ ] No conflicts between plugins
- [ ] Performance is acceptable
- [ ] Memory usage is reasonable
- [ ] All workflows work end-to-end

---

## 🎯 Success Criteria

### **Technical Success:**
- Both plugins compile and run independently
- GitHubSync successfully uses ZipUtilities services
- All original functionality is preserved
- Performance is maintained or improved
- Cross-platform compatibility is maintained

### **User Experience Success:**
- Installation process is straightforward
- Migration from old system is smooth
- Documentation is clear and complete
- Learning curve is minimal
- Support burden is manageable

### **Business Success:**
- Plugins can be published separately
- Each plugin targets its appropriate audience
- Licensing can be managed independently
- Updates can be released independently
- Market positioning is improved

---

This separation strategy ensures both plugins maintain their functionality while becoming independent, marketable products that serve different but complementary needs in the Unreal Engine ecosystem.
