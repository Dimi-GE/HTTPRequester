# RSSSyncGuidance.cpp - Complete Study Guide for Beginners

## Overview

This document provides a comprehensive, beginner-friendly explanation of every function in `RSSSyncGuidance.cpp`. The system creates a GitHub-based source control solution within Unreal Engine 5, allowing users to synchronize files between their local project and GitHub repositories without needing deep knowledge of Git or GitHub API.

## Prerequisites You Need to Understand

### 1. **Access Tokens**
- **What it is**: A special password that GitHub generates for you
- **Why needed**: GitHub requires authentication for accessing repositories
- **How to get**: GitHub Settings → Developer settings → Personal access tokens
- **Types**: 
  - `public_repo` scope: For public repositories
  - `repo` scope: For private repositories (includes public access)

### 2. **Async Programming (TFunction)**
- **What it is**: Code that doesn't wait for operations to complete before continuing
- **Why used**: Network requests (downloading/uploading) take time - we don't want to freeze the editor
- **TFunction**: Unreal Engine's way of storing a function that gets called later when an operation completes
- **Example**: `TFunction<void(bool)> OnComplete` means "store a function that takes a boolean and returns nothing"

### 3. **External Dependencies**
These functions come from other files in your project:
- `CreateZip_Structured`: Makes ZIP files from folders
- `UnpackZip`: Extracts ZIP files
- `CalculateFileHash_UTIL`: Creates unique fingerprints for files to detect changes
- `OpenFolderDialog_UTIL`: Shows folder picker dialog

---

## Core Data Structures

### `FBlobInfo`
```cpp
struct FBlobInfo {
    FString Path;     // File path relative to repository root
    FString SHA;      // GitHub's unique identifier for the file content
    FString Mode;     // File permissions (100644 = normal file)
};
```
**Purpose**: Stores information about a file that's being uploaded to GitHub

### `FBlobCreationState`
```cpp
struct FBlobCreationState {
    TArray<FBlobInfo> BlobInfos;  // List of all files being processed
    int32 CompletedBlobs;         // How many files are done
    int32 TotalBlobs;             // Total number of files
    bool bHasError;               // Whether any errors occurred
};
```
**Purpose**: Tracks progress when uploading multiple files simultaneously

---

## Function Categories

## 1. FOUNDATION FUNCTIONS

### `CreateRSSSyncTempFolder()`
**What it does**: Creates a clean temporary folder for download operations
**Why important**: Ensures clean separation between sync operations
**Returns**: Path to the created folder
**Beginner tip**: Think of this as creating a "workspace" where downloaded files are temporarily stored

---

## 2. DOWNLOAD FUNCTIONS

### `DownloadBranchFromGitHub()`
**What it does**: Downloads a GitHub repository branch as a ZIP file
**Parameters**:
- `RepoOwner`: GitHub username (e.g., "microsoft")
- `RepoName`: Repository name (e.g., "unreal-engine")
- `BranchName`: Which branch to download (e.g., "main")
- `AccessToken`: Your GitHub access token
- `TempFolder`: Where to save the downloaded ZIP
- `OnDownloadComplete`: Function to call when download finishes

**Async Behavior**: This function starts the download and returns immediately. Your `OnDownloadComplete` function gets called later when the download finishes.

**Status**: DEPRECATED - Use `DownloadBranchWithValidation()` instead

### `DownloadBranchWithValidation()`
**What it does**: Enhanced version that validates your access token before downloading
**Why better**: Prevents failed downloads due to invalid tokens
**Callback**: `TFunction<void(bool, FString)>` - receives success status AND error message

**Beginner explanation**: 
1. First checks if your "key" (access token) works
2. If yes, downloads the repository
3. If no, tells you exactly what's wrong

### `ValidateGitHubTokenAccess()`
**What it does**: Tests if your access token can access a specific repository
**How it works**: Makes a simple API call to GitHub to test permissions
**Response codes**:
- 200: Success - token works
- 401: Token is invalid or expired
- 403: Token lacks permissions
- 404: Repository not found or no access

---

## 3. FILE PROCESSING FUNCTIONS

### `UnpackDownloadedBranch()`
**What it does**: Extracts the downloaded ZIP file and cleans up
**Why needed**: Downloaded repositories come as ZIP files that need to be extracted
**Returns**: `true` if successful, `false` if failed

### `CreateManifest()`
**What it does**: Creates a "table of contents" (manifest) for all files in a folder
**What's a manifest**: A JSON file listing every file with its unique fingerprint (hash)
**Why important**: Used to detect which files have changed between local and remote versions

**Process**:
1. Scans all files in a directory
2. Calculates a unique hash for each file
3. Organizes files by directory
4. Saves everything to `RSSManifest.json`

---

## 4. COMPARISON FUNCTIONS

### `CompareManifestsAndFindDifferences()`
**What it does**: Compares local and remote manifests to find what changed
**Parameters**:
- `bExecuteChanges`: If `false`, just analyze; if `true`, prepare for applying changes
- `TempFolder`: Where to find the remote manifest

**Output**: Array of difference strings in format: `ACTION|FilePath|Reason|Priority`

**Actions**:
- `ADD`: File exists remotely but not locally
- `UPDATE`: File exists in both but content differs
- `REMOVE`: File exists locally but not remotely

**Beginner analogy**: Like comparing two photo albums to see which pictures are new, changed, or missing

### `SaveDifferencesToFile()`
**What it does**: Saves the comparison results to a JSON file for review
**Why useful**: Lets you see exactly what will change before applying updates
**Output file**: `RSSDifferences.json`

---

## 5. SYNCHRONIZATION FUNCTIONS

### `ApplyFileChanges()`
**What it does**: Actually performs the file operations (add, update, remove)
**Parameters**:
- `TempFolder`: Source of difference analysis
- `bUpdateLocal`: If `true`, updates local files; if `false`, updates remote files

**Warning**: This function actually modifies files - always backup first!

**Process**:
1. Reads `RSSDifferences.json`
2. For each difference, performs the appropriate action
3. Logs success/failure for each operation

### `UpdateManifestsAfterChanges()`
**What it does**: Regenerates manifests after file synchronization
**Why needed**: Ensures manifests reflect the current state of files
**Process**:
1. Deletes old local manifest
2. Creates new local manifest
3. If remote files changed, creates new remote manifest

### `PackUpdatedBranchToZip()`
**What it does**: Creates a ZIP file from updated files, ready for upload
**Uses**: Existing `CreateZip_Structured` function
**Output**: `updated_branch.zip` in the temp folder

---

## 6. UPLOAD WORKFLOW FUNCTIONS (5-Step Process)

### Step 1: `UploadFilesAndCreateCommit()`
**What it does**: Main coordinator for the upload process
**Process**:
1. Reads all files from extracted folder
2. Creates relative paths for GitHub
3. Initiates blob creation process

### Step 2: `CreateBlobsForFiles()`
**What it does**: Creates GitHub "blob" objects for each file
**What's a blob**: Git's way of storing file content with a unique identifier
**Process**:
1. Creates shared state to track progress
2. Calls `CreateSingleBlob()` for each file
3. Waits for all blobs to complete

### `CreateSingleBlob()` (Helper)
**What it does**: Creates a single blob object on GitHub
**Process**:
1. Reads file content
2. Converts to Base64 encoding (GitHub requirement)
3. Uploads to GitHub via API
4. Stores the returned SHA (unique identifier)

### Step 3: `CreateTreeWithBlobs()`
**What it does**: Creates a GitHub "tree" object that references all blobs
**What's a tree**: Git's way of representing a folder structure
**Process**:
1. Creates JSON with all blob references
2. Uploads tree structure to GitHub
3. Receives tree SHA for next step

### Step 4: `CreateCommitWithTreeSHA()`
**What it does**: Creates a GitHub "commit" object that points to the tree
**What's a commit**: Git's way of saving a snapshot with metadata
**Process**:
1. Creates commit with message and tree reference
2. Links to parent commit (maintains history)
3. Receives commit SHA for final step

### Step 5: `UpdateBranchReference()`
**What it does**: Updates the branch to point to the new commit
**Process**:
1. Updates branch reference using PATCH request
2. Completes the upload process
3. Files are now visible on GitHub

---

## 7. ENHANCED FUNCTIONS

### `UploadUpdatedBranchToGitHub()`
**What it does**: Simplified upload function that uses extracted folders
**Key feature**: Expects files to already be extracted (not in ZIP format)
**Process**:
1. Gets current branch reference
2. Calls the 5-step upload workflow

### `UploadUpdatedBranchWithValidation()`
**What it does**: Enhanced upload with permission validation
**Why important**: Prevents upload failures due to insufficient permissions
**Process**:
1. Validates write permissions first
2. If valid, proceeds with upload
3. Provides detailed error messages for troubleshooting

---

## 8. TEST FUNCTIONS

### `TestCompleteUploadWorkflow()`
**What it does**: Tests the complete 5-step upload process
**Use case**: Validates that all upload components work together
**Parameters**: Uses test values that you can modify for your repository

---

## Understanding Async Flow

### Why Async?
Network operations (downloading from/uploading to GitHub) take time. Without async:
- Editor would freeze during operations
- User interface would become unresponsive
- Large files would cause timeouts

### How Async Works Here:
1. Function starts operation and returns immediately
2. Operation continues in background
3. When finished, calls your provided callback function
4. Callback receives success/failure status

### Callback Pattern Example:
```cpp
// This function returns immediately
DownloadBranchWithValidation(owner, repo, branch, token, folder,
    [](bool success, FString error) {
        // This code runs later when download completes
        if (success) {
            UE_LOG(LogTemp, Warning, TEXT("Download completed!"));
        } else {
            UE_LOG(LogTemp, Error, TEXT("Failed: %s"), *error);
        }
    });
// Code here runs immediately, before download completes
```

---

## Error Handling Patterns

### HTTP Response Codes:
- **200**: Success (GET requests)
- **201**: Created (POST requests)
- **401**: Unauthorized (invalid token)
- **403**: Forbidden (insufficient permissions)
- **404**: Not found (repository/branch doesn't exist)

### Common Error Scenarios:
1. **Invalid Token**: Check token hasn't expired
2. **Wrong Permissions**: Ensure token has required scopes
3. **Network Issues**: Check internet connection
4. **File Access**: Ensure Unreal has permission to read/write files

---

## Best Practices for Beginners

### 1. **Always Test Small First**
- Start with a small test repository
- Test download before upload
- Verify token permissions

### 2. **Read the Logs**
- All functions provide detailed logging
- Use `UE_LOG` messages to understand what's happening
- Look for "RSSSync:" prefix in logs

### 3. **Backup Important Files**
- Always backup before running sync operations
- Test with non-critical projects first

### 4. **Understand Prerequisites**
- Valid GitHub account
- Repository access
- Proper access token with correct scopes

---

## Common Beginner Mistakes

### 1. **Wrong Token Scope**
- Public repos need `public_repo` scope
- Private repos need `repo` scope

### 2. **Path Issues**
- Use forward slashes (/) in paths, not backslashes (\)
- Ensure paths are relative to repository root

### 3. **Async Confusion**
- Don't expect immediate results from async functions
- Always use callbacks to handle completion

### 4. **Missing Error Handling**
- Always check error messages in callbacks
- Don't ignore failed operations

---

## Next Steps

After understanding these functions:
1. Set up a test GitHub repository
2. Generate an access token with appropriate permissions
3. Start with download functions (`DownloadBranchWithValidation`)
4. Practice with manifest creation and comparison
5. Progress to upload functions when comfortable

Remember: This system is designed to be beginner-friendly, but GitHub and Git concepts take time to understand. Start simple and gradually work up to more complex operations.
