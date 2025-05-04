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

void ThrowDialogMessage(FString Message)
{
    FText MsgTitle = FText::FromString(TEXT("Warning!"));
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString((TEXT("%s"), *Message)), &MsgTitle);
}

TSharedPtr<FJsonObject> ThrowRSSInitObject(FString JSONObject)
{
    FString RSSInitPath = FPaths::ProjectDir() + TEXT("\\RSS\\RSSInit.json");
    FString JsonString;

    if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
        TSharedPtr<FJsonObject> MacrosManager = nullptr;
        return MacrosManager;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    TSharedPtr<FJsonObject> MacrosManager;
    
    if (FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Item : JsonArray)
        {
            TSharedPtr<FJsonObject> RootObject = Item->AsObject();
            if (RootObject->HasField(TEXT("lifecycleinit")))
            {
                TSharedPtr<FJsonObject> LifecycleInit = RootObject->GetObjectField(TEXT("LifecycleInit"));
                if (LifecycleInit->HasField(TEXT("MacrosManager")))
                {
                    MacrosManager = LifecycleInit->GetObjectField(TEXT("MacrosManager"));
    
                    // bool bIsInitialized = MacrosManager->GetBoolField(TEXT("bIsInitialized"));
                    // MacrosManager_EXP->SetIsEnabled(bIsInitialized);

                    // int32 SyncState = MacrosManager->GetIntegerField(TEXT("SyncState"));
                    // SyncImage->SetBrushFromMaterial(ThrowDynamicInstance(SyncState));
                }
                else
                {
                    MacrosManager = nullptr;
                    UE_LOG(LogTemp, Error, TEXT("Failed to find MacrosManager field."));
                }
            }
            else
            {
                MacrosManager = nullptr;
                UE_LOG(LogTemp, Error, TEXT("Failed to find LifecycleInit field."));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON."));
        UE_LOG(LogTemp, Warning, TEXT("JSON Raw: %s"), *JsonString);
        MacrosManager = nullptr;
    }

    return MacrosManager;
}

TSharedPtr<FJsonObject> ThrowRSSInitROOT_RWUtil(FString JSONPath, int32 ReadWrite)
{
    FString RSSInitPath = FPaths::ProjectDir() + (("%s"), *JSONPath);
    FString JsonString;
    
    if (!FFileHelper::LoadFileToString(JsonString, *RSSInitPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file."));
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    JsonArray.Empty();
    TSharedPtr<FJsonObject> RootObject = nullptr;

    if (ReadWrite == 0)
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        FJsonSerializer::Deserialize(Reader, JsonArray);
        RootObject = JsonArray[0]->AsObject();
    }
    else if (ReadWrite == 1)
    { 
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonArray, Writer);
        RootObject = JsonArray[0]->AsObject();
    }

    return RootObject;

    // switch (ReadWrite)
    // {
    // case 0:
    //     TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    //     FJsonSerializer::Deserialize(Reader, JsonArray);
    //     RootObject = JsonArray[0]->AsObject();
    //     break;
    // case 1:
    //     TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    //     FJsonSerializer::Serialize(JsonArray, Writer);
    //     RootObject = JsonArray[0]->AsObject();
    //     break;
    
    // default:
    //     RootObject = nullptr;
    //     break;
    // }

    // for (const TSharedPtr<FJsonValue>& Item : JsonArray)
    // {
    //     TSharedPtr<FJsonObject> RootObject = Item->AsObject();
    // }
}