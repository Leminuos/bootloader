#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"

#if SUPPORT_HID
#include "hid.h"
#elif SUPPORT_CDC
#include "cdc.h"
#endif

#define PROGRAM_START_ADDRESS       0
#define FILE_BIN_OUT                "read_image.bin"
#define MAX_COUNTER_FAIL            1000

VOID Usage(VOID);
VOID BootFlashImage(UINT32 Offset, CHAR* FileName);
VOID BootDumpImage(UINT32 Offset, UINT32 Size);

int main(int argc, char** argv)
{
    int cnt = 0;

    if (argc < 3)
    {
        Usage();
        return 1;
    }

    printf("Please plug usb device");

#if SUPPORT_HID
    while (FindHidDevice() == HID_FAILED)
#elif SUPPORT_CDC
    while (FindCdcDevice() == CDC_FAILED)
#endif /* SUPPORT_CDC */
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

#if SUPPORT_CDC
    if (ConfigComState() == CDC_FAILED)
    {
        printf("Config CDC failed\r\n");
        return 3;
    }
#endif /* SUPPORT_CDC */

    if (strcmp(argv[1], "-f") == 0)
    {
        BootFlashImage(PROGRAM_START_ADDRESS, argv[2]);
    }

    if (strcmp(argv[1], "-d") == 0)
    {
        BootDumpImage(strtol(argv[2], NULL, 0), strtol(argv[3], NULL, 0));
    }

    CloseHandle(hDevice);

	return 0;
}

VOID Usage(VOID)
{
    printf("Usage: Bootloader.exe -f [bin-file]");
}

VOID BootDumpImage(UINT32 Offset, UINT32 Size)
{
    FILE* FileHandle    = NULL;
    UINT8* ReadImage    = NULL;

    FileHandle = fopen(FILE_BIN_OUT, "wb");

    if (FileHandle == NULL)
    {
        printf("Can't open file %s\r\n", FILE_BIN_OUT);
        exit(3);
    }

    /* Allocate memory to contain the whole file */
    ReadImage = (UINT8*)malloc(sizeof(UINT8) * Size);
    if (ReadImage == NULL)
    {
        printf("Memory error\r\n");
        fclose(FileHandle);
        exit(4);
    }

    BootMemRead(Offset, ReadImage, Size);

    fwrite(ReadImage, sizeof(UINT8), Size, FileHandle);
    free(ReadImage);
    fclose(FileHandle);
}

VOID BootFlashImage(UINT32 Offset, CHAR* FileName)
{
    FILE* FileHandle    = NULL;
    UINT8* WriteImage   = NULL;
    UINT8* ReadImage    = NULL;
    UINT32 FileSize     = 0;
    UINT64 result       = 0;

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
    BootMemErase(Offset, FileSize);

    /* Programming */
    BootMemWrite(Offset, WriteImage, FileSize);

    /* Verify */
    BootMemRead(Offset, ReadImage, FileSize);
    if (strcmp(ReadImage, WriteImage) == 0)
    {
        printf("Verify successfully\r\n");
        BootReset();
    }

    free(WriteImage);
    fclose(FileHandle);

    FileHandle = fopen(FILE_BIN_OUT, "wb");
    if (FileHandle == NULL)
    {
        printf("Can't open file %s\r\n", FILE_BIN_OUT);
        exit(6);
    }
    fwrite(ReadImage, sizeof(UINT8), FileSize, FileHandle);
    free(ReadImage);
    fclose(FileHandle);
}

