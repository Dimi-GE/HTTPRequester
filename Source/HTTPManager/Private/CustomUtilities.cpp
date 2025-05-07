// Materials Interfaces
#include "Materials/MaterialInstanceDynamic.h"
// Engine
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
// Messaging
#include "Misc/MessageDialog.h"
// JSON
#include "Json.h"
#include "JsonUtilities.h"

// Forward Declared Functions
TSharedPtr<FJsonObject> ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWrite);
TArray<TSharedPtr<FJsonValue>> ThrowJsonArrayFromFile(FString JSONSubPath, int32 ReadWriteBinary);

// The function throws material instance dynamic - hard-coded to work M_SyncNotify so far;
UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue)
{
    static FSoftObjectPath MatPath(TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify"));
    
    UMaterialInterface* BaseMat = Cast<UMaterialInterface>(MatPath.TryLoad());
    if (!BaseMat) return nullptr;

    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, nullptr);
    if (DynMat)
    {
        DynMat->SetScalarParameterValue(FName("SwitchParameter"), ScalarValue);
    }

    return DynMat;
}

// The function throws popups of different types - investigate how to bind actions depending on user's interaction;
void ThrowDialogMessage(FString Message)
{
    FText MsgTitle = FText::FromString(TEXT("Warning!"));
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString((TEXT("%s"), *Message)), &MsgTitle);
}

// The function takes 2 arguments - module and desired object, returning the last one;
// Hard-coded to search in the RSSInit.json 
TSharedPtr<FJsonObject> ThrowRSSInitObject(FString RSSInitModule, FString RSSInitObject, int32 ReadWriteBinary)
{
    FString RSSInitSubPath = TEXT("\\RSS\\RSSInit.json");
    TSharedPtr<FJsonObject> JSONObject;

    TSharedPtr<FJsonObject> ModuleAsObject = ThrowRSSInitModule_RWUtil(RSSInitSubPath, ReadWriteBinary);

    if (ModuleAsObject->HasField(RSSInitModule))
    {
        TSharedPtr<FJsonObject> LifecycleInit = ModuleAsObject->GetObjectField(RSSInitModule);
        if (LifecycleInit->HasField(RSSInitObject))
        {
            JSONObject = LifecycleInit->GetObjectField(RSSInitObject);

            // bool bIsInitialized = MacrosManager->GetBoolField(TEXT("bIsInitialized"));
            // MacrosManager_EXP->SetIsEnabled(bIsInitialized);

            // int32 SyncState = MacrosManager->GetIntegerField(TEXT("SyncState"));
            // SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(SyncState));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find JSONObject field."));
            return nullptr;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find %s (RSSInitModule) field."), *RSSInitModule);
        return nullptr; 
    }
    
    // if (FJsonSerializer::Deserialize(Reader, JsonArray))
    // {
    //     for (const TSharedPtr<FJsonValue>& Item : JsonArray)
    //     {
    //         TSharedPtr<FJsonObject> ModuleAsObject = Item->AsObject();
    //         if (ModuleAsObject->HasField(TEXT("LifecycleInit")))
    //         {
    //             TSharedPtr<FJsonObject> LifecycleInit = ModuleAsObject->GetObjectField(TEXT("LifecycleInit"));
    //             if (LifecycleInit->HasField(TEXT("MacrosManager")))
    //             {
    //                 MacrosManager = LifecycleInit->GetObjectField(TEXT("MacrosManager"));
    
    //                 // bool bIsInitialized = MacrosManager->GetBoolField(TEXT("bIsInitialized"));
    //                 // MacrosManager_EXP->SetIsEnabled(bIsInitialized);

    //                 // int32 SyncState = MacrosManager->GetIntegerField(TEXT("SyncState"));
    //                 // SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(SyncState));
    //             }
    //             else
    //             {
    //                 MacrosManager = nullptr;
    //                 UE_LOG(LogTemp, Error, TEXT("Failed to find MacrosManager field."));
    //             }
    //         }
    //         else
    //         {
    //             MacrosManager = nullptr;
    //             UE_LOG(LogTemp, Error, TEXT("Failed to find LifecycleInit field."));
    //         }
    //     }
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON."));
    //     UE_LOG(LogTemp, Warning, TEXT("JSON Raw: %s"), *JsonString);
    //     MacrosManager = nullptr;
    // }

    return JSONObject;
}

// The function takes a relative path to JSON file and returns a module as an object - hard-coded to a LifecycleInit module;
// --> Modules - top layers within the RSSInit.json array;
// --> Relative Path - subdirectory within the root directory of project;
TSharedPtr<FJsonObject> ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWriteBinary)
{
    FString RSSInitPath = FPaths::ProjectDir() + JSONSubPath;
    FString JsonString;
    
    if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    JsonArray.Empty();
    TSharedPtr<FJsonObject> ModuleAsObject = nullptr;

    if (ReadWriteBinary == 0)
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        FJsonSerializer::Deserialize(Reader, JsonArray);
        ModuleAsObject = JsonArray[1]->AsObject();
    }
    else if (ReadWriteBinary == 1)
    { 
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonArray, Writer);
        ModuleAsObject = JsonArray[1]->AsObject();
    }

    return ModuleAsObject;

    // for (const TSharedPtr<FJsonValue>& Item : JsonArray)
    // {
    //     TSharedPtr<FJsonObject> ModuleAsObject = Item->AsObject();
    // }
}

TArray<TSharedPtr<FJsonValue>> ThrowJsonArrayFromFile_UTIL(FString JSONSubPath)
{   
    FString RSSInitPath = FPaths::ProjectDir() + JSONSubPath;
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    JsonArray.Empty();
    FString JsonString;
    
    if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
        return JsonArray;
    }

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON - returning."));
        return JsonArray;
    }

    return JsonArray;

    // else if (ReadWriteBinary == 1)
    // { 
    //     TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    //     FJsonSerializer::Serialize(JsonArray, Writer);
    // }

    
}

TSharedPtr<FJsonObject> ThrowRSSInitModule_UTIL(TArray<TSharedPtr<FJsonValue>> JsonArray, FString RSSInitModule, FString RSSInitField)
{
    TSharedPtr<FJsonObject> RSSInitModule_AsObject = nullptr;
    TSharedPtr<FJsonObject> RSSInitField_AsObject = nullptr;


    if(JsonArray.Num() <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("JsonArray is empty - returning."));
        return nullptr;
    }

    RSSInitModule_AsObject = JsonArray[1]->AsObject();
    
    if (RSSInitModule_AsObject->HasField(RSSInitModule))
    {
        TSharedPtr<FJsonObject> LifecycleInit = RSSInitModule_AsObject->GetObjectField(RSSInitModule);
        if (LifecycleInit->HasField(RSSInitField))
        {
            RSSInitField_AsObject = LifecycleInit->GetObjectField(RSSInitField);

            // bool bIsInitialized = MacrosManager->GetBoolField(TEXT("bIsInitialized"));
            // MacrosManager_EXP->SetIsEnabled(bIsInitialized);

            // int32 SyncState = MacrosManager->GetIntegerField(TEXT("SyncState"));
            // SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(SyncState));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find JSONObject field."));
            return nullptr;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find %s (RSSInitModule) field."), *RSSInitModule);
        return nullptr; 
    }

    return RSSInitField_AsObject;
}

void SaveJsonArrayToFile_UTIL(const FString& JSONSubPath, const TArray<TSharedPtr<FJsonValue>>& JsonArray)
{
    FString OutputString;
    FString FullPath = FPaths::ProjectDir() + JSONSubPath;

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (!FJsonSerializer::Serialize(JsonArray, Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON - returning."));
        return ;
    }

    if (!FFileHelper::SaveStringToFile(OutputString, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to write JSON to file - returning."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Successfully saved JSON."));
}