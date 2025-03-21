// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Components/TextBlock.h"

#include "MacrosManager.generated.h"

/**
 * 
 */
UCLASS()
class UMacrosManager : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	public:

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary", meta = (ExpandBoolAsExecs = "bIsSucceed"))
	void GetFilesInDirectory_Reviews(bool &bIsSucceed, FString &Output);

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollForward(FString &OutContent);
	
	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void ScrollBackward(FString &OutContent);

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectedFileName_TXT;

	private:

	// Utilities
	TArray<FString> MacrosArray;
	TArray<FString> MacrossArray_FullPath;

	int32 ScrollingIndex = 0;

	FString ReflectFileToScreen_INIT();
};
