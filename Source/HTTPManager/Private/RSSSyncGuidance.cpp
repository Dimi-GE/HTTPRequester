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

/**
 * Step 1: Create Temporary Folder for RSS Synchronization
 * NOTE: Passed the test.
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
 * 
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
                             const FString& BranchName, const FString& AccessToken,
                             const FString& TempFolder, TFunction<void(bool)> OnDownloadComplete)
{
    // Construct GitHub API URL for branch archive download
    // Format: https://api.github.com/repos/{owner}/{repo}/zipball/{branch}
    FString DownloadURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/zipball/%s"), 
                                         *RepoOwner, *RepoName, *BranchName);
    
    UE_LOG(LogTemp, Log, TEXT("RSSSync: Downloading from %s"), *DownloadURL);
    
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    
    // Set request parameters
    Request->SetURL(DownloadURL);
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
    
    // Bind response handler
    Request->OnProcessRequestComplete().BindLambda([TempFolder, OnDownloadComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
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
    Request->ProcessRequest();
}

/**
 * Step 3: Unpack Downloaded Branch and Clean Up
 * 
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
 * Checks if LocalRSSManifest.json exists in the RSS folder. If not, creates it
 * using the existing RSSManifestInit_UTIL function.
 * 
 * PRODUCTION NOTE: DEPRECATED - Use CreateManifest instead.
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
 * 
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
 * 
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
 * 
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
    
    FString SourcePath = bUpdateLocal ? TempFolder + TEXT("extracted/") : FPaths::ProjectDir();
    FString DestinationPath = bUpdateLocal ? FPaths::ProjectDir() : TempFolder + TEXT("updated/");
    
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
 * 
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
        FString LocalManifestPath = FPaths::ProjectDir() + TEXT("RSS/LocalRSSManifest.json");
        if (FPaths::FileExists(LocalManifestPath))
        {
            IFileManager::Get().Delete(*LocalManifestPath);
        }
        
        // Create new local manifest using existing utility
        RSSManifestInit_UTIL();
        
        bLocalSuccess = FPaths::FileExists(LocalManifestPath);
        UE_LOG(LogTemp, Log, TEXT("RSSSync: Local manifest update %s"), 
               bLocalSuccess ? TEXT("succeeded") : TEXT("failed"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Exception updating local manifest"));
    }
    
    // Update remote manifest if we have updated files
    FString UpdatedPath = TempFolder + TEXT("updated/");
    if (IFileManager::Get().DirectoryExists(*UpdatedPath))
    {
        try
        {
            // Remove old remote manifest
            FString RemoteManifestPath = TempFolder + TEXT("RemoteRSSManifest.json");
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
 * 
 * Creates a ZIP file from the updated branch files, ready for upload to GitHub.
 * Uses the existing ZIP creation utilities.
 * 
 * @param TempFolder - Path to temp folder containing updated files
 * @return bool - Success status of ZIP creation
 */
bool PackUpdatedBranchToZip(const FString& TempFolder)
{
    FString UpdatedPath = TempFolder + TEXT("updated/");
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
 * 
 * @param RepoOwner - GitHub username/organization
 * @param RepoName - Repository name
 * @param BranchName - Target branch for upload
 * @param AccessToken - GitHub Personal Access Token (required for push operations)
 * @param TempFolder - Path to temp folder containing the ZIP to upload
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
    
    FString ZipPath = TempFolder + TEXT("updated_branch.zip");
    
    if (!FPaths::FileExists(ZipPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RSSSync: Updated ZIP file not found"));
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
                                   const FString& TempFolder, const FString& CommitMessage,
                                   const FString& ParentSHA, TFunction<void(bool)> OnComplete);

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
            {
                TSharedPtr<FJsonObject> ObjectField = JsonObject->GetObjectField(TEXT("object"));
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
 * This is a simplified version - in reality, you'd need to:
 * 1. Read each file from the ZIP
 * 2. Create GitHub blobs for each file
 * 3. Create a new tree with all the blobs
 * 4. Create a commit pointing to the tree
 * 5. Update the branch reference
 * 
 * For brevity, this shows the general structure.
 */
void UploadFilesAndCreateCommit(const FString& RepoOwner, const FString& RepoName,
                               const FString& BranchName, const FString& AccessToken,
                               const FString& TempFolder, const FString& CommitMessage,
                               const FString& ParentSHA, TFunction<void(bool)> OnComplete)
{
    // This is a complex multi-step process that would require:
    // - Extracting files from ZIP
    // - Creating blob objects for each file via GitHub API
    // - Creating a tree object with all blobs
    // - Creating a commit object
    // - Updating the branch reference
    
    // For demonstration, here's the structure for creating a commit:
    
    FString CommitURL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/git/commits"),
                                       *RepoOwner, *RepoName);
    
    // Create commit JSON (simplified - you'd need to build the full tree)
    TSharedPtr<FJsonObject> CommitObject = MakeShareable(new FJsonObject());
    CommitObject->SetStringField(TEXT("message"), CommitMessage);
    CommitObject->SetStringField(TEXT("tree"), TEXT("TREE_SHA_HERE")); // Would come from tree creation
    
    TArray<TSharedPtr<FJsonValue>> ParentsArray;
    ParentsArray.Add(MakeShareable(new FJsonValueString(ParentSHA)));
    CommitObject->SetArrayField(TEXT("parents"), ParentsArray);
    
    FString CommitJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CommitJson);
    FJsonSerializer::Serialize(CommitObject.ToSharedRef(), Writer);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(CommitURL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *AccessToken));
    Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine-RSSSync/1.0"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(CommitJson);
    
    Request->OnProcessRequestComplete().BindLambda([OnComplete]
        (FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode == 200 || ResponseCode == 201)
            {
                UE_LOG(LogTemp, Log, TEXT("RSSSync: Successfully uploaded to GitHub"));
                OnComplete(true);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RSSSync: GitHub upload failed with code %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Response: %s"), *Response->GetContentAsString());
                OnComplete(false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RSSSync: Network error during upload"));
            OnComplete(false);
        }
    });
    
    Request->ProcessRequest();
}

/**
 * MAIN ORCHESTRATION FUNCTION
 * 
 * This function coordinates the entire RSS synchronization workflow.
 * Call this function to perform a complete sync operation.
 * 
 * BEGINNER GUIDANCE:
 * 1. Start by testing with a small, non-critical repository
 * 2. Always backup your important files before running
 * 3. Monitor the logs to understand what's happening
 * 4. Test the analyze-only mode first (bExecuteChanges = false)
 * 
 * @param RepoOwner - Your GitHub username or organization
 * @param RepoName - The repository name
 * @param BranchName - Branch to sync (usually "main" or "master")
 * @param AccessToken - Your GitHub Personal Access Token
 * @param bExecuteChanges - If false, only analyze differences
 * @param CommitMessage - Message for the commit when uploading changes
 */
void PerformRSSSync(const FString& RepoOwner, const FString& RepoName,
                   const FString& BranchName, const FString& AccessToken,
                   bool bExecuteChanges = false, const FString& CommitMessage = TEXT("RSS Sync Update"))
{
    UE_LOG(LogTemp, Log, TEXT("=== RSS SYNC STARTED ==="));
    UE_LOG(LogTemp, Log, TEXT("Repository: %s/%s"), *RepoOwner, *RepoName);
    UE_LOG(LogTemp, Log, TEXT("Branch: %s"), *BranchName);
    UE_LOG(LogTemp, Log, TEXT("Execute Changes: %s"), bExecuteChanges ? TEXT("Yes") : TEXT("No"));
    
    // Step 1: Create temp folder
    FString TempFolder = CreateRSSSyncTempFolder();
    if (TempFolder.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 1: Temp folder creation"));
        return;
    }
    
    // Step 2: Download branch
    DownloadBranchFromGitHub(RepoOwner, RepoName, BranchName, AccessToken, TempFolder,
        [=](bool bDownloadSuccess)
        {
            if (!bDownloadSuccess)
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 2: Branch download"));
                return;
            }
            
            // Step 3: Unpack downloaded branch
            if (!UnpackDownloadedBranch(TempFolder))
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 3: Branch unpacking"));
                return;
            }
            
            // Step 4: Create remote manifest
            if (!CreateManifest())
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 4: Remote manifest creation"));
                return;
            }
            
            // Step 5: Ensure local manifest exists
            if (!EnsureLocalManifestExists())
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 5: Local manifest check"));
                return;
            }
            
            // Step 6: Compare manifests
            TArray<FString> Differences = CompareManifestsAndFindDifferences(bExecuteChanges, TempFolder);
            
            if (Differences.Num() == 0)
            {
                UE_LOG(LogTemp, Log, TEXT("RSS Sync complete: No differences found"));
                return;
            }
            
            UE_LOG(LogTemp, Log, TEXT("Found %d differences"), Differences.Num());
            
            if (!bExecuteChanges)
            {
                UE_LOG(LogTemp, Log, TEXT("Analysis complete. Check RSSDifferences.json for details."));
                UE_LOG(LogTemp, Log, TEXT("=== RSS SYNC ANALYSIS COMPLETE ==="));
                return;
            }
            
            // Steps 7-10: Execute changes if requested
            if (!ApplyFileChanges(TempFolder, true))
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 7: Applying changes"));
                return;
            }
            
            if (!UpdateManifestsAfterChanges(TempFolder))
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 8: Updating manifests"));
                return;
            }
            
            if (!PackUpdatedBranchToZip(TempFolder))
            {
                UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 9: Packing ZIP"));
                return;
            }
            
            // Step 10: Upload to GitHub
            UploadUpdatedBranchToGitHub(RepoOwner, RepoName, BranchName, AccessToken,
                                       TempFolder, CommitMessage,
                [](bool bUploadSuccess)
                {
                    if (bUploadSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("=== RSS SYNC COMPLETE: SUCCESS ==="));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("RSS Sync failed at Step 10: GitHub upload"));
                    }
                });
        });
}

/**
 * EXAMPLE USAGE FUNCTION
 * 
 * This shows how to call the RSS sync system from your code.
 * Modify the parameters to match your GitHub repository.
 */
void ExampleUsage()
{
    // Example 1: Analyze differences only (safe to test)
    PerformRSSSync(
        TEXT("YourGitHubUsername"),      // Repository owner
        TEXT("YourRepositoryName"),      // Repository name  
        TEXT("main"),                    // Branch name
        TEXT("ghp_your_access_token"),   // Your GitHub token
        false,                           // Just analyze, don't execute
        TEXT("RSS Sync Analysis")        // Commit message (not used in analysis mode)
    );
    
    // Example 2: Full synchronization (executes changes)
    // ONLY run this after testing with analysis mode!
    /*
    PerformRSSSync(
        TEXT("YourGitHubUsername"),
        TEXT("YourRepositoryName"),
        TEXT("main"),
        TEXT("ghp_your_access_token"),
        true,                            // Execute changes
        TEXT("Synchronized via RSS Sync System")
    );
    */
}

/**
 * TROUBLESHOOTING GUIDE
 * 
 * Common Issues and Solutions:
 * 
 * 1. "Authentication failed - check access token"
 *    - Verify your GitHub Personal Access Token is correct
 *    - Ensure the token has 'repo' scope permissions
 *    - Check if the token has expired
 * 
 * 2. "Repository or branch not found"
 *    - Verify the repository owner and name are correct
 *    - Check that the branch exists
 *    - Ensure you have access to the repository
 * 
 * 3. "Failed to create temp folder"
 *    - Check if you have write permissions in the project directory
 *    - Ensure no other process is using the temp folder
 * 
 * 4. "ZIP file was not created"
 *    - Verify the minizip-ng library is properly linked
 *    - Check if there are any file permission issues
 * 
 * 5. "Manifest creation failed"
 *    - Ensure the RSSManifestInit_UTIL function is working
 *    - Check if the file hash calculation is functioning
 * 
 * PERFORMANCE TIPS:
 * - Test with small repositories first
 * - Use analysis mode before executing changes
 * - Monitor GitHub API rate limits
 * - Consider using webhooks for automatic syncing
 * 
 * SECURITY NOTES:
 * - Never commit your access token to version control
 * - Use environment variables or secure storage for tokens
 * - Regularly rotate your access tokens
 * - Use the minimum required permissions for tokens
 */
