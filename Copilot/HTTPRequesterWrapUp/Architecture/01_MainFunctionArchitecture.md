# GitHub Sync Plugin - Main Function Architecture

## Overview

This document outlines the main function architecture for making the GitHub synchronization functionality maximally user-friendly for beginners. The goal is to create simple, intuitive functions that hide the complexity while providing sufficient feedback for users to understand what's happening.

## Core Design Principles

### 1. **Simplicity First**
- One function call for common operations
- Clear, descriptive function names
- Minimal required parameters with smart defaults

### 2. **Beginner-Friendly**
- No need to understand Git/GitHub API internals
- Clear progress feedback
- Detailed error messages with solutions

### 3. **Safety**
- Always validate before making changes
- Clear separation between "preview" and "execute" modes
- Automatic backups when possible

---

## Main Function Architecture

### Primary User Functions

#### 1. `EasyDownloadFromGitHub()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync")
bool EasyDownloadFromGitHub(
    const FString& GitHubURL,           // "https://github.com/owner/repo"
    const FString& AccessToken = "",    // Empty for public repos
    const FString& BranchName = "main", // Default to main branch
    FString& StatusMessage              // User-friendly status/error message
);
```

**What it does**: Downloads and extracts a GitHub repository in one simple call
**User experience**: "Download this GitHub repository to my project"

#### 2. `EasyUploadToGitHub()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync")
bool EasyUploadToGitHub(
    const FString& GitHubURL,           // Target repository
    const FString& AccessToken,        // Required for uploads
    const FString& FolderPath,         // What to upload
    const FString& CommitMessage,      // Description of changes
    const FString& BranchName = "main", // Target branch
    FString& StatusMessage              // User-friendly feedback
);
```

**What it does**: Uploads a local folder to GitHub repository in one call
**User experience**: "Upload this folder to my GitHub repository"

#### 3. `EasySyncWithGitHub()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync")
bool EasySyncWithGitHub(
    const FString& GitHubURL,
    const FString& AccessToken,
    ESyncDirection Direction,           // Download, Upload, or TwoWay
    bool bPreviewOnly = true,           // Safe default - preview first
    FString& StatusMessage
);
```

**What it does**: Intelligent two-way synchronization between local and remote
**User experience**: "Keep my project and GitHub repository in sync"

#### 4. `PreviewGitHubChanges()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync")
bool PreviewGitHubChanges(
    const FString& GitHubURL,
    const FString& AccessToken,
    TArray<FChangeInfo>& Changes,       // What would change
    FString& StatusMessage
);
```

**What it does**: Shows what would change without making any modifications
**User experience**: "Show me what would change if I sync"

---

## Supporting Enums and Structures

### `ESyncDirection`
```cpp
UENUM(BlueprintType)
enum class ESyncDirection : uint8
{
    DownloadOnly    UMETA(DisplayName = "Download from GitHub"),
    UploadOnly      UMETA(DisplayName = "Upload to GitHub"),  
    TwoWaySync      UMETA(DisplayName = "Bidirectional Sync")
};
```

### `FChangeInfo`
```cpp
USTRUCT(BlueprintType)
struct FChangeInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString FilePath;

    UPROPERTY(BlueprintReadOnly)
    FString Action;             // "Add", "Update", "Remove"

    UPROPERTY(BlueprintReadOnly)
    FString Reason;             // "File is newer", "File missing locally", etc.

    UPROPERTY(BlueprintReadOnly)
    FString Direction;          // "Local to GitHub", "GitHub to Local"
};
```

---

## Advanced Functions for Power Users

#### 1. `SetupGitHubRepository()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync|Setup")
bool SetupGitHubRepository(
    const FString& GitHubURL,
    const FString& AccessToken,
    const FString& LocalProjectPath,
    FString& StatusMessage
);
```

**What it does**: One-time setup to connect a project with a GitHub repository

#### 2. `ValidateGitHubConnection()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync|Setup")
bool ValidateGitHubConnection(
    const FString& GitHubURL,
    const FString& AccessToken,
    FString& ValidationResult
);
```

**What it does**: Tests connection and permissions without making changes

#### 3. `CreateProjectBackup()`
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync|Safety")
bool CreateProjectBackup(
    const FString& BackupLocation,
    FString& BackupPath,
    FString& StatusMessage
);
```

**What it does**: Creates a backup before sync operations

---

## Internal Implementation Flow

### `EasyDownloadFromGitHub()` Implementation
```cpp
bool UGitHubSyncManager::EasyDownloadFromGitHub(...)
{
    // 1. Parse GitHub URL to extract owner/repo
    FString RepoOwner, RepoName;
    if (!ParseGitHubURL(GitHubURL, RepoOwner, RepoName))
    {
        StatusMessage = "Invalid GitHub URL format. Expected: https://github.com/owner/repo";
        return false;
    }

    // 2. Create temporary folder
    FString TempFolder = CreateRSSSyncTempFolder();
    if (TempFolder.IsEmpty())
    {
        StatusMessage = "Failed to create temporary folder for download";
        return false;
    }

    // 3. Download with validation
    bool bSuccess = false;
    DownloadBranchWithValidation(RepoOwner, RepoName, BranchName, AccessToken, TempFolder,
        [&bSuccess, &StatusMessage](bool bDownloadSuccess, FString ErrorMessage)
        {
            bSuccess = bDownloadSuccess;
            if (!bSuccess)
            {
                StatusMessage = FString::Printf(TEXT("Download failed: %s"), *ErrorMessage);
            }
            else
            {
                StatusMessage = "Download completed successfully";
            }
        });

    // 4. Wait for async completion (with timeout)
    return WaitForAsyncOperation(bSuccess, 60.0f);
}
```

### `EasyUploadToGitHub()` Implementation
```cpp
bool UGitHubSyncManager::EasyUploadToGitHub(...)
{
    // 1. Validate inputs
    if (AccessToken.IsEmpty())
    {
        StatusMessage = "Access token is required for uploads. Get one from GitHub Settings > Developer settings > Personal access tokens";
        return false;
    }

    if (!IFileManager::Get().DirectoryExists(*FolderPath))
    {
        StatusMessage = FString::Printf(TEXT("Folder not found: %s"), *FolderPath);
        return false;
    }

    // 2. Parse GitHub URL
    FString RepoOwner, RepoName;
    if (!ParseGitHubURL(GitHubURL, RepoOwner, RepoName))
    {
        StatusMessage = "Invalid GitHub URL format";
        return false;
    }

    // 3. Upload with validation
    bool bSuccess = false;
    UploadUpdatedBranchWithValidation(RepoOwner, RepoName, BranchName, AccessToken, 
        FolderPath, CommitMessage,
        [&bSuccess, &StatusMessage](bool bUploadSuccess, FString ErrorMessage)
        {
            bSuccess = bUploadSuccess;
            StatusMessage = bUploadSuccess ? 
                "Upload completed successfully" : 
                FString::Printf(TEXT("Upload failed: %s"), *ErrorMessage);
        });

    return WaitForAsyncOperation(bSuccess, 120.0f);
}
```

---

## User Interface Integration

### Blueprint-Friendly Design
All main functions are marked with `UFUNCTION(BlueprintCallable)` for easy integration in:
- Editor Utility Widgets
- Blueprint graphs
- Custom tool panels

### Widget Layout Suggestion
```
GitHub Repository Sync Tool
┌─────────────────────────────────────────┐
│ Repository URL: [text field]           │
│ Access Token:   [password field]       │
│ Branch:         [dropdown: main]       │
│                                         │
│ [Download from GitHub]  [Preview Changes] │
│ [Upload to GitHub]      [Two-Way Sync]   │
│                                         │
│ Status: [status text area]              │
│ Progress: [progress bar]                │
└─────────────────────────────────────────┘
```

---

## Error Handling and User Guidance

### Common Error Scenarios with Solutions

#### 1. **Invalid Access Token**
```
Error: "Authentication failed - invalid access token"
Solution: "Go to GitHub Settings > Developer settings > Personal access tokens to create a new token"
Help Link: "https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token"
```

#### 2. **Insufficient Permissions**
```
Error: "Token lacks write permissions for this repository"
Solution: "Your token needs 'repo' scope for private repositories or 'public_repo' scope for public repositories"
Action: "Generate a new token with the correct permissions"
```

#### 3. **Repository Not Found**
```
Error: "Repository not found or access denied"
Solution: "Check that the repository URL is correct and you have access to it"
Suggestions: 
- "Verify the repository is public or you have access"
- "Check for typos in the repository URL"
```

### Progress Feedback System
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSyncProgress, float, ProgressPercent, FString, CurrentOperation);

class UGitHubSyncManager 
{
public:
    UPROPERTY(BlueprintAssignable)
    FOnSyncProgress OnProgressUpdate;

    // Usage: OnProgressUpdate.Broadcast(25.0f, "Downloading files...");
};
```

---

## Configuration System

### Easy Setup Wizard
```cpp
UFUNCTION(BlueprintCallable, Category = "GitHub Sync|Setup")
bool RunSetupWizard(
    FString& SetupResult
);
```

Steps:
1. **GitHub Account Validation**: Test basic GitHub connectivity
2. **Token Creation Guide**: Step-by-step token creation
3. **Repository Selection**: Choose or create repository
4. **Initial Sync**: Optional first download/upload
5. **Auto-Sync Setup**: Configure automatic synchronization

### Settings Storage
```cpp
UCLASS(Config = GitHubSync)
class UGitHubSyncSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Connection")
    FString DefaultRepositoryURL;

    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (PasswordField = true))
    FString AccessToken;

    UPROPERTY(Config, EditAnywhere, Category = "Sync")
    bool bAutoSyncOnProjectOpen = false;

    UPROPERTY(Config, EditAnywhere, Category = "Sync")
    float AutoSyncInterval = 300.0f; // 5 minutes

    UPROPERTY(Config, EditAnywhere, Category = "Safety")
    bool bCreateBackupBeforeSync = true;
};
```

---

## Safety Features

### 1. **Automatic Backups**
Before any destructive operation:
```cpp
bool CreateAutomaticBackup()
{
    FString BackupPath = FPaths::ProjectDir() + TEXT("Backups/") + 
                        FDateTime::Now().ToString(TEXT("yyyy-MM-dd_HH-mm-ss"));
    // Create backup using existing ZIP functionality
}
```

### 2. **Dry Run Mode**
All operations support preview mode:
```cpp
bool PreviewMode = true; // Show what would happen without doing it
```

### 3. **Undo System**
Track operations for potential rollback:
```cpp
struct FSyncOperation
{
    FString OperationType;      // "Download", "Upload", "Sync"
    FDateTime Timestamp;
    FString BackupPath;         // For rollback
    TArray<FString> AffectedFiles;
};
```

---

## Performance Considerations

### 1. **Batch Operations**
Group file operations to reduce API calls:
```cpp
// Instead of one API call per file
// Batch files into single tree/commit operations
```

### 2. **Incremental Sync**
Only sync changed files:
```cpp
bool bIncrementalOnly = true; // Only sync files that actually changed
```

### 3. **Background Processing**
Keep UI responsive during operations:
```cpp
// All network operations run on background threads
// UI updates via progress delegates
```

---

## Testing Strategy

### Unit Tests for Each Function
```cpp
// Test basic functionality
bool TestEasyDownload();
bool TestEasyUpload();  
bool TestPreviewChanges();

// Test error scenarios
bool TestInvalidToken();
bool TestNetworkFailure();
bool TestPermissionDenied();
```

### Integration Tests
```cpp
// Test complete workflows
bool TestFullSyncWorkflow();
bool TestSetupWizardFlow();
bool TestErrorRecovery();
```

---

## Documentation for End Users

### Quick Start Guide
1. **Get GitHub Token**: Link to GitHub documentation
2. **Enter Repository URL**: Format examples
3. **Test Connection**: Use validation function
4. **First Download**: Get initial copy
5. **Make Changes**: Edit your files
6. **Upload Changes**: Sync back to GitHub

### Video Tutorial Suggestions
1. "Setting up GitHub Sync in 5 minutes"
2. "Your first download and upload"
3. "Understanding sync conflicts"
4. "Advanced features and settings"

---

This architecture prioritizes ease of use while maintaining the power and flexibility needed for serious development work. The key is hiding complexity behind simple interfaces while providing clear feedback and guidance when things go wrong.
