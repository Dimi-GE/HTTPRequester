// Fill out your copyright notice in the Description page of Project Settings.


#include "MacrosManager.h"
// Components
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ExpandableArea.h" 
#include "Components/ComboBoxString.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/MultiLineEditableTextBox.h"

// Utilities
#include "HAL/PlatformFilemanager.h"
// Custom Utilities Externals
extern UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue);
extern void ThrowDialogMessage(FString Message);
// extern TSharedPtr<FJsonObject> ThrowRSSInitObject(FString RSSInitModule, FString JSONObject, int32 ReadWriteBinary);

extern TArray<TSharedPtr<FJsonValue>> ThrowJsonArrayFromFile_UTIL(FString JSONSubPath);
extern TSharedPtr<FJsonObject> ThrowRSSInitModule_UTIL(TArray<TSharedPtr<FJsonValue>> JsonArray, FString RSSInitModule, FString RSSInitField);
extern void SaveJsonArrayToFile_UTIL(const FString& JSONSubPath, const TArray<TSharedPtr<FJsonValue>>& JsonArray);

extern void RSSManifestInit_UTIL();

extern void MakeZIPInDir();
extern void UnZipInDir();
extern void ThrowTimer_UTIL();

// Integration externals
extern FString CreateRSSSyncTempFolder();
extern bool CreateManifest();
extern bool EnsureLocalManifestExists();
extern TArray<FString> CompareManifestsAndFindDifferences(bool bExecuteChanges, const FString& TempFolder);
extern bool ApplyFileChanges(const FString& TempFolder, bool bUpdateLocal = true);
extern bool UpdateManifestsAfterChanges(const FString& TempFolder);
extern bool PackUpdatedBranchToZip(const FString& TempFolder);
extern void DownloadBranchFromGitHub(const FString& RepoOwner, const FString& RepoName, 
                             const FString& BranchName, const FString& AccessToken,
                             const FString& TempFolder, TFunction<void(bool)> OnDownloadComplete);
extern void DownloadBranchWithValidation(const FString& RepoOwner, const FString& RepoName, 
                                 const FString& BranchName, const FString& AccessToken,
                                 const FString& TempFolder, TFunction<void(bool, FString)> OnComplete);
extern void UploadUpdatedBranchWithValidation(const FString& RepoOwner, const FString& RepoName,
                                      const FString& BranchName, const FString& AccessToken,
                                      const FString& TempFolder, const FString& CommitMessage,
                                      TFunction<void(bool, FString)> OnComplete);


void UMacrosManager::NativePreConstruct()
{
    Super::NativePreConstruct();

    // ThrowDialogMessage("Remember to sync changes before continue any further.");
}

void UMacrosManager::NativeConstruct()
{
    Super::NativeConstruct();

    // Buttons Dynamic Delegates
    RSSInit_BTN->OnClicked.AddDynamic(this, &UMacrosManager::RSSInit);
    RSSManifestInit_BTN->OnClicked.AddDynamic(this, &UMacrosManager::RSSManifestInit);
    ZIP_BTN->OnClicked.AddDynamic(this, &UMacrosManager::ZIPFiles_UTIL);
    UNZIP_BTN->OnClicked.AddDynamic(this, &UMacrosManager::UNZIPFiles_UTIL);

    this->HandleThisLifycycle();

    // SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(1));
    // ThrowDialogMessage("Remember to sync changes before continue any further.");
}

void UMacrosManager::NativeDestruct()
{
    Super::NativeDestruct();

    // ThrowDialogMessage("Remember to sync changes before continue any further.");
}

void UMacrosManager::Destruct()
{
    // ThrowDialogMessage("Remember to sync changes before continue any further.");
    UE_LOG(LogTemp, Error, TEXT("Destruct fired"));
}

void UMacrosManager::RequestDestroyWindow()
{
    UE_LOG(LogTemp, Error, TEXT("Destruct fired"));
}

void UMacrosManager::HandleThisLifycycle()
{
    FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
    FString RSSInitModule = TEXT("LifecycleInit");
    FString RSSInitField = TEXT("MacrosManager");

    TArray<TSharedPtr<FJsonValue>> JsonArray = ThrowJsonArrayFromFile_UTIL(RSSInitSubPath);
    if (JsonArray.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("HandleThisLifycle::JsonArray is empty - returning."));
        return;
    }
    
    TSharedPtr<FJsonObject> RSSMacrosManager = ThrowRSSInitModule_UTIL(JsonArray, RSSInitModule, RSSInitField);
    if (RSSMacrosManager == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleThisLifycle::MacrosManager is nullptr - returning."));
        return;
    }

    if (RSSMacrosManager->GetBoolField(TEXT("bIsInitialized")))
    {
        SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(RSSMacrosManager->GetNumberField(TEXT("SyncState"))));
        return;
    }

    MacrosManager_EXP->SetIsEnabled(false);
    MacrosManager_EXP->SetIsExpanded(false);
}

void UMacrosManager::RSSManifestInit()
{
    RSSManifestInit_UTIL();
}

// The function is designed to initialize the Macros Manager as an editor window; 
// The main responsibility is tracking post-sync progress by making a timestamp - it should prevent loosing data after widgets recompilation; 
void UMacrosManager::RSSInit()
{
    FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
    FString RSSInitModule = TEXT("LifecycleInit");
    FString RSSInitField = TEXT("MacrosManager");

    TArray<TSharedPtr<FJsonValue>> JsonArray = ThrowJsonArrayFromFile_UTIL(RSSInitSubPath);
    if (JsonArray.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RSSInit::JsonArray is empty - returning."));
        return;
    }
    
    TSharedPtr<FJsonObject> RSSMacrosManager = ThrowRSSInitModule_UTIL(JsonArray, RSSInitModule, RSSInitField);
    if (RSSMacrosManager == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RSSInit::MacrosManager is nullptr - returning."));
        return;
    }

    if (RSSMacrosManager->GetBoolField(TEXT("bIsInitialized")))
    {
        return;
    }

    SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(RSSMacrosManager->GetNumberField(TEXT("SyncState"))));
    RSSMacrosManager->SetBoolField(TEXT("bIsInitialized"), true);
    MacrosManager_EXP->SetIsEnabled(true);

    SaveJsonArrayToFile_UTIL(RSSInitSubPath, JsonArray);

    // UE_LOG(LogTemp, Warning, TEXT("RSSInit::Initialization successful - %f."), RSSMacrosManager->GetNumberField(TEXT("SyncState")));

    // TSharedPtr<FJsonObject> MacrosManager = ThrowRSSInitObject(RSSInitModule, RSSInitObject, ReadWriteBinary);

    // if(MacrosManager == nullptr)
    // {
    //     UE_LOG(LogTemp, Error, TEXT("RSSInit::MacrosManager is nullptr - returning."));
    //     return;
    // }

    // MacrosManager->SetNumberField(TEXT("SyncState"), 0);

    // UE_LOG(LogTemp, Warning, TEXT("RSSInit::Initialization successful."));
    // FString RSSInitPath = FPaths::ProjectDir() + TEXT("\\RSS\\RSSInit.json");
    // FString JsonString;

    // if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    // {
    //     UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
    //     return;
    // }

    // TArray<TSharedPtr<FJsonValue>> JsonArray;
    // TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    // if (FJsonSerializer::Deserialize(Reader, JsonArray))
    // {
    //     for (const TSharedPtr<FJsonValue>& Item : JsonArray)
    //     {
    //         TSharedPtr<FJsonObject> RootObject = Item->AsObject();
    //         if (RootObject->HasField(TEXT("lifecycleinit")))
    //         {
    //             TSharedPtr<FJsonObject> LifecycleInit = RootObject->GetObjectField(TEXT("LifecycleInit"));
    //             if (LifecycleInit->HasField(TEXT("MacrosManager")))
    //             {
    //                 TSharedPtr<FJsonObject> MacrosManager = LifecycleInit->GetObjectField(TEXT("MacrosManager"));
    
    //                 bool bIsInitialized = MacrosManager->GetBoolField(TEXT("bIsInitialized"));
    //                 MacrosManager_EXP->SetIsEnabled(bIsInitialized);

    //                 int32 SyncState = MacrosManager->GetIntegerField(TEXT("SyncState"));
    //                 SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(SyncState));
    //             }
    //             else
    //             {
    //                 UE_LOG(LogTemp, Error, TEXT("Failed to find MacrosManager field."));
    //             }
    //         }
    //         else
    //         {
    //             UE_LOG(LogTemp, Error, TEXT("Failed to find lifecycleinit field."));
    //         }
    //     }
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON."));
    //     UE_LOG(LogTemp, Warning, TEXT("JSON Raw: %s"), *JsonString);
    //     MacrosManager_EXP->SetIsEnabled(false);
    //     return;
    // }
}

// The function is designed to read files under a specific path - bound to work in the editor utility widget blueprint;
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

// The function allows forward scrolling through loaded files - bound to work in the editor utility widget blueprint;
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

// The function allows backward scrolling through loaded files - bound to work in the editor utility widget blueprint;
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

// The function is under development - potentially deprecated;
// Imitates recursion by checking subdirectories for last changes on the repository; 
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

// Reflects content to a text - bound to work in the editor utility widget blueprint;
FString UMacrosManager::ReflectFileToScreen_UTIL(int32 CurrentIndex)
{
    FString Content;
    FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[CurrentIndex]);

    return Content;
}

// The function build custom log message - bound to work in the editor utility widget blueprint;
void UMacrosManager::CustomLog_FText_UTIL(FString FunctionName, FString LogText)
{
    FString logBuild = CustomLogPrefix + FunctionName + TEXT("::") + LogText + TEXT(".");
    CustomLog_TXT->SetText(FText::FromString(logBuild));
}

// The function checks when the last local changes were made;
FDateTime UMacrosManager::CheckLocalChanges(FString LocalFolderPath)
{
    FDateTime LastModifiedUTC = IFileManager::Get().GetTimeStamp(*LocalFolderPath);
    
    // Convert to local time
    FDateTime LastModifiedLocal = LastModifiedUTC + (FDateTime::Now() - FDateTime::UtcNow());

    // FString LastModifiedStr = LastModifiedLocal.ToString();
    // UE_LOG(LogTemp, Warning, TEXT("Last modified (Local Time): %s"), *LastModifiedStr);

    return LastModifiedLocal;
}

// The function checks when the last changes were made in files on GitHub;
void UMacrosManager::GetLastModifiedFromGitHub(FString RepositoryURL, FString LocalFolderPath, bool &bIsSyncNeeded)
{
    FString Url = RepositoryURL;
   
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");

    Request->OnProcessRequestComplete().BindLambda(
        [this, LocalFolderPath, &bIsSyncNeeded](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
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
                        FDateTime LocalTimeStamp = this->CheckLocalChanges(LocalFolderPath);
                        FDateTime GitHubTimeStamp = ParsedTime + (FDateTime::Now() - FDateTime::UtcNow());

                        FTimespan Difference = GitHubTimeStamp - LocalTimeStamp;

                        if (GitHubTimeStamp > LocalTimeStamp && FMath::Abs(Difference.GetTotalMinutes()) > 2.0)
                        {
                            bIsSyncNeeded = true;
                            FString logBuild = FString::Printf(TEXT("Last Local Changes: %s\nLast GitHub Commit: %s"), *LocalTimeStamp.ToString(), *GitHubTimeStamp.ToString());
                            CustomLog_TXT->SetText(FText::FromString(logBuild));

                            SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(2));

                            FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
                            FString RSSInitModule = TEXT("LifecycleInit");
                            FString RSSInitField = TEXT("MacrosManager");

                            TArray<TSharedPtr<FJsonValue>> JsonArray = ThrowJsonArrayFromFile_UTIL(RSSInitSubPath);
                            if (JsonArray.IsEmpty())
                            {
                                UE_LOG(LogTemp, Error, TEXT("RSSInit::JsonArray is empty - returning."));
                                return;
                            }
                            
                            TSharedPtr<FJsonObject> RSSMacrosManager = ThrowRSSInitModule_UTIL(JsonArray, RSSInitModule, RSSInitField);
                            if (RSSMacrosManager == nullptr)
                            {
                                UE_LOG(LogTemp, Error, TEXT("RSSInit::MacrosManager is nullptr - returning."));
                                return;
                            }

                            RSSMacrosManager->SetNumberField(TEXT("SyncState"), 2);
                            RSSMacrosManager->SetStringField(TEXT("RateLimit"), *RateLimit);
                            RSSMacrosManager->SetStringField(TEXT("RateLimitResetAt"), *RateReset);
                            RSSMacrosManager->SetNumberField(TEXT("ResponseCode"), 200);

                            SaveJsonArrayToFile_UTIL(RSSInitSubPath, JsonArray);
                            
                            UE_LOG(LogTemp, Warning, TEXT("Last Local Changes: %s"), *LocalTimeStamp.ToString());
                            UE_LOG(LogTemp, Warning, TEXT("Last GitHub Commit: %s"), *GitHubTimeStamp.ToString());
                        }
                        else
                        {
                            SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(0));

                            // Re-wrap into another function in order to change a single specific parameter
                            // Alternatively - set up RSSInit as completed only aftere sync
                            // this->RSSInit();

                            FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
                            FString RSSInitModule = TEXT("LifecycleInit");
                            FString RSSInitField = TEXT("MacrosManager");

                            TArray<TSharedPtr<FJsonValue>> JsonArray = ThrowJsonArrayFromFile_UTIL(RSSInitSubPath);
                            if (JsonArray.IsEmpty())
                            {
                                UE_LOG(LogTemp, Error, TEXT("RSSInit::JsonArray is empty - returning."));
                                return;
                            }
                            
                            TSharedPtr<FJsonObject> RSSMacrosManager = ThrowRSSInitModule_UTIL(JsonArray, RSSInitModule, RSSInitField);
                            if (RSSMacrosManager == nullptr)
                            {
                                UE_LOG(LogTemp, Error, TEXT("RSSInit::MacrosManager is nullptr - returning."));
                                return;
                            }

                            RSSMacrosManager->SetNumberField(TEXT("SyncState"), 0);
                            RSSMacrosManager->SetStringField(TEXT("RateLimit"), *RateLimit);
                            RSSMacrosManager->SetStringField(TEXT("RateLimitResetAt"), *RateReset);
                            RSSMacrosManager->SetNumberField(TEXT("ResponseCode"), 200);

                            SaveJsonArrayToFile_UTIL(RSSInitSubPath, JsonArray);

                            FString logBuild = FString::Printf(TEXT("All changes are synchronized."));
                            CustomLog_TXT->SetText(FText::FromString(logBuild));
                            UE_LOG(LogTemp, Warning, TEXT("The sync is not needed."));
                        }
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

// The function is potentially deprecated - don't remember what it was designed for;
void UMacrosManager::SyncLastCommitWithLocalChanges(FString RepositoryURL, FString LocalFolderPath, bool &bIsSyncNeeded)
{
    FDateTime LocalTimeStamp = this->CheckLocalChanges(LocalFolderPath);
    // this->GetLastModifiedFromGitHub(RepositoryURL, LocalTimeStamp, bIsSyncNeeded);
}

void UMacrosManager::ZIPFiles_UTIL()
{
    UE_LOG(LogTemp, Warning, TEXT("ZIPFiles_UTIL::Called."));
    MakeZIPInDir();
}

void UMacrosManager::UNZIPFiles_UTIL()
{
    UE_LOG(LogTemp, Warning, TEXT("UNZIPFiles_UTIL::Called."));
    UnZipInDir();

    // ThrowTimer_UTIL();
}

void UMacrosManager::TestWrapperFunction()
{
    // FString TempPath = CreateRSSSyncTempFolder();

    // UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::TempPath: %s"), *TempPath);

    // bool bSuccess = CreateManifest();
    // if (bSuccess)
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::Remote manifest created successfully."));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Failed to create remote manifest."));
    // }

    // bool bExecute = false; // Set to true to execute changes, false to just compare manifests

    // FString TempFolder = TEXT("D:/[DGE]/Projects/HTTPRequester/Temp/RSSSync/");

    // TArray<FString> Diffs = CompareManifestsAndFindDifferences(bExecute, TempFolder);

    // bool bUpdateLocal = false; // Set to true to update local files, false to just apply changes
    // Create proper temp folder for download
    FString TempFolder = TEXT("D:/[DGE]/Projects/HTTPRequester/Temp/RSSSync/");
    if (TempFolder.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Failed to create temp folder"));
        return;
    }

    bool bSuccess = PackUpdatedBranchToZip(TempFolder);
    // if(bSuccess)
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::Manifests updated successfully."));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Failed to update manifests."));
    // }
    // // Format: https://api.github.com/repos/{owner}/{repo}/zipball/{branch}    TFunction<void(bool)> OnDownloadComplete;    // Enhanced callback with error message support for private repos
    // TFunction<void(bool, FString)> OnDownloadCompleteEnhanced = [this, TempFolder](bool bSuccess, FString ErrorMessage)
    // {
    //     if (bSuccess)
    //     {
    //         UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::Download completed successfully to %s"), *TempFolder);
    //         // You can call ApplyFileChanges here if needed
    //         // bool bApplySuccess = ApplyFileChanges(TempFolder);
    //         // if (bApplySuccess)
    //         // {
    //         //     UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::File changes applied successfully."));
    //         // }
    //         // else
    //         // {
    //         //     UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Failed to apply file changes."));
    //         // }
    //     }
    //     else
    //     {
    //         UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Download failed: %s"), *ErrorMessage);
    //     }
    // };
    
    // FString RepoOwner = TEXT("Dimi-GE");
    // FString RepoName = TEXT("EasyGitHub");
    // FString BranchName = TEXT("main");
    // FString AccessToken = TEXT("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"); // Expires in 1 week, replace with your own token

    // UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::Using ENHANCED download with PRIVATE REPO support"));
    
    // // Use the enhanced function that validates token first (essential for private repos)
    // DownloadBranchWithValidation(RepoOwner, RepoName, BranchName, AccessToken,
    //                             TempFolder, OnDownloadCompleteEnhanced);
                             
    // UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::Enhanced download started - Editor should remain responsive"));

    // bool bSuccess = ApplyFileChanges(TempFolder, bUpdateLocal);
    // if( bSuccess)
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("TestWrapperFunction::File changes applied successfully."));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("TestWrapperFunction::Failed to apply file changes."));
    // }



}

/**
 * Test function for Enhanced Upload with Write Permission Validation
 * 
 * This function demonstrates how to use the enhanced upload function that validates
 * write permissions before attempting to upload to GitHub repositories.
 */
void UMacrosManager::TestEnhancedUploadFunction()
{
    UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::Starting enhanced upload test with permission validation"));
    
    // Create proper temp folder for upload
    FString TempFolder = TEXT("D:/[DGE]/Projects/HTTPRequester/Temp/RSSSync/branch_download/");
    if (TempFolder.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("TestEnhancedUpload::Failed to create temp folder"));
        return;
    }
    
    // Enhanced callback with detailed error message support for upload operations
    TFunction<void(bool, FString)> OnUploadCompleteEnhanced = [this, TempFolder](bool bSuccess, FString ErrorMessage)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::Upload completed successfully to repository"));
            // You could trigger additional actions here, like refreshing local manifests
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TestEnhancedUpload::Upload failed: %s"), *ErrorMessage);
            
            // Provide user-friendly guidance based on error type
            if (ErrorMessage.Contains(TEXT("lacks push")))
            {
                UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::SOLUTION: Generate a new token with 'repo' scope for private repos"));
            }
            else if (ErrorMessage.Contains(TEXT("401")) || ErrorMessage.Contains(TEXT("Invalid")))
            {
                UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::SOLUTION: Check if your access token has expired"));
            }
        }
    };
      FString RepoOwner = TEXT("Dimi-GE");
    FString RepoName = TEXT("EasyGitHub");
    FString BranchName = TEXT("main");
    FString AccessToken = TEXT(""); // Expires in 1 week, replace with your own token
    FString CommitMessage = TEXT("Update files via Unreal Engine RSS Sync system");

    // NOTE: For testing purposes, use an extracted folder path instead of TempFolder
    // This way you can test ZIP extraction separately from upload
    // FString ExtractedFolderPath = TempFolder + TEXT("extracted/");
    
    UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::Using ENHANCED upload with WRITE PERMISSION validation"));
    // UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::ExtractedFolderPath: %s"), *ExtractedFolderPath);
    
    // Use the enhanced function that validates write permissions first
    // NOTE: Passing ExtractedFolderPath instead of TempFolder for better architecture
    UploadUpdatedBranchWithValidation(RepoOwner, RepoName, BranchName, AccessToken,
                                     TempFolder, CommitMessage, OnUploadCompleteEnhanced);
                             
    UE_LOG(LogTemp, Warning, TEXT("TestEnhancedUpload::Enhanced upload validation started - Editor should remain responsive"));
}

/**
 * Enhanced Test Function for Complete Upload Workflow
 * 
 * This function tests all 5 steps of the upload workflow:
 * 1. Read files from extracted folder
 * 2. Create blob objects for each file
 * 3. Create tree object with all blobs  
 * 4. Create commit with valid tree SHA
 * 5. Update branch reference
 */
void UMacrosManager::TestCompleteUploadWorkflow()
{
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::Starting complete 5-step upload workflow test"));
    
    // Test parameters - update these for your repository
    FString RepoOwner = TEXT("Dimi-GE");
    FString RepoName = TEXT("EasyGitHub");
    FString BranchName = TEXT("main");
    FString AccessToken = TEXT(""); // Add your token here for testing
    FString CommitMessage = TEXT("Complete workflow test from Unreal Engine");
    
    // Path to extracted files (not ZIP) - for testing, use your existing extracted folder
    FString ExtractedFolderPath = TEXT("D:/[DGE]/Projects/HTTPRequester/Temp/RSSSync/branch_download/");
    
    if (AccessToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("TestCompleteUpload::Please set AccessToken for testing"));
        return;
    }
    
    if (!IFileManager::Get().DirectoryExists(*ExtractedFolderPath))
    {
        UE_LOG(LogTemp, Error, TEXT("TestCompleteUpload::Extracted folder not found: %s"), *ExtractedFolderPath);
        UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::Create the folder and add test files, or run download first"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::=== WORKFLOW STATUS ==="));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::✅ Step 1: Read files from extracted folder - IMPLEMENTED"));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::✅ Step 2: Create blob objects for each file - IMPLEMENTED"));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::✅ Step 3: Create tree object with all blobs - IMPLEMENTED"));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::✅ Step 4: Create commit with valid tree SHA - IMPLEMENTED"));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::✅ Step 5: Update branch reference - IMPLEMENTED"));
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::=== ALL STEPS READY FOR TESTING ==="));
    
    // Enhanced callback with detailed status reporting
    TFunction<void(bool, FString)> OnUploadCompleteDetailed = [this](bool bSuccess, FString ErrorMessage)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::🎉 SUCCESS: Complete 5-step upload workflow executed successfully!"));
            UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::All files have been uploaded to GitHub repository"));
            UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::You can now verify the changes on GitHub"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TestCompleteUpload::❌ FAILED: Upload workflow failed: %s"), *ErrorMessage);
            UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::Check the logs above to see which step failed"));
        }
    };
    
    // Use the enhanced upload function with full validation
    UploadUpdatedBranchWithValidation(RepoOwner, RepoName, BranchName, AccessToken,
                                     ExtractedFolderPath, CommitMessage, OnUploadCompleteDetailed);
                             
    UE_LOG(LogTemp, Warning, TEXT("TestCompleteUpload::Upload workflow initiated - Monitor logs for progress"));
}