// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MacrosManager.generated.h"

/**
 * 
 */
UCLASS()
class UMacrosManager : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	public:

	UFUNCTION(BlueprintCallable, Category = "MacrosManagerLibrary")
	void GetFilesInDirectory_Reviews(bool &bIsSucceed, FString &Output);

	UPROPERTY(BlueprintReadOnly,  Category = "MacrosManagerLibrary")
	TArray<FString> MacrosArray;

	private:

	// Utilities
	TArray<FString> MacrossArray_FullPath;

	FString ReflectFileToScreen_INIT();
};
