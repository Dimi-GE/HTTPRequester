// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// Editor Utility Widget
#include "EditorUtilityWidget.h"
#include "UObject/NoExportTypes.h"
// HTTP Interfaces
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HTTPRequester.generated.h"

// Dynamic delegate that Blueprints can pass as a custom event
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDownloadResponse, const FString&, FileContent);

UCLASS()
class HTTPMANAGER_API UHTTPRequester : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	public:
	
	// Function to start downloading (now accepts a Blueprint event)
	UFUNCTION(BlueprintCallable, Category="HTTP Utilities", meta = (AdvancedDisplay = "bSaveToFile"))
	void DownloadFile(const FString& URL, bool bSaveToFile, FOnDownloadResponse Callback);


	private:
	// Store the callback to call later
	FOnDownloadResponse DownloadResponseDelegate;


	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
