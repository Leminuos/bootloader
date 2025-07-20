#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "hid.h"
#include "mem.h"

#define MAX_COUNTER_FAIL    1000

VOID Usage(VOID);

int main(int argc, char** argv)
{
    int cnt = 0;

    if (argc < 3)
    {
        Usage();
        return 1;
    }

    printf("Please plug usb device");

    while (FindHidDevice() == HID_FAILED)
    {
        ++cnt;
        if (cnt == MAX_COUNTER_FAIL)
        {
            printf("\r\ntimeout\r\n");
            return 2;
        }
        printf(".");
        Sleep(20);
    }

    printf("\r\nUsb device found\r\n");

    if (strcmp(argv[1], "-f") == 0)
    {
        BootFlashImage(argv[2]);
    }

    CloseHandle(hDevice);

	return 0;
}

VOID Usage(VOID)
{
    printf("Usage: Bootloader.exe -f [bin-file]");
}

VOID BootFlashImage(CHAR* FileName)
{
    FILE* FileHandle                    = NULL;
    UINT8* WriteImage                   = NULL;
    UINT8* ReadImage                    = NULL;
    UINT32 FileSize                     = 0;
    UINT64 result                       = 0;

    /* Open file */
    FileHandle = fopen(FileName, "rb");

    if (FileHandle == NULL)
    {
        printf("Can't open file %s\r\n", FileName);
        exit(3);
    }

    /* File size */
    fseek(FileHandle, 0, SEEK_END);
    FileSize = ftell(FileHandle);
    rewind(FileHandle);

    printf("Open file %s successfully with file size: %d\r\n", FileName, FileSize);

    /* Allocate memory to contain the whole file */
    WriteImage = (UINT8*)malloc(sizeof(UINT8) * FileSize);
    ReadImage = (UINT8*)malloc(sizeof(UINT8) * FileSize);
    if (WriteImage == NULL || ReadImage == NULL)
    {
        printf("Memory error\r\n");
        fclose(FileHandle);
        exit(4);
    }

    // Copy the file into the buffer:
    result = fread(WriteImage, sizeof(UINT8), FileSize, FileHandle);
    if (result != FileSize)
    {
        printf("Reading error\r\n");
        fclose(FileHandle);
        exit(5);
    }

    /* Erase */
    BootMemErase(FileSize);

    /* Programming */
    BootMemWrite(WriteImage, FileSize);

    /* Verify */
    BootMemRead(ReadImage, FileSize);
    if (strcmp(ReadImage, WriteImage) == 0)
    {
        printf("Verify successfully\r\n");
        BootReset();
    }

    free(WriteImage);
    fclose(FileHandle);

    FileHandle = fopen("read_image.bin", "wb");
    fwrite(ReadImage, sizeof(UINT8), FileSize, FileHandle);
    fclose(FileHandle);
}

