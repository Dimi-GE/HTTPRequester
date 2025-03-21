// Fill out your copyright notice in the Description page of Project Settings.


#include "MacrosManager.h"

#include "HAL/PlatformFilemanager.h"

void UMacrosManager::GetFilesInDirectory_Reviews(bool &bIsSucceed, FString &Output)
{
    // Declare defaults
    FString Directory = FPaths::ProjectDir() + TEXT("Macros/Reviews/");
    FString ExtensionFilter = TEXT("*.csv");
    TArray<FString> FoundFiles;

    // Clear the macros arrays
    MacrosArray.Empty();
    MacrossArray_FullPath.Empty();
    
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

    Output = this->ReflectFileToScreen_INIT();
    SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[0]));
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
        FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[ScrollingIndex]);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
    // Closing the scrolling loop
    else
    {
        ScrollingIndex = 0;
        FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[ScrollingIndex]);
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
        FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[ScrollingIndex]);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
    else
    {
        FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[ScrollingIndex]);
        OutContent = Content;
        SelectedFileName_TXT->SetText(FText::FromString(MacrosArray[ScrollingIndex]));
    }
}

FString UMacrosManager::ReflectFileToScreen_INIT()
{
    FString Content;
    FFileHelper::LoadFileToString(Content, *MacrossArray_FullPath[0]);

    return Content;
}