# Unreal Engine C++ Documentation Template

## Overview

This template is based on the well-structured documentation found in `RSSSyncGuidance.cpp` and provides guidelines for standardizing documentation across the HTTP Requester project and future Unreal Engine plugins.

## Documentation Structure Hierarchy

### 1. File Header Documentation
Every source file should begin with comprehensive header documentation:

```cpp
/**
 * [File Name] - [Brief Purpose Description]
 * 
 * [Detailed description of what this file provides - 2-3 sentences explaining
 * the overall functionality and how it fits into the larger system]
 * 
 * Author: [Author Name]
 * Date: [Creation Date]
 * 
 * Prerequisites:
 * - [Prerequisite 1: e.g., HTTPManager project with existing utilities]
 * - [Prerequisite 2: e.g., GitHub repository with branch access]
 * - [Prerequisite 3: e.g., Personal Access Token for GitHub API]
 * 
 * IMPLEMENTATION LIKELIHOOD: [Percentage]%
 * [Brief assessment of implementation difficulty and current project readiness]
 */
```

### 2. Include Statements and Dependencies
Document major dependencies and their purposes:

```cpp
#include "Engine/Engine.h"      // Core UE5 engine functionality
#include "Http.h"               // HTTP request handling
#include "Json.h"               // JSON parsing and creation
#include "HAL/FileManager.h"    // File system operations
#include "Misc/Paths.h"         // Path manipulation utilities

// External function declarations from other project files
extern void ExternalFunction_UTIL(const FString& Parameter);
extern FString AnotherUtility_UTIL(const TArray<FString>& Input);
```

### 3. Data Structures and Enums
Document all custom structures with clear purpose and usage:

```cpp
// Structure to hold [specific information] for [specific workflow]
struct FExampleInfo
{
    FString Path;           // Relative path to the file/resource
    FString SHA;            // Unique identifier (e.g., Git SHA hash)
    FString Mode;           // File mode (100644 for regular files, 100755 for executable)
};

// Shared state for async [operation] operations
struct FExampleState
{
    TArray<FExampleInfo> InfoArray;    // Collection of processed items
    int32 CompletedItems = 0;          // Progress counter
    int32 TotalItems = 0;              // Total items to process
    bool bHasError = false;            // Error flag for batch operations
};
```

## Function Documentation Standards

### Public API Functions (Main Interface)

Public functions that users will call directly need **comprehensive documentation**:

```cpp
/**
 * Step [N]: [Function Purpose Title]
 * PRODUCTION NOTE: [Status - e.g., "Passed the test", "Failed the test", "Under development"]
 * 
 * [Detailed description of what this function does, why it's needed, and how it fits
 * into the overall workflow. 2-3 sentences explaining the core functionality.]
 * 
 * BEGINNER NOTE: [Explanation for non-technical users. Define any technical terms,
 * explain concepts that might be unfamiliar, provide context about why this step
 * is necessary. Include analogies if helpful.]
 * 
 * [Additional technical details, workflow steps, or important considerations.
 * Explain any complex logic or potential gotchas.]
 * 
 * @param Parameter1 - [Type and purpose] (e.g., "GitHub username/organization")
 * @param Parameter2 - [Type and purpose] (e.g., "Repository name")
 * @param Parameter3 - [Type and purpose with examples] (e.g., "Branch to download (e.g., 'main', 'develop')")
 * @param Parameter4 - [Type and purpose with requirements] (e.g., "GitHub Personal Access Token (optional for public repos)")
 * @param CallbackParam - [Callback description] (e.g., "Callback function when operation finishes")
 * @return [Return type and meaning] (e.g., "FString - The full path to the created folder, empty on failure")
 */
ReturnType PublicFunctionName(const FString& Parameter1, const FString& Parameter2, 
                             const FString& Parameter3, const FString& Parameter4,
                             TFunction<void(bool)> CallbackParam)
```

### Helper Functions (Internal Implementation)

Helper functions that support main functions need **focused documentation**:

```cpp
/**
 * Helper Function: [Clear Purpose Statement]
 * 
 * [1-2 sentence description of what this helper does and why it exists.
 * Focus on the specific role it plays in the larger workflow.]
 * 
 * BEGINNER NOTE: [Optional - include if the helper involves complex concepts
 * that beginners might not understand. Explain technical terms or concepts.]
 * 
 * [Optional: Brief workflow explanation if the helper has multiple steps]
 * 
 * @param Parameter1 - [Type and purpose]
 * @param Parameter2 - [Type and purpose]
 * @param CallbackParam - [Callback description with parameter meanings]
 */
void HelperFunctionName(const FString& Parameter1, const FString& Parameter2,
                       TFunction<void(bool, FString)> CallbackParam)
```

### Utility Functions (Single Purpose)

Simple utility functions need **minimal but clear documentation**:

```cpp
/**
 * Utility Function: [Clear Purpose Statement]
 * 
 * [One sentence describing what it does and when to use it.]
 * 
 * @param InputParam - [Type and purpose]
 * @return [Return type and meaning]
 */
ReturnType UtilityFunctionName(const InputType& InputParam)
```

## Documentation Content Guidelines

### 1. Writing Style

#### Tone and Clarity
- **Use clear, direct language** - avoid unnecessary jargon
- **Write for beginners** - assume readers are learning UE5 and Git concepts
- **Be specific** - provide concrete examples rather than abstract descriptions
- **Stay concise** - comprehensive but not verbose

#### Technical Terminology
- **Define acronyms** on first use (e.g., "SHA (Secure Hash Algorithm)")
- **Explain concepts** that might be unfamiliar (e.g., "blob", "tree", "commit")
- **Provide context** for why technical concepts matter
- **Use analogies** when helpful (e.g., "manifest is like a table of contents")

### 2. Parameter Documentation

#### Format
```cpp
@param ParameterName - [Type] [Purpose] [Additional context/examples/requirements]
```

#### Examples
```cpp
@param RepoOwner - GitHub username or organization name (e.g., "microsoft", "epic-games")
@param AccessToken - GitHub Personal Access Token (optional for public repos, required for private)
@param TempFolder - Absolute path to temporary directory (will be created if it doesn't exist)
@param OnComplete - Callback function with (bSuccess, ErrorMessage) - ErrorMessage empty on success
```

### 3. Beginner Notes

Include "BEGINNER NOTE" sections for:
- **Complex technical concepts** (async operations, callbacks, Git terminology)
- **Prerequisites or setup requirements** (access tokens, repository permissions)
- **Common pitfalls or gotchas** (rate limiting, file path limitations)
- **Workflow context** (why this step is necessary in the larger process)

#### Examples
```cpp
// BEGINNER NOTE: Async tasks in UE5 don't block the main thread, allowing the 
// editor to remain responsive while operations complete in the background.

// BEGINNER NOTE: You'll need a GitHub Personal Access Token for private repositories.
// Generate one at: GitHub Settings > Developer settings > Personal access tokens

// BEGINNER NOTE: A manifest is like a "table of contents" for your files,
// listing each file with its location and a hash to detect changes.
```

### 4. Production Notes

Include production status for tracking implementation progress:
- **"Passed the test"** - Function is working and verified
- **"Failed the test"** - Function has issues that need resolution  
- **"Under development"** - Function is incomplete or experimental
- **"Requires adjustment"** - Function works but needs refinement

## Code Comment Standards

### Inline Comments

#### Complex Logic Blocks
```cpp
// GitHub API workflow for updating files:
// 1. Get the current branch reference
// 2. Get the commit tree for that reference  
// 3. Create new blobs for changed files
// 4. Create a new tree with updated blobs
// 5. Create a new commit pointing to the new tree
// 6. Update the branch reference to point to the new commit
```

#### Variable Explanations
```cpp
FString DownloadURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/zipball/%s"), 
                                     *RepoOwner, *RepoName, *BranchName);
// Format: https://api.github.com/repos/{owner}/{repo}/zipball/{branch}
```

#### Async Operation Context
```cpp
Request->OnProcessRequestComplete().BindLambda([TempFolder, OnDownloadComplete]
    (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // This lambda executes when the HTTP request completes
    // Variables captured: TempFolder, OnDownloadComplete
    // Parameters provided: Request, Response, bWasSuccessful
```

### Section Headers

Use clear section headers to organize code:

```cpp
// =============================================================================
// DOWNLOAD WORKFLOW FUNCTIONS
// =============================================================================

// =============================================================================  
// UPLOAD WORKFLOW FUNCTIONS
// =============================================================================

// =============================================================================
// UTILITY AND HELPER FUNCTIONS
// =============================================================================
```

## Example Documentation Template

Here's a complete example following all guidelines:

```cpp
/**
 * Example Sync Manager - GitHub Integration for UE5
 * 
 * This file provides comprehensive GitHub integration functionality for Unreal Engine 5
 * projects, allowing developers to sync project files with GitHub repositories without
 * leaving the editor environment.
 * 
 * Author: Development Team
 * Date: June 19, 2025
 * 
 * Prerequisites:
 * - Unreal Engine 5.1 or higher
 * - GitHub repository with appropriate access permissions
 * - GitHub Personal Access Token for private repositories
 * - HTTP and JSON modules enabled in project
 * 
 * IMPLEMENTATION LIKELIHOOD: 85%
 * Core functionality is stable, requires UI polish and error handling improvements.
 */

#include "Engine/Engine.h"          // Core UE5 functionality
#include "Http.h"                   // HTTP request management
#include "Json.h"                   // JSON parsing and serialization

// External dependencies from other project modules
extern FString CalculateFileHash_UTIL(const FString& FilePath);
extern void CreateZip_UTIL(const TArray<FString>& Files, const FString& OutputPath);

/**
 * Step 1: Initialize Project Sync with GitHub
 * PRODUCTION NOTE: Passed the test.
 * 
 * Establishes connection between local UE5 project and GitHub repository.
 * Validates credentials, checks repository access, and prepares sync environment.
 * 
 * BEGINNER NOTE: This is your starting point for GitHub integration. The function
 * will test your access token and make sure everything is configured correctly
 * before attempting any file operations.
 * 
 * @param RepoOwner - GitHub username or organization (e.g., "epic-games")
 * @param RepoName - Repository name (e.g., "unreal-tournament")  
 * @param AccessToken - GitHub Personal Access Token with repo permissions
 * @param OnInitComplete - Callback with (bSuccess, ErrorMessage) for result handling
 * @return bool - True if initialization started successfully (actual result in callback)
 */
bool InitializeGitHubSync(const FString& RepoOwner, const FString& RepoName,
                         const FString& AccessToken, 
                         TFunction<void(bool, FString)> OnInitComplete)
{
    // Validation logic here...
    
    // BEGINNER NOTE: This function returns immediately but the real work happens
    // asynchronously. Check the OnInitComplete callback for the actual result.
    return true;
}

/**
 * Helper Function: Validate Repository Access
 * 
 * Tests GitHub API access with provided credentials and repository information.
 * Makes a lightweight API call to verify permissions before attempting heavy operations.
 * 
 * BEGINNER NOTE: GitHub has different permission levels (read, write, admin).
 * This function ensures your token has enough permissions for the operations you want to perform.
 * 
 * @param RepoOwner - GitHub username or organization name
 * @param RepoName - Repository name  
 * @param AccessToken - GitHub Personal Access Token
 * @param OnValidated - Callback with (bHasAccess, RequiredPermissions, ErrorDetails)
 */
void ValidateRepositoryAccess(const FString& RepoOwner, const FString& RepoName,
                             const FString& AccessToken,
                             TFunction<void(bool, FString, FString)> OnValidated)
{
    // Implementation here...
}
```

## Quality Checklist

Before finalizing documentation, verify:

### Content Quality
- [ ] **Purpose is clear** - anyone can understand what the function does
- [ ] **Parameters explained** - all inputs have clear descriptions and examples
- [ ] **Return values documented** - outputs and callbacks are explained
- [ ] **Prerequisites listed** - users know what they need before calling
- [ ] **Beginner notes included** - complex concepts are explained simply

### Technical Accuracy  
- [ ] **Parameter types match** - documentation reflects actual function signature
- [ ] **Production status accurate** - testing status is current and correct
- [ ] **Dependencies documented** - external functions and includes are explained
- [ ] **Error conditions mentioned** - potential failure modes are described

### Consistency
- [ ] **Formatting matches template** - structure follows established patterns
- [ ] **Terminology consistent** - same terms used throughout project
- [ ] **Style consistent** - tone and approach match other documentation
- [ ] **Examples relevant** - code samples use project-appropriate context

This template ensures that all documentation in the HTTP Requester project maintains the high standard established in RSSSyncGuidance.cpp while providing clear guidance for future development and commercial plugin preparation.
