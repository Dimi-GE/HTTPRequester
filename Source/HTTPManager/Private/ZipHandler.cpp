// ZIP Libraries
#include "ThirdParty/minizip-ng/include/mz.h"
#include "ThirdParty/minizip-ng/include/mz_os.h"
#include "ThirdParty/minizip-ng/include/mz_zip.h"
#include "ThirdParty/minizip-ng/include/mz_strm_os.h"
// File management
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// Forward Declared Functions
void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath);

void CreateZip(const TArray<FString>& FilePaths, const FString& ZipPath)
{
    void* FileStream = nullptr;
    void* ZipHandle = nullptr;
    
    // Step 1: Create the file stream
    mz_stream_os_create(&FileStream);
    mz_stream_os_set_filename(FileStream, TCHAR_TO_ANSI(*ZipPath));

    // Step 2: Open the file stream (write mode)
    if (mz_stream_open(FileStream, MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE) != MZ_OK)
    {
        // handle error
        UE_LOG(LogTemp, Error, TEXT("ZipHandler::Failed to open stream for writing - returning. . ."));
        return;
    }

    // Step 3: Create and open zip handle
    mz_zip_create(&ZipHandle);
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
        FString FileName = FPaths::GetCleanFilename(FilePath); // Only filename in archive
        void* FileEntryStream = nullptr;
        mz_stream_os_create(&FileEntryStream);
        mz_stream_os_set_filename(FileEntryStream, TCHAR_TO_ANSI(*FilePath));

        if (mz_stream_open(FileEntryStream, MZ_OPEN_MODE_READ) != MZ_OK)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZipHandler::Failed to open file %s - skipping..."), *FilePath);
            mz_stream_delete(&FileEntryStream);
            continue;
        }

        if (mz_zip_entry_open(ZipHandle, TCHAR_TO_ANSI(*FileName), MZ_COMPRESS_METHOD_DEFLATE, 9, 0) != MZ_OK)
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