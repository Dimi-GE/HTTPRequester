// Fill out your copyright notice in the Description page of Project Settings.


#include "HTTPRequester.h"
// HTTP Request
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFilemanager.h"

void UHTTPRequester::DownloadFile()
{
    // Replace with the raw URL of your GitHub file
    FString FileURL = TEXT("https://raw.githubusercontent.com/Dimi-GE/HTTPRequester/refs/heads/main/README.md");

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UHTTPRequester::OnResponseReceived);
    Request->SetURL(FileURL);
    Request->SetVerb(TEXT("GET"));
    Request->ProcessRequest();
}

void UHTTPRequester::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        FString FileContent = Response->GetContentAsString();
        UE_LOG(LogTemp, Log, TEXT("File Content:\n%s"), *FileContent);

        // Save the file locally (optional)
        FString SavePath = FPaths::ProjectDir() + TEXT("DownloadedFile.txt");
        FFileHelper::SaveStringToFile(FileContent, *SavePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to download file!"));
    }
}