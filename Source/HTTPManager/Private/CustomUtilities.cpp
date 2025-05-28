// Materials Interfaces
#include "Materials/MaterialInstanceDynamic.h"
// Engine
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
// File management
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
// Messaging
#include "Misc/MessageDialog.h"
// JSON
#include "Json.h"
#include "JsonUtilities.h"

// Forward Declared Functions
TSharedPtr<FJsonObject> ThrowRSSInitModule_RWUtil(FString JSONSubPath, int32 ReadWrite);
TArray<TSharedPtr<FJsonValue>> ThrowJsonArrayFromFile_UTIL(FString JSONSubPath);
FString CalculateFileHash(const FString& FilePath);
FString CalculateDirectoryHash(const TMap<FString, FString>& FileHashes);

// The function throws material instance dynamic - hard-coded to work M_SyncNotify so far;
UMaterialInstanceDynamic* ThrowDynamicInstance(float ScalarValue)
{
    static FSoftObjectPath MatPath(TEXT("/Game/Mats/UMG/M_SyncNotify.M_SyncNotify"));
    
    UMaterialInterface* BaseMat = Cast<UMaterialInterface>(MatPath.TryLoad());
    if (!BaseMat) return nullptr;

    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, nullptr);
    if (DynMat)
    {
        DynMat->SetScalarParameterValue(FName("SyncState"), ScalarValue);
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

TSharedPtr<FJsonObject> ThrowJsonObjectFromFile_UTIL(FString FilePath)
{
    // File manager initialization
    IFileManager& FileManager = IFileManager::Get();

    // Manifest placeholder
    TSharedPtr<FJsonObject> RSSManifestJSON = nullptr;
    
    // Path construction
    FString FullPath = FPaths::ProjectDir() + FilePath;
    
    // Array to hold the names of found files
    TArray<FString> FoundFiles;

    // Search for any files in the directory, excluding directories
    FileManager.FindFiles(FoundFiles, *FullPath, *FString("*.json*"));

    for(const FString& File : FoundFiles)
    {
        if (File.Equals(TEXT("RSSManifest.json"), ESearchCase::IgnoreCase))
        {
            FString FullFilePath = FullPath / File;
            FString FileContent;

            if (FFileHelper::LoadFileToString(FileContent, *FullFilePath))
            {
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
                FJsonSerializer::Deserialize(Reader, RSSManifestJSON);
            }
        }
    }

    return RSSManifestJSON;
}

FString OpenFolderDialog_UTIL()
{
    FString SelectedFolder;

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        void* ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
        
        // Set the default path if needed
        FString DefaultPath = FPaths::ProjectDir();
        
        // Open the folder dialog
        bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
            ParentWindowHandle,
            TEXT("Select a folder"),
            DefaultPath,
            SelectedFolder
        );
    }

    return SelectedFolder;
}

TSharedPtr<FJsonObject> ThrowNewJSONObject_UTIL( /* Root directory of the structure (Macros) */)
{
    /* TODO:
    1. Create base JSON object:
        TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
        TSharedPtr<FJsonObject> DescriptorObject = MakeShareable(new FJsonObject());
        RootObject->SetObjectField(TEXT("descriptor"), DescriptorObject);

    2. Open target directory with name and set as root (currently Macros):
        TSharedPtr<FJsonObject> Macros = MakeShareable(new  FJsonObject());
        FString MacrosName = FPaths::GetCleanFilename(OpenFolderDialog_UTIL());
        RootObject->SetObjectField(MacrosName, Macros);

    3. Set last local commit timestamp:
        Macros->SetStringField(TEXT("LastLocalCommit"), TEXT("timestamp"));

    4. Set the last GitHub commit timestamp:
        Macros->SetStringField(TEXT("LastGitHubCommit"), TEXT("timestamp"));

    5. Create Categories (or custom name) JSON object:
        TSharedPtr<FJsonObject> Categories = MakeShareable(new FJsonObject());
        FString CategoriesName = FPaths::GetCleanFilename(Get clean folder name);
        Macros->SetObjectField(CategoriesName, Categories);
    
    6. Find first internal directory:
        6.a Obtain subdirectory name from the full path;
        6.b Create new JSON object for the directory with name:
                TSharedPtr<FJsonObject> 1stDir = MakeShareable(new FJsonObject());
                FString 1stDirName = FPaths::GetCleanFilename(Get clean folder name);
                Categories->SetObjectField(1stDirName, 1stDir);
        6.c Calculate and set the Hash string field:
                hash = hash calculations;
                1stDir->SetStringField(TEXT("hash"), *hash.ToString());
        6.d Create Files(Content?..) JSON object:
                TSharedPtr<FJsonObject> Files = MakeShareable(new FJsonObject());
                1stDir->SetObjectField(TEXT("Files"), Files);


    7. Iterate through files within the folder, filling the structure:
        FString FileName = FPaths::GetCleanFilename(From file path);
        hash = hash calculations;
        Files->SetStringField(TEXT("FileName"), *hash.ToString());

    8. Save RSSManifest.json:
        
        8.a Serialize to string
                FString OutputString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
                FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

        8.b Ensure directory exists
                IFileManager::Get().MakeDirectory(*RSSPath, true);

        8.c Save to file
                FFileHelper::SaveStringToFile(OutputString, *ManifestPath);
    */

    return nullptr;
}

void RSSManifestInit_UTIL()
{

    // Declare defaults
    FString Directory = OpenFolderDialog_UTIL();
    FString SearchPattern = TEXT("*");
    TArray<FString> FoundFiles;
    TMap<FString, FString> FileHashes;

    IFileManager& FileManager = IFileManager::Get();

    // Retrieve file list
    FileManager.FindFilesRecursive(FoundFiles, *Directory, *SearchPattern, true, false);


    // Check if array is not empty - assumed to handle more then 1 macro
    int32 MacrosNum = FoundFiles.Num();
    if (MacrosNum <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("UTIL::RSSManifestInit::Failed to load files - returning"));
        return;
    }

    // Obtain full paths to reflect the macros (currently ignores possible subdirectories inside the root one)
    for (const FString& FilePath : FoundFiles)
    {

        FString FileHash = CalculateFileHash(FilePath);

        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *Directory);

        FString MacrosDir;
        FString RemainingPath;
        FString FileName;

        RelativePath.Split(TEXT("/"), &MacrosDir, &RemainingPath);
        RemainingPath.Split(TEXT("/"), &RemainingPath, &FileName);

        FString CategoryName = RemainingPath.Contains(TEXT("/")) ? RemainingPath.Left(RemainingPath.Find(TEXT("/"))) : RemainingPath;

        FileHashes.Add(FileName, FileHash);
        
        FString DirectoryHash = CalculateDirectoryHash(FileHashes);

        UE_LOG(LogTemp, Error, TEXT("\n CategoryName: %s; Haash: %s.\n"), *CategoryName, *DirectoryHash);
        UE_LOG(LogTemp, Error, TEXT("\n FileName: %s; Hash: %s.\n"), *FileName, *FileHash);
    }

    // FString RSSPath = FPaths::ProjectDir() / TEXT("RSS");
    // FString ManifestPath = RSSPath / TEXT("RSSManifest.json");
 
    // // Check if file already exists
    // if (!FPaths::FileExists(ManifestPath))
    // {
    //     // Create base JSON object
    //     TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    //     TSharedPtr<FJsonObject> DescriptorObject = MakeShareable(new FJsonObject());
    //     TSharedPtr<FJsonObject> StructureRoot = MakeShareable(new  FJsonObject());
    //     // TSharedPtr<FJsonObject> JSONObject = MakeShareable(new  FJsonObject());

    //     FString StructureRootName = FPaths::GetCleanFilename(OpenFolderDialog_UTIL());

    //     DescriptorObject->SetStringField(TEXT("RSSManifest"), TEXT("description"));
    //     StructureRoot->SetStringField(TEXT("LastLocalCommit"), TEXT("timestamp"));
    //     StructureRoot->SetStringField(TEXT("LastGitHubCommit"), TEXT("timestamp"));

    //     RootObject->SetObjectField(TEXT("descriptor"), DescriptorObject);
    //     RootObject->SetObjectField(StructureRootName, StructureRoot);

    //     // Serialize to string
    //     FString OutputString;
    //     TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    //     FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    //     // Ensure directory exists
    //     IFileManager::Get().MakeDirectory(*RSSPath, true);

    //     // Save to file
    //     FFileHelper::SaveStringToFile(OutputString, *ManifestPath);
    // }

}

FString CalculateFileHash(const FString& FilePath)
{
    // Read the file into a byte array
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load file: %s"), *FilePath);
        return TEXT("InvalidHash");
    }

    // Generate MD5 hash
    FMD5 Md5Gen;
    Md5Gen.Update(FileData.GetData(), FileData.Num());

    // Digest buffer
    uint8 Digest[16];
    Md5Gen.Final(Digest);

    // Convert digest to hex string
    FString HashString;
    for (int32 i = 0; i < 16; i++)
    {
        HashString += FString::Printf(TEXT("%02x"), Digest[i]);
    }

    return HashString;
}

FString CalculateDirectoryHash(const TMap<FString, FString>& FileHashes)
{
    // Step 1: Sort file names for consistent hash order
    TArray<FString> SortedKeys;
    FileHashes.GetKeys(SortedKeys);
    SortedKeys.Sort();

    // Step 2: Concatenate the hashes
    FString CombinedHashes;
    for (const FString& FileName : SortedKeys)
    {
        CombinedHashes += FileName + FileHashes[FileName]; // Optional: include filename for more uniqueness
    }

    // Step 3: Hash the concatenated string
    FTCHARToUTF8 Converter(*CombinedHashes);
    FMD5 Md5Gen;
    Md5Gen.Update((const uint8*)Converter.Get(), Converter.Length());

    uint8 Digest[16];
    Md5Gen.Final(Digest);

    // Step 4: Convert to hex string
    FString FinalHash;
    for (int32 i = 0; i < 16; i++)
    {
        FinalHash += FString::Printf(TEXT("%02x"), Digest[i]);
    }

    return FinalHash;
}