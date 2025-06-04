// ZIP Libraries
#include "ThirdParty/minizip-ng/include/mz.h"
#include "ThirdParty/minizip-ng/include/mz_os.h"
#include "ThirdParty/minizip-ng/include/mz_zip.h"
#include "ThirdParty/minizip-ng/include/mz_strm.h"
#include "ThirdParty/minizip-ng/include/mz_strm_os.h"
// File management
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// Forward Declared Functions
void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath);
TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder);
void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath);
void UnpackZip(const FString& PathToZip, const FString& TempDir);

void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath)
{
    void* FileStream = mz_stream_os_create();

    // Step 1: Create the file stream
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Open the file stream with path directly
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Create and open zip handle
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_WRITE) != MZ_OK)
    {
        // handle error
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for writing - returning. . ."))
        return;
    }

    // Step 4: Add each file
    for (const FString& FilePath : FilePaths)
    {
        void* FileEntryStream = mz_stream_os_create();

        if (mz_stream_open(FileEntryStream, TCHAR_TO_UTF8(*FilePath), MZ_OPEN_MODE_READ) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open file %s - skipping..."), *FilePath);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        FString FileName = FPaths::GetCleanFilename(FilePath); // Only filename in archive

        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        FileMeta.filename = TCHAR_TO_ANSI(*FileName);
        FileMeta.modified_date = time(nullptr); // or use UE’s FDateTime if needed

        if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_STORE, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open zip entry for %s - skipping..."), *FilePath);
            mz_stream_close(FileEntryStream);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        const int32 BufferSize = 4096;
        uint8 Buffer[BufferSize];

        int32 BytesRead = 0;
        do
        {
            BytesRead = mz_stream_read(FileEntryStream, Buffer, BufferSize);
            if (BytesRead < 0) break;
            if (mz_zip_entry_write(ZipHandle, Buffer, BytesRead) != BytesRead) break;
        } while (BytesRead > 0);

        mz_zip_entry_close(ZipHandle);
        mz_stream_close(FileEntryStream);
        mz_stream_delete(&FileEntryStream);
    }

    // Step 5: Finalize and cleanup
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);

    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("ZipHandler::ZIP archive created successfully at %s"), *ZipPath);
}

TArray<TPair<FString, FString>> CollectFilesForZip_UTIL(const FString& RootFolder)
{
    TArray<TPair<FString, FString>> FilesToZip;

    // Normalize folder path
    FString NormalizedRoot = RootFolder;
    FPaths::NormalizeDirectoryName(NormalizedRoot);

    TArray<FString> FoundFiles;
    IFileManager::Get().FindFilesRecursive(
        FoundFiles,
        *NormalizedRoot,
        TEXT("*.*"),
        true,   // Files
        false   // Don't include directories
    );

    for (const FString& FilePath : FoundFiles)
    {
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *NormalizedRoot);
        FilesToZip.Add(TPair<FString, FString>(FilePath, RelativePath));
    }

    FilesToZip.Sort([](const TPair<FString, FString>& A, const TPair<FString, FString>& B)
    {
        return A.Value < B.Value; // Sort by relative path
    });

    return FilesToZip;
}

void CreateZip_Structured(TArray<TPair<FString, FString>> FilesStructure, const FString& ZipPath)
{
    void* FileStream = mz_stream_os_create();

    // Step 1: Create the file stream
    if (FileStream == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateZip: Failed to create file stream"));
        return;
    }

    // Open the file stream with path directly
    int32 Result = mz_stream_open(FileStream, TCHAR_TO_UTF8(*ZipPath), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE);
    if (Result != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file stream: %d"), Result);
        mz_stream_os_delete(&FileStream);
        return;
    }

    // Step 3: Create and open zip handle
    void* ZipHandle = mz_zip_create();
    if (mz_zip_open(ZipHandle, FileStream, MZ_OPEN_MODE_WRITE) != MZ_OK)
    {
        // handle error
        mz_stream_close(FileStream);
        mz_stream_delete(&FileStream);
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to create and open ZipHandle for writing - returning. . ."))
        return;
    }

    for (const TPair<FString, FString>& FilePair : FilesStructure)
    {
        const FString& FullPath = FilePair.Key;
        const FString& RelativePath = FilePair.Value;

        // UE_LOG(LogTemp, Warning, TEXT("Relative Path: %s; Full Path: %s.")*RelativePath, *FullPath);

        void* FileEntryStream = mz_stream_os_create();

        if (mz_stream_open(FileEntryStream, TCHAR_TO_UTF8(*FullPath), MZ_OPEN_MODE_READ) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open file %s - skipping..."), *FullPath);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        FTCHARToUTF8 EntryNameUTF8(*RelativePath);

        mz_zip_file FileMeta = {};
        FileMeta.version_madeby = MZ_VERSION_MADEBY;
        FileMeta.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        FileMeta.filename = EntryNameUTF8.Get();
        FileMeta.modified_date = time(nullptr); // or use UE’s FDateTime if needed

        if (mz_zip_entry_write_open(ZipHandle, &FileMeta, MZ_COMPRESS_METHOD_STORE, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open zip entry for %s - skipping..."), *FullPath);
            mz_stream_close(FileEntryStream);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        const int32 BufferSize = 4096;
        uint8 Buffer[BufferSize];

        int32 BytesRead = 0;
        do
        {
            BytesRead = mz_stream_read(FileEntryStream, Buffer, BufferSize);
            if (BytesRead < 0) break;
            if (mz_zip_entry_write(ZipHandle, Buffer, BytesRead) != BytesRead) break;
        } while (BytesRead > 0);

        mz_zip_entry_close(ZipHandle);
        mz_stream_close(FileEntryStream);
        mz_stream_delete(&FileEntryStream);
    }

    // Step 5: Finalize and cleanup
    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);

    mz_stream_close(FileStream);
    mz_stream_delete(&FileStream);

    UE_LOG(LogTemp, Log, TEXT("\nZipHandler::ZIP archive created successfully at %s"), *ZipPath);
}

void UnpackZip(const FString& PathToZip, const FString& TempDir)
{
    // Step 1: Create ZipHandle
    void* ZipHandle = mz_zip_create();
   
    if (mz_zip_open(ZipHandle, TCHAR_TO_UTF8(*PathToZip), MZ_OPEN_MODE_READ) != MZ_OK)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open ZIP file at %s"), *PathToZip);
        mz_zip_delete(&ZipHandle);
        return;
    }

    // Step 2:Loop through entries
    int32 EntryIndex = 0;
    while (mz_zip_goto_entry(ZipHandle, EntryIndex++) == MZ_OK)
    {
        mz_zip_file* FileInfo = nullptr;
        mz_zip_entry_get_info(ZipHandle, &FileInfo);
        FString EntryName = UTF8_TO_TCHAR(FileInfo->filename);

        FString AbsoluteDestPath = FPaths::Combine(TempDir, EntryName);

        // Step 3: Handle folder creation
        if (EntryName.EndsWith(TEXT("/")))
        {
            IFileManager::Get().MakeDirectory(*AbsoluteDestPath, true);
            continue;
        }

        IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsoluteDestPath), true);

        // Step 4: Open and read the entry
        if (mz_zip_entry_read_open(ZipHandle, 0, nullptr) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to open entry %s"), *EntryName);
            continue;
        }

        TArray<uint8> Buffer;
        Buffer.SetNumUninitialized(FileInfo->uncompressed_size);

        if (mz_zip_entry_read(ZipHandle, Buffer.GetData(), Buffer.Num()) != FileInfo->uncompressed_size)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to read entry %s"), *EntryName);
            mz_zip_entry_close(ZipHandle);
            continue;
        }

        FFileHelper::SaveArrayToFile(Buffer, *AbsoluteDestPath);
        mz_zip_entry_close(ZipHandle);
    }

    mz_zip_close(ZipHandle);
    mz_zip_delete(&ZipHandle);
}