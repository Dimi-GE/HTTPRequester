// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "UObject/NoExportTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HTTPRequester.generated.h"

// class IHttpRequest;
// class IHttpResponse;

UCLASS()
class HTTPMANAGER_API UHTTPRequester : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	public:

	UFUNCTION(BlueprintCallable, Category="HTTP Utilities", meta=(WorldContext="WorldContextObject"))
	void DownloadFile();

	private:

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
