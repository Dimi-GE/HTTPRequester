/**
 * RSS Sync Guidance - Complete Implementation Guide
 * 
 * This file provides a comprehensive, beginner-friendly guide for implementing 
 * a GitHub-based source control system within Unreal Engine 5. The system allows 
 * downloading, comparing, and synchronizing files between local project and GitHub repository.
 * 
 * Author: GitHub Copilot
 * Date: June 13, 2025
 * 
 * Prerequisites:
 * - HTTPManager project with existing CustomUtilities.cpp, ZipHandler.cpp, MacrosManager.cpp
 * - GitHub repository with branch access
 * - Personal Access Token for GitHub API
 * 
 * IMPLEMENTATION LIKELIHOOD: 75%
 * You have good foundation with existing utilities, but will need to add HTTP functionality
 * and learn GitHub API integration. The existing hash calculation and ZIP handling 
 * provide excellent building blocks.
 */

#include "Engine/Engine.h"
#include "Http.h"
#include "Json.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/Base64.h"

// External function declarations from existing files
extern void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath);
extern void UnpackZip(const FString& ZipPath, const FString& TempDir);
extern FString CalculateFileHash_UTIL(const FString& FilePath);
extern FString CalculateDirectoryHash_UTIL(const TMap<FString, FString>& FileHashes);
extern void RSSManifestInit_UTIL();
extern FString OpenFolderDialog_UTIL();

// Structure to hold blob information for GitHub upload workflow
struct FBlobInfo
{
    FString Path;
    FString SHA;
    FString Mode; // File mode (100644 for regular files, 100755 for executable, 040000 for directories)
};

// Shared state for async blob creation operations
struct FBlobCreationState
{
    TArray<FBlobInfo> BlobInfos;
    int32 CompletedBlobs = 0;
    int32 TotalBlobs = 0;
    bool bHasError = false;
};

// Forward declarations for upload workflow functions
void CreateBlobsForFiles(const FString& RepoOwner, const FString& RepoName, const FString& BranchName, 
                        const FString& AccessToken, const TArray<FString>& FilePaths, const TArray<FString>& RelativePaths,
                        const FString& CommitMessage, const FString& ParentSHA, TFunction<void(bool)> OnComplete);

void CreateSingleBlob(const FString& RepoOwner, const FString& RepoName, const FString& AccessToken,
                     const FString& FilePath, int32 BlobIndex, TSharedPtr<FBlobCreationState> State,
                     TFunction<void(TSharedPtr<FBlobCreationState>)> OnAllBlobsComplete);

void CreateTreeWithBlobs(const FString& RepoOwner, const FString& RepoName, const FString& BranchName,
                        const FString& AccessToken, const TArray<FBlobInfo>& BlobInfos, 
                        const FString& CommitMessage, const FString& ParentSHA, TFunction<void(bool)> OnComplete);

void CreateCommitWithTreeSHA(const FString& RepoOwner, const FString& RepoName, const FString& BranchName,
                            const FString& AccessToken, const FString& CommitMessage, const FString& TreeSHA, 
                            const FString& ParentSHA, TFunction<void(bool)> OnComplete);

void UpdateBranchReference(const FString& RepoOwner, const FString& RepoName, const FString& BranchName, 
                          const FString& AccessToken, const FString& CommitSHA, TFunction<void(bool)> OnComplete);

/**
 * Step 1: Create Temporary Folder for RSS Synchronization
 * PRODUCTION NOTE: Passed the test.
 * Creates a temporary folder named 'RSSSync' where downloaded ZIP files will be 
 * stored and unpacked. This ensures clean separation between sync operations.
 * 
 * @return FString - The full path to the created RSSSync folder
 * 
 */
FString CreateRSSSyncTempFolder()
{
    // Get the project's temp directory path
    FString TempDir = FPaths::ProjectDir() + TEXT("Temp/RSSSync/");
    
    // Ensure the directory structure exists
    IFileManager& FileManager = IFileManager::Get();
    
    // Remove existing RSSSync folder if it exists to ensure clean state
    if (FileManager.DirectoryExists(*TempDir))
    {
        FileManager.DeleteDirectory(*TempDir, false, true);
        UE_LOG(LogTemp, Warning, TEXT("RSSSync: Cleaned existing temp folder"));
    }
    
    // Create fresh directory
    if (FileManager.MakeDirectory(*TempDir, true))
    {
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Created temp folder at %s"), *TempDir);
        return TempDir;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to create temp folder"));
        return FString();
    }
}

/**
 * Step 2: Download Branch from GitHub
 * PRODUCTION NOTE: Failed the test.
 * PRODUCTION NOTE: DEPRECATED - use DownloadBranchWithValidation(. . .) instead.
 * Downloads a specific branch from GitHub repository as a ZIP file.
 * Uses GitHub API to fetch the branch archive and saves it to the RSSSync folder.
 * 
 * BEGINNER NOTE: You'll need a GitHub Personal Access Token for private repositories.
 * Generate one at: GitHub Settings > Developer settings > Personal access tokens
 * 
 * @param RepoOwner - GitHub username/organization (e.g., "microsoft")
 * @param RepoName - Repository name (e.g., "unreal-engine-plugin") 
 * @param BranchName - Branch to download (e.g., "main", "develop")
 * @param AccessToken - GitHub Personal Access Token (optional for public repos)
 * @param TempFolder - Path where to save the downloaded ZIP
 * @param OnDownloadComplete - Callback function when download finishes
 */
void DownloadBranchFromGitHub(const FString& RepoOwner, const FString& RepoName, 
                             const FString& BranchName, const FString& AccessToken,                             const FString& TempFolder, TFunction<void(bool)> OnDownloadComplete)
{
    // Construct GitHub API URL for branch archive download
    // Format: https://api.github.com/repos/{owner}/{repo}/zipball/{branch}
    FString DownloadURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/zipball/%s"), 
                                         *RepoOwner, *RepoName, *BranchName);
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Downloading from %s"), *DownloadURL);
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Starting download request for %s/%s:%s"), *RepoOwner, *RepoName, *BranchName);
    
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    
    // Set request parameters    Request->SetURL(DownloadURL);
    Request->SetVerb(TEXT("GET"));
    
    // Add authorization header if access token provided
    if (!AccessToken.IsEmpty())
    {
        FString AuthHeader = FString::Printf(TEXT("token %s"), *AccessToken);
        Request->SetHeader(TEXT("Authorization"), AuthHeader);
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Using access token for authentication"));
    }
    
    // Set user agent (GitHub requires this)
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    
    // Set timeout to prevent hanging (30 seconds)
    Request->SetTimeout(30.0f);
    
    // Add progress delegate for monitoring
    Request->OnRequestProgress64().BindLambda([RepoOwner, RepoName](FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived)
    {
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Download progress for %s/%s - Received: %llu bytes"), *RepoOwner, *RepoName, BytesReceived);
    });
    
    // Bind response handler
    Request->OnProcessRequestComplete().BindLambda([TempFolder, OnDownloadComplete, RepoOwner, RepoName, BranchName]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("RSSSync: Download request completed for %s/%s:%s"), *RepoOwner, *RepoName, *BranchName);
        
        bool bDownloadSuccess = false;
        
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            
            if (ResponseCode == 200)
            {
                // Get the binary content (ZIP file)
                TArray<uint8> ZipData = Response->GetContent();
                
                // Save ZIP file to temp folder
                FString ZipPath = TempFolder + TEXT("branch_download.zip");
                
                if (FFileHelper::SaveArrayToFile(ZipData, *ZipPath))
                {
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: Successfully downloaded ZIP (%d bytes) to %s"), 
                           ZipData.Num(), *ZipPath);
                    bDownloadSuccess = true;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to save downloaded ZIP"));
                }
            }
            else if (ResponseCode == 401)
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Authentication failed - check access token"));
            }
            else if (ResponseCode == 404)
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Repository or branch not found"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Download failed with HTTP %d"), ResponseCode);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network request failed"));
        }
        
        // Call completion callback
        OnDownloadComplete(bDownloadSuccess);
    });
      // Start the download
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: ProcessRequest() called - Request should be non-blocking"));
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to start HTTP request"));
        OnDownloadComplete(false);
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: HTTP request started successfully, function returning immediately"));
}

/**
 * Step 2.5: Enhanced Download Function with Token Validation
 * PRODUCTION NOTE: Passed the test.
 * This function first validates the token permissions, then proceeds with download.
 * Essential for private repositories.
 */

 // Forward declaration of validation function
void ValidateGitHubTokenAccess(const FString& RepoOwner, const FString& RepoName, 
                               const FString& AccessToken, TFunction<void(bool, FString)> OnValidationComplete);

void DownloadBranchWithValidation(const FString& RepoOwner, const FString& RepoName, 
                                 const FString& BranchName, const FString& AccessToken,
                                 const FString& TempFolder, TFunction<void(bool, FString)> OnComplete)
{
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Starting validation for %s/%s (Private repo support)"), *RepoOwner, *RepoName);
    
    // First validate token access
    ValidateGitHubTokenAccess(RepoOwner, RepoName, AccessToken, 
        [RepoOwner, RepoName, BranchName, AccessToken, TempFolder, OnComplete]
        (bool bHasAccess, FString ErrorMessage)
        {
            if (bHasAccess)
            {
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Token validated successfully, proceeding with download"));
                
                // Proceed with download
                DownloadBranchFromGitHub(RepoOwner, RepoName, BranchName, AccessToken, TempFolder,
                    [OnComplete](bool bDownloadSuccess)
                    {
                        if (bDownloadSuccess)
                        {
                            OnComplete(true, TEXT("Download completed successfully"));
                        }
                        else
                        {
                            OnComplete(false, TEXT("Download failed"));
                        }
                    });
            }
            else
            {
                FString FullError = FString::Printf(TEXT("Token validation failed: %s"), *ErrorMessage);
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *FullError);
                OnComplete(false, FullError);
            }
        });
}

/**
 * Step 3: Unpack Downloaded Branch and Clean Up
 * PRODUCTION NOTE: Passed the test - requires paths adjustment.
 * Unpacks the downloaded ZIP file into the temp folder and removes the ZIP file
 * to save space. Uses the existing UnpackZip function.
 * 
 * @param TempFolder - Path to the RSSSync temp folder
 * @return bool - Success status of unpacking operation
 */
bool UnpackDownloadedBranch(const FString& TempFolder)
{
    FString ZipPath = TempFolder + TEXT("branch_download.zip");
    FString ExtractPath = TempFolder + TEXT("extracted/");
    
    // Check if ZIP file exists
    if (!FPaths::FileExists(ZipPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: ZIP file not found at %s"), *ZipPath);
        return false;
    }
    
    // Create extraction directory
    IFileManager::Get().MakeDirectory(*ExtractPath, true);
    
    try
    {
        // Use existing UnpackZip function
        UnpackZip(ZipPath, ExtractPath);
        
        // Remove the ZIP file to save space
        IFileManager::Get().Delete(*ZipPath);
        
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Successfully unpacked and cleaned ZIP"));
        return true;
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to unpack ZIP file"));
        return false;
    }
}

/**
 * Step 4: Create Remote Manifest from Downloaded Files
 * NOTE: Passed the test.
 * Scans the extracted files and creates a RemoteRSSManifest.json file
 * containing file hashes and directory structure. This manifest will be
 * compared with the local manifest to detect differences.
 * 
 * BEGINNER NOTE: A manifest is like a "table of contents" for your files,
 * containing checksums (hashes) to detect if files have changed.
 * 
 * PRODUCTION NOTE: USE THIS FUNCTION TO GENERATE A MANIFEST FOR THE PROJECT
 * 
 * @return bool - Success status of manifest creation
 */
bool CreateManifest()
{

    FString RepoPath = OpenFolderDialog_UTIL();

    if (RepoPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: No extracted path provided"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Creating manifest for %s"), *RepoPath);
    
    // Recursively find all files
    TArray<FString> AllFiles;
    IFileManager::Get().FindFilesRecursive(AllFiles, *RepoPath, TEXT("*"), true, false);
    
    // Calculate hashes for all files
    TMap<FString, FString> FileHashes;
    TMap<FString, TMap<FString, FString>> DirectoryStructure;
    
    for (const FString& FilePath : AllFiles)
    {
        // Calculate file hash using existing utility
        FString FileHash = CalculateFileHash_UTIL(FilePath);
        
        // Get relative path from repo root
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *RepoPath);
        
        // Store in our structure
        FileHashes.Add(RelativePath, FileHash);
        
        // Organize by directory for structured manifest
        FString Directory = FPaths::GetPath(RelativePath);
        FString FileName = FPaths::GetCleanFilename(RelativePath);
        
        if (Directory.IsEmpty())
        {
            Directory = TEXT("Root");
        }
        
        DirectoryStructure.FindOrAdd(Directory).Add(FileName, FileHash);
    }
    
    // Create JSON manifest structure
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject());
    TSharedPtr<FJsonObject> FilesObject = MakeShareable(new FJsonObject());
    
    // Add metadata
    MetadataObject->SetStringField(TEXT("created_date"), FDateTime::Now().ToIso8601());
    MetadataObject->SetNumberField(TEXT("total_files"), AllFiles.Num());
    MetadataObject->SetStringField(TEXT("manifest_type"), TEXT("remote"));
    
    RootObject->SetObjectField(TEXT("metadata"), MetadataObject);
    
    // Add directory structure
    for (const auto& DirPair : DirectoryStructure)
    {
        TSharedPtr<FJsonObject> DirObject = MakeShareable(new FJsonObject());
        
        // Calculate directory hash
        FString DirHash = CalculateDirectoryHash_UTIL(DirPair.Value);
        DirObject->SetStringField(TEXT("directory_hash"), DirHash);
        
        // Add files in this directory
        TSharedPtr<FJsonObject> DirFilesObject = MakeShareable(new FJsonObject());
        for (const auto& FilePair : DirPair.Value)
        {
            DirFilesObject->SetStringField(FilePair.Key, FilePair.Value);
        }
        DirObject->SetObjectField(TEXT("files"), DirFilesObject);
        
        FilesObject->SetObjectField(DirPair.Key, DirObject);
    }
    
    RootObject->SetObjectField(TEXT("directories"), FilesObject);
    
    // Save manifest to file
    FString ManifestPath = FPaths::ProjectDir() / TEXT("RSSManifest.json");
    UE_LOG(LogTemp, Warning, TEXT("%s."), *ManifestPath);
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    
    if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
    {
        if (FFileHelper::SaveStringToFile(OutputString, *ManifestPath))
        {
            UE_LOG(LogTemp, Log, TEXT("RSSSync: Created remote manifest with %d files"), AllFiles.Num());
            return true;
        }
    }
    
    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to save remote manifest"));
    return false;
}

/**
 * Step 5: Ensure Local Manifest Exists
 * NOTE: Failed the test.
 * PRODUCTION NOTE: DEPRECATED - Use CreateManifest instead.
 * Checks if LocalRSSManifest.json exists in the RSS folder. If not, creates it
 * using the existing RSSManifestInit function.
 * 
 * @return bool - Success status (true if manifest exists or was created successfully)
 */
bool EnsureLocalManifestExists()
{
    FString LocalManifestPath = FPaths::ProjectDir() + TEXT("RSS/LocalRSSManifest.json");
    
    // Check if local manifest already exists
    if (FPaths::FileExists(LocalManifestPath))
    {
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Local manifest already exists"));
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Local manifest not found, creating new one"));
    
    try
    {
        // Use existing utility to create manifest
        // Note: You may need to modify RSSManifestInit_UTIL to accept a specific directory
        // and output filename, as currently it's hardcoded
        RSSManifestInit_UTIL();
        
        // Check if creation was successful
        if (FPaths::FileExists(LocalManifestPath))
        {
            UE_LOG(LogTemp, Log, TEXT("RSSSync: Successfully created local manifest"));
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Local manifest creation failed"));
            return false;
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Exception during local manifest creation"));
        return false;
    }
}

/**
 * Step 6: Compare Manifests and Identify Differences
 * PRODUCTION NOTE: Passed the test.
 * Compares LocalRSSManifest and RemoteRSSManifest files by their hashes.
 * Identifies files that need to be added, updated, or removed based on timestamp priority.
 * 
 * BEGINNER NOTE: This function implements a "merge conflict resolution" strategy.
 * By default, newer files (based on modification time) take precedence.
 * 
 * @param bExecuteChanges - If false, only analyze differences. If true, prepare for execution.
 * @param TempFolder - Path to temp folder containing RemoteRSSManifest
 * @return TArray<FString> - List of files that need to be synchronized
 */

 // Forward declaration of utility function
void SaveDifferencesToFile(const TArray<FString>& Differences, const FString& TempFolder);

TArray<FString> CompareManifestsAndFindDifferences(bool bExecuteChanges, const FString& TempFolder)
{
    TArray<FString> DifferencesArray;
    
    // Load local manifest
    FString LocalManifestPath = FPaths::ProjectDir() + TEXT("RSSManifest.json");
    FString RemoteManifestPath = TempFolder + TEXT("RSSManifest.json");
    
    FString LocalJsonString, RemoteJsonString;
    
    if (!FFileHelper::LoadFileToString(LocalJsonString, *LocalManifestPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to load local manifest"));
        return DifferencesArray;
    }
    
    if (!FFileHelper::LoadFileToString(RemoteJsonString, *RemoteManifestPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to load remote manifest"));
        return DifferencesArray;
    }
    
    // Parse JSON manifests
    TSharedPtr<FJsonObject> LocalManifest, RemoteManifest;
    TSharedRef<TJsonReader<>> LocalReader = TJsonReaderFactory<>::Create(LocalJsonString);
    TSharedRef<TJsonReader<>> RemoteReader = TJsonReaderFactory<>::Create(RemoteJsonString);
    
    if (!FJsonSerializer::Deserialize(LocalReader, LocalManifest) ||
        !FJsonSerializer::Deserialize(RemoteReader, RemoteManifest))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to parse manifest JSON"));
        return DifferencesArray;
    }
    
    // Create difference analysis structure
    TArray<TSharedPtr<FJsonValue>> DifferencesJsonArray;
    
    // Get directories from both manifests
    TSharedPtr<FJsonObject> LocalDirs = LocalManifest->GetObjectField(TEXT("directories"));
    TSharedPtr<FJsonObject> RemoteDirs = RemoteManifest->GetObjectField(TEXT("directories"));
    
    if (!LocalDirs.IsValid() || !RemoteDirs.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Invalid manifest structure"));
        return DifferencesArray;
    }
    
    // Compare each directory in remote manifest
    for (const auto& RemoteDirPair : RemoteDirs->Values)
    {
        FString DirName = RemoteDirPair.Key;
        TSharedPtr<FJsonObject> RemoteDir = RemoteDirPair.Value->AsObject();
        
        if (!RemoteDir.IsValid()) continue;
        
        TSharedPtr<FJsonObject> RemoteFiles = RemoteDir->GetObjectField(TEXT("files"));
        if (!RemoteFiles.IsValid()) continue;
        
        // Check if directory exists in local manifest
        if (LocalDirs->HasField(DirName))
        {
            TSharedPtr<FJsonObject> LocalDir = LocalDirs->GetObjectField(DirName);
            TSharedPtr<FJsonObject> LocalFiles = LocalDir->GetObjectField(TEXT("files"));
            
            // Compare files in this directory
            for (const auto& RemoteFilePair : RemoteFiles->Values)
            {
                FString FileName = RemoteFilePair.Key;
                FString RemoteHash = RemoteFilePair.Value->AsString();
                
                if (LocalFiles->HasField(FileName))
                {
                    FString LocalHash = LocalFiles->GetStringField(FileName);
                    
                    // File exists in both - check if hashes differ
                    if (LocalHash != RemoteHash)
                    {
                        // Hashes differ - add to differences with priority logic
                        FString DifferenceEntry = FString::Printf(TEXT("UPDATE|%s/%s|HASH_MISMATCH|REMOTE_PRIORITY"), 
                                                                 *DirName, *FileName);
                        DifferencesArray.Add(DifferenceEntry);
                        
                        UE_LOG(LogTemp, Warning, TEXT("RSSSync: Hash mismatch for %s/%s"), *DirName, *FileName);
                    }
                }
                else
                {
                    // File exists in remote but not local - add to differences
                    FString DifferenceEntry = FString::Printf(TEXT("ADD|%s/%s|NEW_FILE|REMOTE_ORIGIN"), 
                                                             *DirName, *FileName);
                    DifferencesArray.Add(DifferenceEntry);
                    
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: New file to add: %s/%s"), *DirName, *FileName);
                }
            }
        }
        else
        {
            // Entire directory is new - add all files
            for (const auto& RemoteFilePair : RemoteFiles->Values)
            {
                FString FileName = RemoteFilePair.Key;
                FString DifferenceEntry = FString::Printf(TEXT("ADD|%s/%s|NEW_DIRECTORY|REMOTE_ORIGIN"), 
                                                         *DirName, *FileName);
                DifferencesArray.Add(DifferenceEntry);
            }
            
            UE_LOG(LogTemp, Log, TEXT("RSSSync: New directory to create: %s"), *DirName);
        }
    }
    
    // Check for files that exist locally but not remotely (potential deletions)
    for (const auto& LocalDirPair : LocalDirs->Values)
    {
        FString DirName = LocalDirPair.Key;
        TSharedPtr<FJsonObject> LocalDir = LocalDirPair.Value->AsObject();
        
        if (!LocalDir.IsValid()) continue;
        
        TSharedPtr<FJsonObject> LocalFiles = LocalDir->GetObjectField(TEXT("files"));
        if (!LocalFiles.IsValid()) continue;
        
        // If directory doesn't exist in remote, mark files for potential removal
        if (!RemoteDirs->HasField(DirName))
        {
            for (const auto& LocalFilePair : LocalFiles->Values)
            {
                FString FileName = LocalFilePair.Key;
                FString DifferenceEntry = FString::Printf(TEXT("REMOVE|%s/%s|DELETED_REMOTE|LOCAL_ONLY"), 
                                                         *DirName, *FileName);
                DifferencesArray.Add(DifferenceEntry);
            }
        }
        else
        {
            // Directory exists in both - check for removed files
            TSharedPtr<FJsonObject> RemoteDir = RemoteDirs->GetObjectField(DirName);
            TSharedPtr<FJsonObject> RemoteFiles = RemoteDir->GetObjectField(TEXT("files"));
            
            for (const auto& LocalFilePair : LocalFiles->Values)
            {
                FString FileName = LocalFilePair.Key;
                
                if (!RemoteFiles->HasField(FileName))
                {
                    FString DifferenceEntry = FString::Printf(TEXT("REMOVE|%s/%s|DELETED_REMOTE|LOCAL_ONLY"), 
                                                             *DirName, *FileName);
                    DifferencesArray.Add(DifferenceEntry);
                }
            }
        }
    }
    
    // Save differences to RSSDifferences.json if not executing changes
    if (!bExecuteChanges)
    {
        SaveDifferencesToFile(DifferencesArray, TempFolder);
    }
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Found %d differences between manifests"), DifferencesArray.Num());
    
    return DifferencesArray;
}

/**
 * Helper Function: Save Differences to JSON File
 * PRODUCTION NOTE: Passed the test.
 * Saves the differences analysis to a structured JSON file for review.
 * This allows users to see what changes will be made before executing them.
 * 
 * @param Differences - Array of difference strings
 * @param TempFolder - Where to save the differences file
 */
void SaveDifferencesToFile(const TArray<FString>& Differences, const FString& TempFolder)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    TArray<TSharedPtr<FJsonValue>> DifferencesArray;
    
    for (const FString& Difference : Differences)
    {
        TArray<FString> Parts;
        Difference.ParseIntoArray(Parts, TEXT("|"));
        
        if (Parts.Num() >= 4)
        {
            TSharedPtr<FJsonObject> DiffObject = MakeShareable(new FJsonObject());
            DiffObject->SetStringField(TEXT("action"), Parts[0]);
            DiffObject->SetStringField(TEXT("file_path"), Parts[1]);
            DiffObject->SetStringField(TEXT("reason"), Parts[2]);
            DiffObject->SetStringField(TEXT("priority"), Parts[3]);
            
            DifferencesArray.Add(MakeShareable(new FJsonValueObject(DiffObject)));
        }
    }
    
    RootObject->SetArrayField(TEXT("differences"), DifferencesArray);
    RootObject->SetStringField(TEXT("analysis_date"), FDateTime::Now().ToIso8601());
    RootObject->SetNumberField(TEXT("total_differences"), Differences.Num());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    
    if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
    {
        FString DifferencesPath = TempFolder + TEXT("RSSDifferences.json");
        FFileHelper::SaveStringToFile(OutputString, *DifferencesPath);
        
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Saved differences analysis to %s"), *DifferencesPath);
    }
}

/**
 * Step 7: Apply File Changes Based on Differences
 * PRODUCTION NOTE: Passed the test - reuired paths adjustments.
 * Reads the RSSDifferences.json file and performs the actual file operations
 * (add, update, remove) for both local and remote repositories.
 * 
 * BEGINNER NOTE: This is where the actual file synchronization happens.
 * Always backup your important files before running this function!
 * 
 * @param TempFolder - Path to temp folder containing difference analysis
 * @param bUpdateLocal - If true, update local files. If false, update remote files.
 * @return bool - Success status of the operation
 */
bool ApplyFileChanges(const FString& TempFolder, bool bUpdateLocal = true)
{
    FString DifferencesPath = TempFolder + TEXT("RSSDifferences.json");
    
    if (!FPaths::FileExists(DifferencesPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: RSSDifferences.json not found"));
        return false;
    }
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *DifferencesPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to load differences file"));
        return false;
    }
    
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to parse differences JSON"));
        return false;
    }
    
    TArray<TSharedPtr<FJsonValue>> DifferencesArray = RootObject->GetArrayField(TEXT("differences"));
    
    int32 SuccessCount = 0;
    int32 FailureCount = 0;
    
    FString SourcePath = bUpdateLocal ? TempFolder : FPaths::ProjectDir();
    FString DestinationPath = bUpdateLocal ? FPaths::ProjectDir() : TempFolder;
    
    // Create destination directory if updating remote
    if (!bUpdateLocal)
    {
        IFileManager::Get().MakeDirectory(*DestinationPath, true);
    }
    
    for (const TSharedPtr<FJsonValue>& DiffValue : DifferencesArray)
    {
        TSharedPtr<FJsonObject> DiffObject = DiffValue->AsObject();
        if (!DiffObject.IsValid()) continue;
        
        FString Action = DiffObject->GetStringField(TEXT("action"));
        FString FilePath = DiffObject->GetStringField(TEXT("file_path"));
        
        if (Action == TEXT("ADD") || Action == TEXT("UPDATE"))
        {
            // Copy file from source to destination
            FString SourceFilePath = SourcePath + FilePath;
            FString DestFilePath = DestinationPath + FilePath;
            
            // Ensure destination directory exists
            FString DestDir = FPaths::GetPath(DestFilePath);
            IFileManager::Get().MakeDirectory(*DestDir, true);
            
            if (IFileManager::Get().Copy(*DestFilePath, *SourceFilePath) == COPY_OK)
            {
                SuccessCount++;
                UE_LOG(LogTemp, Log, TEXT("RSSSync: %s %s"), *Action, *FilePath);
            }
            else
            {
                FailureCount++;
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to %s %s"), *Action.ToLower(), *FilePath);
                UE_LOG(LogTemp, Warning, TEXT("RSSSync: Source file may not exist: %s"), *SourceFilePath);
            }
        }
        else if (Action == TEXT("REMOVE"))
        {
            // Remove file from destination
            FString FileToRemove = DestinationPath + FilePath;
            
            if (IFileManager::Get().Delete(*FileToRemove))
            {
                SuccessCount++;
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Removed %s"), *FilePath);
            }
            else
            {
                FailureCount++;
                UE_LOG(LogTemp, Warning, TEXT("RSSSync: Could not remove %s (may not exist)"), *FilePath);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Applied changes - %d succeeded, %d failed"), 
           SuccessCount, FailureCount);
    
    return FailureCount == 0;
}

/**
 * Step 8: Update Both Manifests After Changes
 * PRODUCTION NOTE: Passed the test - paths adjustments for CreateManifest() are required. 
 * Regenerates both local and remote manifests to reflect the current state
 * after synchronization. This ensures the manifests stay in sync with actual files.
 * 
 * @param TempFolder - Path to temp folder
 * @return bool - Success status of manifest updates
 */
bool UpdateManifestsAfterChanges(const FString& TempFolder)
{
    bool bLocalSuccess = false;
    bool bRemoteSuccess = false;
    
    // Update local manifest
    try
    {
        // Remove old local manifest
        FString LocalManifestPath = FPaths::ProjectDir() + TEXT("RSSManifest.json");
        if (FPaths::FileExists(LocalManifestPath))
        {
            IFileManager::Get().Delete(*LocalManifestPath);
        }
        
        // Create new local manifest using existing utility
        CreateManifest();
        
        bLocalSuccess = FPaths::FileExists(LocalManifestPath);
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Local manifest update %s"), 
               bLocalSuccess ? TEXT("succeeded") : TEXT("failed"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Exception updating local manifest"));
    }
    
    // Update remote manifest if we have updated files
    FString UpdatedPath = TempFolder;
    if (IFileManager::Get().DirectoryExists(*UpdatedPath))
    {
        try
        {
            // Remove old remote manifest
            FString RemoteManifestPath = TempFolder + TEXT("RSSManifest.json");
            if (FPaths::FileExists(RemoteManifestPath))
            {
                IFileManager::Get().Delete(*RemoteManifestPath);
            }
            
            // Create new remote manifest from updated files
            bRemoteSuccess = CreateManifest();
            UE_LOG(LogTemp, Log, TEXT("RSSSync: Remote manifest update %s"), 
                   bRemoteSuccess ? TEXT("succeeded") : TEXT("failed"));
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Exception updating remote manifest"));
        }
    }
    else
    {
        bRemoteSuccess = true; // No remote changes needed
    }
    
    return bLocalSuccess && bRemoteSuccess;
}

/**
 * Step 9: Pack Updated Branch into ZIP File
 * PRODUCTION NOTE: Passed the test.
 * Creates a ZIP file from the updated branch files, ready for upload to GitHub.
 * Uses the existing ZIP creation utilities.
 * 
 * @param TempFolder - Path to temp folder containing updated files
 * @return bool - Success status of ZIP creation
 */
bool PackUpdatedBranchToZip(const FString& TempFolder)
{
    FString UpdatedPath = TempFolder;
    FString ZipOutputPath = TempFolder + TEXT("updated_branch.zip");
    
    if (!IFileManager::Get().DirectoryExists(*UpdatedPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: No updated files directory found"));
        return false;
    }
    
    try
    {
        // Collect all files for ZIP using existing utility function approach
        TArray<FString> AllFiles;
        IFileManager::Get().FindFilesRecursive(AllFiles, *UpdatedPath, TEXT("*"), true, false);
        
        if (AllFiles.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("RSSSync: No files to pack"));
            return false;
        }
        
        // Create structured file list for ZIP
        TArray<TPair<FString, FString>> FilesToZip;
        for (const FString& FilePath : AllFiles)
        {
            FString RelativePath = FilePath;
            FPaths::MakePathRelativeTo(RelativePath, *UpdatedPath);
            FilesToZip.Add(TPair<FString, FString>(FilePath, RelativePath));
        }
        
        // Create ZIP using existing function
        CreateZip_Structured(FilesToZip, ZipOutputPath);
        
        if (FPaths::FileExists(ZipOutputPath))
        {
            UE_LOG(LogTemp, Log, TEXT("RSSSync: Successfully created ZIP with %d files"), AllFiles.Num());
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: ZIP file was not created"));
            return false;
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Exception during ZIP creation"));
        return false;
    }
}

/**
 * Step 10: Upload Updated Branch to GitHub
 * 
 * Uploads the updated ZIP file back to GitHub using the GitHub API.
 * This involves creating a new commit with the updated files.
 * 
 * BEGINNER NOTE: This is the most complex part as it requires understanding
 * of Git concepts like commits, trees, and blobs. GitHub API handles the Git
 * operations for us, but we need to follow their specific workflow.
 * @param RepoOwner - GitHub username/organization
 * @param RepoName - Repository name
 * @param BranchName - Target branch for upload
 * @param AccessToken - GitHub Personal Access Token (required for push operations)
 * @param TempFolder - Path to folder containing extracted files ready for upload
 * @param CommitMessage - Commit message for the update
 * @param OnUploadComplete - Callback function when upload finishes
 */
void UploadUpdatedBranchToGitHub(const FString& RepoOwner, const FString& RepoName,
                                const FString& BranchName, const FString& AccessToken,
                                const FString& TempFolder, const FString& CommitMessage,
                                TFunction<void(bool)> OnUploadComplete)
{
    if (AccessToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Access token required for GitHub upload"));
        OnUploadComplete(false);
        return;
    }
    
    // NOTE: No longer checking for ZIP file - expecting extracted files in TempFolder
    // This allows you to test extraction separately from upload
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Using extracted folder path: %s"), *TempFolder);
    
    // Check if the folder exists
    if (!IFileManager::Get().DirectoryExists(*TempFolder))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Extracted folder not found: %s"), *TempFolder);
        OnUploadComplete(false);
        return;
    }
    
    // GitHub API workflow for updating files:
    // 1. Get the current branch reference
    // 2. Get the commit tree for that reference
    // 3. Create new blobs for changed files
    // 4. Create a new tree with updated blobs
    // 5. Create a new commit pointing to the new tree
    // 6. Update the branch reference to point to the new commit
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Starting GitHub upload process..."));
      // Forward declaring the helper function to get branch reference
    void GetBranchReference(const FString& RepoOwner, const FString& RepoName,
                       const FString& BranchName, const FString& AccessToken,
                       TFunction<void(bool, FString)> OnComplete);
      // Forward declaring the helper function to upload files and create commit
    void UploadFilesAndCreateCommit(const FString& RepoOwner, const FString& RepoName,
                                   const FString& BranchName, const FString& AccessToken,
                                   const FString& ExtractedFolderPath, const FString& CommitMessage,                                   const FString& ParentSHA, TFunction<void(bool)> OnComplete);
                                   
    // Forward declaring helper functions for commit creation
    void CreateSimpleTestCommit(const FString& RepoOwner, const FString& RepoName,
                               const FString& BranchName, const FString& AccessToken,
                               const FString& CommitMessage, const FString& ParentSHA,
                               TFunction<void(bool)> OnComplete);

    // Step 1: Get current branch reference
    GetBranchReference(RepoOwner, RepoName, BranchName, AccessToken,
        [=](bool bSuccess, const FString& CurrentSHA)
        {
            if (!bSuccess)
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to get branch reference"));
                OnUploadComplete(false);
                return;
            }
              // Step 2: Upload files and create commit
            // NOTE: Passing TempFolder as ExtractedFolderPath - assumes files are already extracted
            UploadFilesAndCreateCommit(RepoOwner, RepoName, BranchName, AccessToken,
                                     TempFolder, CommitMessage, CurrentSHA, OnUploadComplete);
        });
}

/**
 * Helper Function: Get Branch Reference from GitHub
 * 
 * Gets the current SHA hash of the branch head, which we need to create a new commit.
 */
void GetBranchReference(const FString& RepoOwner, const FString& RepoName,
                       const FString& BranchName, const FString& AccessToken,
                       TFunction<void(bool, FString)> OnComplete)
{
    FString URL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/ref/heads/%s"),
                                 *RepoOwner, *RepoName, *BranchName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    
    Request->OnProcessRequestComplete().BindLambda([OnComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            
            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {                TSharedPtr<FJsonObject> ObjectField = JsonObject->GetObjectField(TEXT("object"));
                if (ObjectField.IsValid())
                {
                    FString SHA = ObjectField->GetStringField(TEXT("sha"));
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: Got branch SHA: %s"), *SHA);
                    OnComplete(true, SHA);
                    return;
                }
            }
        }
        
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to get branch reference"));
        OnComplete(false, FString());
    });

    Request->ProcessRequest();
}

/**
 * Helper Function: Upload Files and Create Commit
 * 
 * This is a simplified version - in reality, you'd need to: * 1. Read each file from the ZIP
 * 2. Create GitHub blobs for each file
 * 3. Create a new tree object with all blobs
 * 4. Create a commit object pointing to the tree
 * 5. Update the branch reference
 * 
 * For brevity, this shows the general structure.
 */

// Forward declaration of helper function to create a simple test commit
void CreateSimpleTestCommit(const FString& RepoOwner, const FString& RepoName,
                           const FString& BranchName, const FString& AccessToken,
                           const FString& CommitMessage, const FString& ParentSHA,
                           TFunction<void(bool)> OnComplete);

void UploadFilesAndCreateCommit(const FString& RepoOwner, const FString& RepoName,
                               const FString& BranchName, const FString& AccessToken,
                               const FString& ExtractedFolderPath, const FString& CommitMessage,
                               const FString& ParentSHA, TFunction<void(bool)> OnComplete)
{
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Starting actual file upload implementation"));
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: ExtractedFolderPath: %s"), *ExtractedFolderPath);
    
    // Step 1: Read files from extracted folder
    TArray<FString> FilePaths;
    TArray<FString> RelativePaths;
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Step 1 - Reading files from extracted folder"));
    
    IFileManager& FileManager = IFileManager::Get();
    
    // Get all files recursively from the extracted folder
    FileManager.FindFilesRecursive(FilePaths, *ExtractedFolderPath, TEXT("*"), true, false);
    
    if (FilePaths.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: No files found in extracted folder"));
        OnComplete(false);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Found %d files to upload"), FilePaths.Num());
    
    // Create relative paths (remove the ExtractedFolderPath prefix)
    for (const FString& FilePath : FilePaths)
    {
        FString RelativePath = FilePath;
        RelativePath.RemoveFromStart(ExtractedFolderPath);
        // Normalize path separators for GitHub (use forward slashes)
        RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));
        RelativePaths.Add(RelativePath);
        
        UE_LOG(LogTemp, Log, TEXT("RSSSync: File: %s -> %s"), *FilePath, *RelativePath);
    }
      // Step 2: Create blob objects for each file
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Step 2 - Creating blob objects"));
    CreateBlobsForFiles(RepoOwner, RepoName, BranchName, AccessToken, FilePaths, RelativePaths, CommitMessage, ParentSHA, OnComplete);
}

/**
 * Step 2: Create Blob Objects for Files
 * 
 * Creates GitHub blob objects for each file that needs to be uploaded.
 * Blobs are the Git objects that store file content.
 */
void CreateBlobsForFiles(const FString& RepoOwner, const FString& RepoName, const FString& BranchName, 
                        const FString& AccessToken, const TArray<FString>& FilePaths, const TArray<FString>& RelativePaths,
                        const FString& CommitMessage, const FString& ParentSHA,
                        TFunction<void(bool)> OnComplete)
{
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Creating blobs for %d files"), FilePaths.Num());

    TSharedPtr<FBlobCreationState> State = MakeShared<FBlobCreationState>();
    State->TotalBlobs = FilePaths.Num();
    State->BlobInfos.Reserve(FilePaths.Num());
    
    // Initialize blob infos with paths
    for (int32 i = 0; i < FilePaths.Num(); i++)
    {
        FBlobInfo BlobInfo;
        BlobInfo.Path = RelativePaths[i];
        BlobInfo.Mode = TEXT("100644"); // Regular file mode
        State->BlobInfos.Add(BlobInfo);
    }
    
    // Create blobs for each file
    for (int32 i = 0; i < FilePaths.Num(); i++)
    {        CreateSingleBlob(RepoOwner, RepoName, AccessToken, FilePaths[i], i, State, 
            [RepoOwner, RepoName, BranchName, AccessToken, CommitMessage, ParentSHA, OnComplete](TSharedPtr<FBlobCreationState> FinalState)
            {
                if (FinalState->bHasError)
                {
                    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to create one or more blobs"));
                    OnComplete(false);
                    return;
                }
                
                UE_LOG(LogTemp, Log, TEXT("RSSSync: All blobs created successfully"));                // Step 3: Create tree object with all blobs
                CreateTreeWithBlobs(RepoOwner, RepoName, BranchName, AccessToken, FinalState->BlobInfos, CommitMessage, ParentSHA, OnComplete);
            });
    }
}

/**
 * Helper: Create a Single Blob Object
 */
void CreateSingleBlob(const FString& RepoOwner, const FString& RepoName, const FString& AccessToken,
                     const FString& FilePath, int32 BlobIndex, TSharedPtr<struct FBlobCreationState> State,
                     TFunction<void(TSharedPtr<struct FBlobCreationState>)> OnAllBlobsComplete)
{
    // Read file content
    TArray<uint8> FileContent;
    if (!FFileHelper::LoadFileToArray(FileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to read file: %s"), *FilePath);
        State->bHasError = true;
        OnAllBlobsComplete(State);
        return;
    }
    
    // Convert to base64 for GitHub API
    FString Base64Content = FBase64::Encode(FileContent);
    
    // Create blob via GitHub API
    FString BlobURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/blobs"), *RepoOwner, *RepoName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(BlobURL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Create JSON payload
    TSharedPtr<FJsonObject> BlobObject = MakeShareable(new FJsonObject());
    BlobObject->SetStringField(TEXT("content"), Base64Content);
    BlobObject->SetStringField(TEXT("encoding"), TEXT("base64"));
    
    FString BlobJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BlobJson);
    FJsonSerializer::Serialize(BlobObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(BlobJson);
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Creating blob for file: %s (%d bytes)"), *FilePath, FileContent.Num());
    
    Request->OnProcessRequestComplete().BindLambda([State, BlobIndex, OnAllBlobsComplete, FilePath]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode == 201) // Created
            {
                // Parse response to get blob SHA
                FString ResponseString = Response->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
                
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString BlobSHA = JsonObject->GetStringField(TEXT("sha"));
                    State->BlobInfos[BlobIndex].SHA = BlobSHA;
                    
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: Blob created for %s - SHA: %s"), *FilePath, *BlobSHA);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to parse blob creation response"));
                    State->bHasError = true;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Blob creation failed with code %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                State->bHasError = true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network error during blob creation"));
            State->bHasError = true;
        }
        
        // Check if all blobs are complete
        State->CompletedBlobs++;
        if (State->CompletedBlobs >= State->TotalBlobs)
        {
            OnAllBlobsComplete(State);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to start blob creation request"));
        State->bHasError = true;
        OnAllBlobsComplete(State);
    }
}

/**
 * Step 3: Create Tree Object with All Blobs
 * 
 * Creates a GitHub tree object that references all the blob objects.
 * This is equivalent to a Git tree that defines the file structure.
 */
void CreateTreeWithBlobs(const FString& RepoOwner, const FString& RepoName, const FString& BranchName,
                        const FString& AccessToken, const TArray<FBlobInfo>& BlobInfos, 
                        const FString& CommitMessage, const FString& ParentSHA, TFunction<void(bool)> OnComplete)
{
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Step 3 - Creating tree object with %d blobs"), BlobInfos.Num());
    
    FString TreeURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/trees"), *RepoOwner, *RepoName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TreeURL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Create JSON payload
    TSharedPtr<FJsonObject> TreeObject = MakeShareable(new FJsonObject());
    TArray<TSharedPtr<FJsonValue>> TreeArray;
    
    for (const FBlobInfo& BlobInfo : BlobInfos)
    {
        TSharedPtr<FJsonObject> TreeEntry = MakeShareable(new FJsonObject());
        TreeEntry->SetStringField(TEXT("path"), BlobInfo.Path);
        TreeEntry->SetStringField(TEXT("mode"), BlobInfo.Mode);
        TreeEntry->SetStringField(TEXT("type"), TEXT("blob"));
        TreeEntry->SetStringField(TEXT("sha"), BlobInfo.SHA);
        
        TreeArray.Add(MakeShareable(new FJsonValueObject(TreeEntry)));
    }
    
    TreeObject->SetArrayField(TEXT("tree"), TreeArray);
    
    FString TreeJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TreeJson);
    FJsonSerializer::Serialize(TreeObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(TreeJson);
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Creating tree with %d entries"), TreeArray.Num());
    
    Request->OnProcessRequestComplete().BindLambda([RepoOwner, RepoName, BranchName, AccessToken, CommitMessage, ParentSHA, OnComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode == 201) // Created
            {
                // Parse response to get tree SHA
                FString ResponseString = Response->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
                
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {                    FString TreeSHA = JsonObject->GetStringField(TEXT("sha"));
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: Tree created successfully - SHA: %s"), *TreeSHA);
                    
                    // Step 4: Create commit with the new tree
                    CreateCommitWithTreeSHA(RepoOwner, RepoName, BranchName, AccessToken, CommitMessage, TreeSHA, ParentSHA, OnComplete);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to parse tree creation response"));
                    OnComplete(false);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Tree creation failed with code %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                OnComplete(false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network error during tree creation"));
            OnComplete(false);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to start tree creation request"));
        OnComplete(false);
    }
}

/**
 * Step 4: Create Commit with Tree SHA
 * 
 * Creates a GitHub commit object that points to the new tree.
 */
void CreateCommitWithTreeSHA(const FString& RepoOwner, const FString& RepoName, const FString& BranchName,
                            const FString& AccessToken, const FString& CommitMessage, const FString& TreeSHA, 
                            const FString& ParentSHA, TFunction<void(bool)> OnComplete)
{
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Step 4 - Creating commit with tree SHA: %s"), *TreeSHA);
    
    FString CommitURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/commits"), *RepoOwner, *RepoName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(CommitURL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Create JSON payload
    TSharedPtr<FJsonObject> CommitObject = MakeShareable(new FJsonObject());
    CommitObject->SetStringField(TEXT("message"), CommitMessage);
    CommitObject->SetStringField(TEXT("tree"), TreeSHA);
    
    // Add parent commit
    TArray<TSharedPtr<FJsonValue>> ParentsArray;
    ParentsArray.Add(MakeShareable(new FJsonValueString(ParentSHA)));
    CommitObject->SetArrayField(TEXT("parents"), ParentsArray);
    
    FString CommitJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CommitJson);
    FJsonSerializer::Serialize(CommitObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(CommitJson);
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Creating commit with message: %s"), *CommitMessage);
    
    Request->OnProcessRequestComplete().BindLambda([RepoOwner, RepoName, BranchName, AccessToken, OnComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode == 201) // Created
            {
                // Parse response to get commit SHA
                FString ResponseString = Response->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
                
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString CommitSHA = JsonObject->GetStringField(TEXT("sha"));
                    UE_LOG(LogTemp, Log, TEXT("RSSSync: Commit created successfully - SHA: %s"), *CommitSHA);                    // Step 5: Update branch reference
                    UpdateBranchReference(RepoOwner, RepoName, BranchName, AccessToken, CommitSHA, OnComplete);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to parse commit creation response"));
                    OnComplete(false);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Commit creation failed with code %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                OnComplete(false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network error during commit creation"));
            OnComplete(false);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to start commit creation request"));
        OnComplete(false);
    }
}

/**
 * Step 5: Update Branch Reference
 * 
 * Updates the branch reference to point to the new commit, completing the upload.
 */
void UpdateBranchReference(const FString& RepoOwner, const FString& RepoName, const FString& BranchName,
                          const FString& AccessToken, const FString& CommitSHA, TFunction<void(bool)> OnComplete)
{
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Step 5 - Updating branch reference to commit: %s"), *CommitSHA);
    
    FString RefURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/refs/heads/%s"), *RepoOwner, *RepoName, *BranchName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(RefURL);
    Request->SetVerb(TEXT("PATCH"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Create JSON payload
    TSharedPtr<FJsonObject> RefObject = MakeShareable(new FJsonObject());
    RefObject->SetStringField(TEXT("sha"), CommitSHA);
    RefObject->SetBoolField(TEXT("force"), false); // Don't force push
    
    FString RefJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RefJson);
    FJsonSerializer::Serialize(RefObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(RefJson);
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Updating branch reference"));
    
    Request->OnProcessRequestComplete().BindLambda([OnComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode == 200) // OK
            {
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Branch reference updated successfully!"));
                UE_LOG(LogTemp, Warning, TEXT("=== UPLOAD COMPLETE: All files uploaded to GitHub successfully ==="));
                OnComplete(true);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Branch reference update failed with code %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                OnComplete(false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network error during branch reference update"));
            OnComplete(false);
        }
    });
      if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to start branch reference update request"));
        OnComplete(false);
    }
}

/**
 * Enhanced Upload Function with Write Permission Validation
 * 
 * This function first validates that the token has WRITE permissions, then proceeds with upload.
 * Essential for private repositories and any repository that requires push access.
 *  * @param RepoOwner - GitHub username/organization
 * @param RepoName - Repository name
 * @param BranchName - Branch to upload to
 * @param AccessToken - GitHub Personal Access Token with write permissions
 * @param ExtractedFolderPath - Path containing the extracted files ready for upload
 * @param CommitMessage - Commit message for the update
 * @param OnComplete - Callback with success status and error message
 */
void UploadUpdatedBranchWithValidation(const FString& RepoOwner, const FString& RepoName,
                                      const FString& BranchName, const FString& AccessToken,
                                      const FString& TempFolder, const FString& CommitMessage,
                                      TFunction<void(bool, FString)> OnComplete)
{
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Starting WRITE validation for %s/%s (Upload requires push permissions)"), *RepoOwner, *RepoName);
    
    if (AccessToken.IsEmpty())
    {
        FString Error = TEXT("Access token is required for GitHub upload operations");
        UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
        OnComplete(false, Error);
        return;
    }
    
    // First validate write permissions
    ValidateGitHubTokenAccess(RepoOwner, RepoName, AccessToken, 
        [RepoOwner, RepoName, BranchName, AccessToken, TempFolder, CommitMessage, OnComplete]
        (bool bHasWriteAccess, FString ErrorMessage)
        {
            if (bHasWriteAccess)
            {
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Token validated for WRITE access, proceeding with upload"));
                
                // Proceed with upload using the original function
                UploadUpdatedBranchToGitHub(RepoOwner, RepoName, BranchName, AccessToken, TempFolder, CommitMessage,
                    [OnComplete](bool bUploadSuccess)
                    {
                        if (bUploadSuccess)
                        {
                            OnComplete(true, TEXT("Upload completed successfully"));
                        }
                        else
                        {
                            OnComplete(false, TEXT("Upload operation failed"));
                        }
                    });
            }
            else
            {
                FString FullError = FString::Printf(TEXT("Write permission validation failed: %s"), *ErrorMessage);
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *FullError);
                
                // Provide specific guidance for common permission issues
                if (ErrorMessage.Contains(TEXT("read access but lacks push")))
                {
                    FullError += TEXT("\n\nSOLUTION: Your token needs 'repo' scope for private repos or 'public_repo' scope for public repos.");
                    FullError += TEXT("\nGenerate a new token at: GitHub Settings > Developer settings > Personal access tokens");
                }
                
                OnComplete(false, FullError);
            }
        });
}

/**
 * Test function for complete upload workflow
 * Tests the full 5-step upload process to GitHub
 */
void TestCompleteUploadWorkflow()
{
    UE_LOG(LogTemp, Warning, TEXT("RSSSync: Starting TestCompleteUploadWorkflow"));
    
    // Test parameters
    FString RepoOwner = TEXT("your-username");
    FString RepoName = TEXT("your-repo");
    FString BranchName = TEXT("main");
    FString AccessToken = TEXT("your-token");
    FString ExtractedFolderPath = TEXT("D:/TestFolder");
    FString CommitMessage = TEXT("Test commit from Unreal Engine");
    
    // Create completion callback
    TFunction<void(bool)> OnComplete = [](bool bSuccess)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("RSSSync: TestCompleteUploadWorkflow - SUCCESS"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: TestCompleteUploadWorkflow - FAILED"));
        }
    };
      // Start the upload workflow
    GetBranchReference(RepoOwner, RepoName, BranchName, AccessToken,
        [=](bool bSuccess, FString ParentSHA)
        {
            if (bSuccess)
            {
                UploadFilesAndCreateCommit(RepoOwner, RepoName, BranchName, AccessToken, 
                                          ExtractedFolderPath, CommitMessage, ParentSHA, OnComplete);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: Failed to get branch reference for test"));
                OnComplete(false);
            }
        });
}

/**
 * Validate GitHub Token Access
 * 
 * Tests if the provided token has access to the specified repository.
 * This function makes a simple API call to test token validity and permissions.
 */
void ValidateGitHubTokenAccess(const FString& RepoOwner, const FString& RepoName, 
                               const FString& AccessToken, TFunction<void(bool, FString)> OnValidationComplete)
{
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Validating GitHub token access for %s/%s"), *RepoOwner, *RepoName);
    
    if (AccessToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("RSSSync: No access token provided - will attempt public access"));
        OnValidationComplete(true, TEXT("No token provided - public access"));
        return;
    }
    
    // Test token by making a simple API call to get repository information
    // This validates both token validity and repository access
    FString ValidationURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s"), *RepoOwner, *RepoName);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ValidationURL);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetTimeout(15.0f);
    
    Request->OnProcessRequestComplete().BindLambda([OnValidationComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            
            if (ResponseCode == 200)
            {
                // Success - token has read access to repository
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Token validation successful - has repository access"));
                OnValidationComplete(true, TEXT("Token validated successfully"));
            }
            else if (ResponseCode == 401)
            {
                // Unauthorized - invalid token
                FString Error = TEXT("Invalid or expired access token");
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
                OnValidationComplete(false, Error);
            }
            else if (ResponseCode == 403)
            {
                // Forbidden - token lacks permissions
                FString Error = TEXT("Access token lacks required permissions for this repository");
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
                OnValidationComplete(false, Error);
            }
            else if (ResponseCode == 404)
            {
                // Not found - repository doesn't exist or token lacks access
                FString Error = TEXT("Repository not found or access denied");
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
                OnValidationComplete(false, Error);
            }
            else
            {
                // Other error
                FString Error = FString::Printf(TEXT("Token validation failed with HTTP %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                OnValidationComplete(false, Error);
            }
        }
        else
        {
            FString Error = TEXT("Network error during token validation");
            UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
            OnValidationComplete(false, Error);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        FString Error = TEXT("Failed to start token validation request");
        UE_LOG(LogTemp, Error, TEXT("RSSSync: %s"), *Error);
        OnValidationComplete(false, Error);
    }
}
