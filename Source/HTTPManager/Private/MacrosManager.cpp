// Fill out your copyright notice in the Description page of Project Settings.


#include "MacrosManager.h"

#include "HAL/PlatformFilemanager.h"

void UMacrosManager::GetFilesInDirectory_Reviews(bool &bIsSucceed, FString &Output)
{
    // Declare defaults
    FString Directory = FPaths::ProjectDir() + TEXT("Macros/Reviews/");
    FString ExtensionFilter = TEXT("*.csv");
    TArray<FString> FoundFiles;

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
    if (MacrosNum <= 0) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::Failed to load files - returning")); bIsSucceed = false; return;}

    // Obtain full paths to reflect the macros and remove full path to reflect the files name
    for (const FString& FilePath : FoundFiles)
    {
        MacrossArray_FullPath.Add((FilePath));
        MacrosArray.Add(FPaths::GetCleanFilename(FilePath));
    }

    Output = this->ReflectFileToScreen_UTIL(ScrollingIndex);
    SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    bIsSucceed = true;
}

void UMacrosManager::ScrollForward(FString &OutContent)
{
    // Check if array is not empty - assumed to handle more then 1 macro
    if (MacrossArray_FullPath.IsEmpty()) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::The MacrossArray_FullPath is empty - returning")); return;}

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
    if (MacrossArray_FullPath.IsEmpty()) {UE_LOG(LogTemp, Error, TEXT("UMacrosManager::The MacrossArray_FullPath is empty - returning")); return;}

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

void UMacrosManager::SearchInRepository(const FString &RepoOwner, const FString &RepoName, const FString &FolderPath)
{
	// Build the API request URL
	FString URL = FString::Printf(TEXT("https://api.github.com/repos/%s/%s/contents/Macros/%s"), 
		*RepoOwner, *RepoName, *FolderPath);
	
	// Create the HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UMacrosManager::OnSearchInRepositoryResponse);
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

void UMacrosManager::OnSearchInRepositoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed or invalid response."));
		return;
	}

	// Check HTTP response code
	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 200)
	{
		UE_LOG(LogTemp, Log, TEXT("✅ Folder exists!"));
	}
	else if (ResponseCode == 404)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Folder does NOT exist."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Unexpected response: %d"), ResponseCode);
	}
}

FString UMacrosManager::ReflectFileToScreen_UTIL(int32 CurrentIndex)
{
    FString Content;
    FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[CurrentIndex]);

    return Content;
}