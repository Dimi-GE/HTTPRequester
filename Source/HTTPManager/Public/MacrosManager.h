// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// Editor Utility Widget
#include "EditorUtilityWidget.h"
#include "Components/TextBlock.h"
// HTTP Interfaces
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
// JSON
#include "Json.h"
#include "JsonUtilities.h"

#include "MacrosManager.generated.h"

/**
 * 
 */
UCLASS()
class UMacrosManager : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	public:


	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectedFileName_TXT;

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary", meta = (ExpandBoolAsExecs = "bIsSucceed"))
	void GetFilesInDirectory_Reviews(bool &bIsSucceed, FString &Output);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollForward(FString &OutContent);
	
	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollBackward(FString &OutContent);

	// Function to check if folder exists
	UFUNCTION(BlueprintCallable, Category="HTTP Utilities")
	void SearchInRepository(const FString& RepoOwner, const FString& RepoName, const FString& FolderPath);

	private:

	// Utilities
	TArray<FString> MacrosArray;
	TArray<FString> MacrossArray_FullPath;

	int32 ScrollingIndex = 0;

	// Callback function when request completes
	void OnSearchInRepositoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	FString ReflectFileToScreen_UTIL(int32 CurrentIndex);
};
