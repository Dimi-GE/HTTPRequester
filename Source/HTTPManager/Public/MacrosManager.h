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
	
	// Expandable Area
	UPROPERTY(meta = (BindWidget))
	class UExpandableArea* MacrosManager_EXP;

	// Text Blocks
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectedFileName_TXT;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LOADALL_TXT;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectCategory_TXT;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HeaderText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScrollBack_TXT;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScrollForward_TXT;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* CustomLog_TXT;

	// Imagess
    UPROPERTY(meta = (BindWidget))
    class UImage* SyncImage;

	// Buttons
	UPROPERTY(meta = (BindWidget))
	class UButton* RSSInit_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* LOADALL_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* RSSManifestInit_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* ZIP_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* UNZIP_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* SYNC_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* ScrollBack_BTN;

	UPROPERTY(meta = (BindWidget))
	class UButton* ScrollForward_BTN;

	// Boxes
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UMultiLineEditableTextBox* CodeReflectionField_MLTXTB;

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* FileExtension_CBS;

    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* Main_VB;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* Category_HB;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* SyncMacros_HB;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SyncState_SB;

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

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void TestWrapperFunction();

	// Delegates
	UFUNCTION()
	void RSSInit();

	UFUNCTION()
	void RSSManifestInit();

	UFUNCTION()
	void ZIPFiles_UTIL();

	UFUNCTION()
	void UNZIPFiles_UTIL();

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
