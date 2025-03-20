// Fill out your copyright notice in the Description page of Project Settings.


#include "HTTPRequester.h"
// HTTP Request
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFilemanager.h"

void UHTTPRequester::DownloadFile(const FString& URL, bool bSaveToFile, FOnDownloadResponse Callback)
{
	// Store the Blueprint callback function
	DownloadResponseDelegate = Callback;

	// Create an HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UHTTPRequester::OnResponseReceived);
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

void UHTTPRequester::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response.IsValid())
	{
		FString FileContent = Response->GetContentAsString();
		UE_LOG(LogTemp, Log, TEXT("Downloaded content: %s"), *FileContent);

		// Call the Blueprint event if it's valid
		if (DownloadResponseDelegate.IsBound())
		{
			DownloadResponseDelegate.Execute(FileContent);

            // Save the file locally (optional)
            FString SavePath = FPaths::ProjectDir() + TEXT("DownloadedFile.txt");
            FFileHelper::SaveStringToFile(FileContent, *SavePath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to download file!"));
		if (DownloadResponseDelegate.IsBound())
		{
			DownloadResponseDelegate.Execute(TEXT("ERROR"));
		}
	}
}