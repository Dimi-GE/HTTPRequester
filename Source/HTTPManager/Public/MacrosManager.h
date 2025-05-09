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

// UENUM(BlueprintType)
// enum EMacroCategory
// {
// 	YouTube,
// 	Reviews
// };

UCLASS()
class UMacrosManager : public UEditorUtilityWidget
{
	GENERATED_BODY()

	protected:
    virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void RequestDestroyWindow();
	void Destruct();



	public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectedFileName_TXT;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* CustomLog_TXT;

    UPROPERTY(meta = (BindWidget))
    class UImage* SyncImage;

	UPROPERTY(meta = (BindWidget))
	class UButton* RSSInit_B;

	UPROPERTY(meta = (BindWidget))
	class UExpandableArea* MacrosManager_EXP;

	// UPROPERTY(BlueprintReadOnly)
	// EMacroCategory MacroCategoryEnum;

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary", meta = (ExpandBoolAsExecs = "bIsSucceed"))
	void GetFilesByCategory(bool &bIsSucceed, FString &MacroContent, FString MacroCategoryFolder);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollForward(FString &OutContent);
	
	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollBackward(FString &OutContent);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void FetchFilesRecursive_SYNC(FString FullURLPath);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	FDateTime CheckLocalChanges(FString LocalFolderPath);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary", meta = (ExpandBoolAsExecs = "bIsSyncNeeded"))
	void GetLastModifiedFromGitHub(FString RepositoryURL, FString LocalFolderPath, bool &bIsSyncNeeded);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary", meta = (ExpandBoolAsExecs = "bIsSyncNeeded"))
	void SyncLastCommitWithLocalChanges(FString RepositoryURL, FString LocalFolderPath, bool &bIsSyncNeeded);

	// Function to check if folder exists
	// UFUNCTION(BlueprintCallable, Category="HTTP Utilities")
	// void SearchInRepository(const FString& RepoOwner, const FString& RepoName, const FString& FolderPath);

	// UFUNCTION(BlueprintCallable, Category="HTTP")
	// void GetLocalFiles(const FString& LocalPath, TArray<FString>& OutFiles);

	// Delegates
	UFUNCTION()
	void RSSInit();

	private:

	// Utilities
	TArray<FString> MacrosArray;
	TArray<FString> MacrossArray_FullPath;
	FString CustomLogPrefix = "Custom Log Output:\n";

	int32 ScrollingIndex = 0;

	// Callback function when request completes
	// void OnSearchInRepositoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	// void CompareRepoToLocal(const FString& LocalPath, const TMap<FString, int64>& RemoteFiles);

	FString ReflectFileToScreen_UTIL(int32 CurrentIndex);
	void CustomLog_FText_UTIL(FString FunctionName, FString LogText);
	void HandleThisLifycycle();
};
