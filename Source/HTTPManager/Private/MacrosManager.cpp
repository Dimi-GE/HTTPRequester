// Fill out your copyright notice in the Description page of Project Settings.


#include "MacrosManager.h"

#include "HAL/PlatformFilemanager.h"

void UMacrosManager::GetFilesByCategory(bool &bIsSucceed, FString &MacroContent, FString MacroCategoryFolder)
{
    // Declare defaults
    FString Directory = FPaths::ProjectDir() + TEXT("Macros/") + MacroCategoryFolder + TEXT("/");
    FString ExtensionFilter = TEXT("*.csv");
    TArray<FString> FoundFiles;

    // Build the path to macros
    // if (MacroCategoryFolder.IsEmpty())
    // {
    //     Directory = FPaths::ProjectDir() + TEXT("Macros/");
    // }
    // else
    // {
    //     Directory = FPaths::ProjectDir() + TEXT("Macros/") + MacroCategoryFolder + TEXT("/");
    // }

    // Clear the defaults
    MacrosArray.Empty();
    MacrossArray_FullPath.Empty();
    ScrollingIndex = 0;
    
    // Construct search path (e.g., "C:/MyFolder/*.txt")
    FString SearchPattern = Directory + ExtensionFilter;

    IFileManager& FileManager = IFileManager::Get();

    // Retrieve file list
    FileManager.FindFilesRecursive(FoundFiles, *Directory, *SearchPattern, true, false);

    // Check if array is not empty - assumed to handle more then 1 macro
    int32 MacrosNum = FoundFiles.Num();
    if (MacrosNum <= 0) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::Failed to load files - returning")); bIsSucceed = false; 
        CustomLog_FText_UTIL("GetFilesByCategory", "Failed to load files - returning");
        return;}

    // Obtain full paths to reflect the macros and remove full path to reflect the files name
    for (const FString& FilePath : FoundFiles)
    {
        MacrossArray_FullPath.Add((FilePath));
        MacrosArray.Add(FPaths::GetCleanFilename(FilePath));
    }

    MacroContent = this->ReflectFileToScreen_UTIL(ScrollingIndex);
    SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    bIsSucceed = true;
    CustomLog_FText_UTIL("GetFilesByCategory", "Files are successfully retrieved");
}

void UMacrosManager::ScrollForward(FString &OutContent)
{
    // Check if array is not empty - assumed to handle more then 1 macro
    if (MacrossArray_FullPath.IsEmpty()) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::The MacrossArray_FullPath is empty - returning"));
        CustomLog_FText_UTIL("ScrollForward", "The MacrossArray_FullPath is empty - returning");
        return;}

    // Declare function's defaults
    FString Content;

    // Current scrolling index incrementation
    ScrollingIndex++;

    // Obtain the length of the array
    int32 FullPathsNum = MacrossArray_FullPath.Num() - 1; 

    // Check the length of the array and return the content
    if (ScrollingIndex <= FullPathsNum)
    {
        Content = this->ReflectFileToScreen_UTIL(ScrollingIndex);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
    // Closing the scrolling loop
    else
    {
        ScrollingIndex = 0;
        Content = this->ReflectFileToScreen_UTIL(ScrollingIndex);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
}

void UMacrosManager::ScrollBackward(FString &OutContent)
{
    // Check if array is not empty - assumed to handle more then 1 macro
    if (MacrossArray_FullPath.IsEmpty()) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::The MacrossArray_FullPath is empty - returning"));
        CustomLog_FText_UTIL("ScrollBackward", "The MacrossArray_FullPath is empty - returning");
        return;}

    // Declare function's defaults
    FString Content;

    // Current scrolling index decrementation
    ScrollingIndex--;

    // Obtain the length of the array
    int32 FullPathsNum = MacrossArray_FullPath.Num() - 1; 

    // Closing the scrolling loop
    if (ScrollingIndex < 0)
    {
        ScrollingIndex = FullPathsNum;
        Content = this->ReflectFileToScreen_UTIL(ScrollingIndex);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
    else
    {
        Content = this->ReflectFileToScreen_UTIL(ScrollingIndex);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
}

void UMacrosManager::FetchFilesRecursive_SYNC(FString FullURLPath)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FullURLPath);
    Request->SetVerb("GET");
    Request->OnProcessRequestComplete().BindLambda([this, FullURLPath](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)

    {
        int32 ResponseCode = Response->GetResponseCode();
        if (bWasSuccessful && ResponseCode == 200)
        {
            TArray<TSharedPtr<FJsonValue>> JsonArray;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, JsonArray))
            {
                TArray<FString> FileList;
                for (TSharedPtr<FJsonValue> Value : JsonArray)
                {
                    if (Value.IsValid() && Value->Type == EJson::Object)
                    {
                        TSharedPtr<FJsonObject> Object = Value->AsObject();
                        FString Name = Object->GetStringField(TEXT("name"));
                        FString Type = Object->GetStringField(TEXT("type"));
                        FString Path = Object->GetStringField(TEXT("path"));

                        if (Type == "file")
                        {
                            FileList.Add(Path);
                            UE_LOG(LogTemp, Log, TEXT("File found: %s"), *Path);
                        }
                        else if (Type == "dir") // It's a subfolder, fetch its contents
                        {
                            FString SubFullURLPath = FullURLPath / Path + TEXT("/");
                            FetchFilesRecursive_SYNC(SubFullURLPath); // Recursively fetch files
                        }
                    }
                }
            }
        }
        else
        {
            FString RateLimit = Response->GetHeader("X-RateLimit-Remaining");
            FString RateReset = Response->GetHeader("X-RateLimit-Reset");
            
            UE_LOG(LogTemp, Error, TEXT("Failed to fetch: %s"), *FullURLPath);
            UE_LOG(LogTemp, Error, TEXT("Unexpected response: %d"), ResponseCode);
            UE_LOG(LogTemp, Warning, TEXT("Rate limit remaining: %s, resets at: %s"), *RateLimit, *RateReset);
        }
    });

    Request->ProcessRequest();
}

// void UMacrosManager::SearchInRepository(const FString &RepoOwner, const FString &RepoName, const FString &FolderPath)
// {
// 	// Build the API request URL
// 	FString URL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/contents/Macros/%s"), 
// 		*RepoOwner, *RepoName, *FolderPath);
	
// 	// Create the HTTP request
// 	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
// 	Request->OnProcessRequestComplete().BindUObject(this, &UMacrosManager::OnSearchInRepositoryResponse);
// 	Request->SetURL(URL);
// 	Request->SetVerb(TEXT("GET"));
// 	Request->ProcessRequest();
// }

// void UMacrosManager::OnSearchInRepositoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
// {
// 	if (!bWasSuccessful || !Response.IsValid())
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Request failed or invalid response."));
//         CustomLog_FText_UTIL("OnSearchInRepositoryResponse", "Request failed or invalid response - returning");
// 		return;
// 	}

//     int32 ResponseCode = Response->GetResponseCode();
//     if (ResponseCode == 200) // Folder exists
//     {
//         TArray<TSharedPtr<FJsonValue>> JsonArray;
//         TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        
//         if (FJsonSerializer::Deserialize(Reader, JsonArray))
//         {
//             UE_LOG(LogTemp, Log, TEXT("JSON Response contains %d items:"), JsonArray.Num());

//             for (TSharedPtr<FJsonValue> Value : JsonArray)
//             {
//                 if (Value.IsValid() && Value->Type == EJson::Object)
//                 {
//                     TSharedPtr<FJsonObject> Object = Value->AsObject();
//                     FString Name = Object->GetStringField(TEXT("name"));
//                     FString Type = Object->GetStringField(TEXT("type"));
        
//                     UE_LOG(LogTemp, Log, TEXT("Name: %s, Type: %s"), *Name, *Type);
//                 }
//             }
//         }
//     }
//     else if (ResponseCode == 404)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("❌ Folder does NOT exist."));
//         CustomLog_FText_UTIL("OnSearchInRepositoryResponse", "Folder does NOT exist.");
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("Unexpected response: %d"), ResponseCode);
//         CustomLog_FText_UTIL("OnSearchInRepositoryResponse", (TEXT("Unexpected response: ") + ResponseCode));
//     }

// 	// Check HTTP response code
// 	int32 ResponseCode = Response->GetResponseCode();
// 	if (ResponseCode == 200)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("✅ Folder exists!"));
// 	}
// 	else if (ResponseCode == 404)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("❌ Folder does NOT exist."));
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Unexpected response: %d"), ResponseCode);
// 	}
// }

FString UMacrosManager::ReflectFileToScreen_UTIL(int32 CurrentIndex)
{
    FString Content;
    FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[CurrentIndex]);

    return Content;
}

void UMacrosManager::CustomLog_FText_UTIL(FString FunctionName, FString LogText)
{
    FString logBuild = CustomLogPrefix + FunctionName + TEXT("::") + LogText + TEXT(".");
    CustomLog_TXT->SetText(FText::FromString(logBuild));
}

FDateTime UMacrosManager::CheckLocalChanges(FString LocalFolderPath)
{
    FDateTime LastModifiedUTC = IFileManager::Get().GetTimeStamp(*LocalFolderPath);
    
    // Convert to local time
    FDateTime LastModifiedLocal = LastModifiedUTC + (FDateTime::Now() - FDateTime::UtcNow());

    FString LastModifiedStr = LastModifiedLocal.ToString();
    UE_LOG(LogTemp, Warning, TEXT("Last modified (Local Time): %s"), *LastModifiedStr);

    return LastModifiedLocal;
}

void UMacrosManager::GetLastModifiedFromGitHub(FString RepositoryURL, FDateTime LocalChangesTimeStamp, bool &bIsSyncNeeded)
{
    FString Url = RepositoryURL;

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");

    Request->OnProcessRequestComplete().BindLambda(
        [this, LocalChangesTimeStamp, &bIsSyncNeeded](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("HTTP Response Code: %d"), Response->GetResponseCode());

                FString RateLimit = Response->GetHeader("X-RateLimit-Remaining");
                FString RateReset = Response->GetHeader("X-RateLimit-Reset");
                UE_LOG(LogTemp, Warning, TEXT("Rate limit remaining: %s, resets at: %s"), *RateLimit, *RateReset);

                FString ResponseStr = Response->GetContentAsString();
                // UE_LOG(LogTemp, Warning, TEXT("GitHub API Response: %s"), *ResponseStr);
                
                TArray<TSharedPtr<FJsonValue>> CommitArray;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());                

                if (FJsonSerializer::Deserialize(Reader, CommitArray) && CommitArray.Num() > 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("The JsonObject was successfully serialazide."));

                    // Navigate the JSON structure to find the commit date
                    TSharedPtr<FJsonObject> CommitObject = CommitArray[0]->AsObject();
                    if (CommitObject.IsValid())
                    {
                        FString DateString = CommitObject->GetObjectField(TEXT("commit"))
                                                         ->GetObjectField(TEXT("author"))
                                                         ->GetStringField(TEXT("date"));

                        FDateTime ParsedTime;
                        FDateTime::ParseIso8601(*DateString, ParsedTime);
                        FDateTime LastModifiedLocal = ParsedTime + (FDateTime::Now() - FDateTime::UtcNow());

                        if(LocalChangesTimeStamp != LastModifiedLocal) { bIsSyncNeeded = false; }

                        // FDateTime LastCommitTime = ParseGitHubTimestamp(DateString);
                        UE_LOG(LogTemp, Warning, TEXT("Last GitHub Commit: %s"), *LastModifiedLocal.ToString());
                        UE_LOG(LogTemp, Warning, TEXT("Last Local Changes: %s"), *LocalChangesTimeStamp.ToString());

                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Commits.Num() is less or equal to 0"));
                        return;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to deserialize the JsonObject."));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP Response Code: %d"), Response->GetResponseCode());
                UE_LOG(LogTemp, Error, TEXT("Request failed!"));
                return;
            }
        });

    UE_LOG(LogTemp, Warning, TEXT("Sending request to: %s"), *Url);
    Request->ProcessRequest();
}

void UMacrosManager::SyncLastCommitWithLocalChanges(FString RepositoryURL, FString LocalFolderPath, bool &bIsSyncNeeded)
{
    FDateTime LocalTimeStamp = this->CheckLocalChanges(LocalFolderPath);
    this->GetLastModifiedFromGitHub(RepositoryURL, LocalTimeStamp, bIsSyncNeeded);
}

// void UMacrosManager::GetLocalFiles(const FString &LocalPath, TArray<FString> &OutFiles)
// {
//     IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

//     if (!PlatformFile.DirectoryExists(*LocalPath))
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Local path does not exist: %s"), *LocalPath);
//         return;
//     }

//     // Scan directory
//     OutFiles.Empty();
//     PlatformFile.FindFiles(OutFiles, *LocalPath, nullptr); // nullptr means "all file types"

//     UE_LOG(LogTemp, Log, TEXT("Found %d local files."), OutFiles.Num());
// }

// void UMacrosManager::CompareRepoToLocal(const FString &LocalPath, const TMap<FString, int64>& RemoteFiles)
// {
//     IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

//     if (!PlatformFile.DirectoryExists(*LocalPath))
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Local path does not exist: %s"), *LocalPath);
//         CustomLog_FText_UTIL("CompareRepoToLocal", (TEXT("Local path does not exist: ") + LocalPath));
//         return;
//     }

//     for (const TPair<FString, int64>& RemoteFile : RemoteFiles)
//     {
//         FString LocalFilePath = LocalPath + RemoteFile.Key;

//         if (PlatformFile.FileExists(*LocalFilePath))
//         {
//             int64 LocalFileSize = PlatformFile.FileSize(*LocalFilePath);

//             if (LocalFileSize == RemoteFile.Value)
//             {
//                 UE_LOG(LogTemp, Log, TEXT("✅ File exists & size matches: %s"), *RemoteFile.Key);
//             }
//             else
//             {
//                 UE_LOG(LogTemp, Warning, TEXT("⚠️ File exists but size differs (local: %lld, remote: %lld): %s"), 
//                     LocalFileSize, RemoteFile.Value, *RemoteFile.Key);
//             }
//         }
//         else
//         {
//             UE_LOG(LogTemp, Warning, TEXT("⬇️ File missing locally, needs download: %s"), *RemoteFile.Key);
//         }
//     }
// }