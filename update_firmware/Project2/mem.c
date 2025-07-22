#include "mem.h"

#if SUPPORT_HID
#include "hid.h"
#define EP_MAX_SIZE                 HID_MAX_SIZE_REPORT
#elif SUPPORT_CDC
#include "cdc.h"
#define EP_MAX_SIZE                 CDC_MAX_SIZE_BUFFER
#endif

static UINT32 CalculateCRC(_In_ UINT8* data, _In_ UINT32 length)
{
    UINT32 i, j;
    UINT32 crc = 0xFFFFFFFF;

    for (i = 0; i < length; i++)
    {
        crc = crc ^ data[i];

        for (j = 0; j < 32; j++)
        {
            if (crc & 0x80000000)
            {
                crc = (crc << 1) ^ 0x04C11DB7;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

static void print_progress(int current, int max) {
    const int bar_width = 50;
    static int last_percent_displayed = -1;

    double percent = (double)current * 100.0 / max;
    int percent_int = (int)(percent * 100); // Làm tròn đến phần trăm x100 (ví dụ 4226)

    // Chỉ cập nhật nếu phần trăm thực sự thay đổi (giúp mượt hơn)
    if (percent_int == last_percent_displayed)
        return;

    last_percent_displayed = percent_int;

    int filled = (int)(percent * bar_width / 100.0);

    char bar[128];
    int n = snprintf(bar, sizeof(bar), "\r[");
    for (int i = 0; i < bar_width; ++i)
        n += snprintf(bar + n, sizeof(bar) - n, "%c", (i < filled) ? '#' : '-');
    snprintf(bar + n, sizeof(bar) - n, "] %6.2f%%", percent);

    printf("%s", bar);
    fflush(stdout);
}

static BOOLEAN SendRequest(UINT8 command, UINT32 address, UINT8 length, UINT8* data)
{
    BOOL result = FALSE;
    DWORD bytesWrite = 0;
    BOOT_PACKET_REQ_T request = { 0 };
    UINT8 buffer[EP_MAX_SIZE] = { 0 };

    memset(&request, 0, sizeof(request));
    request.header = 0x424C4443;
    request.command = command;
    request.length = length;
    request.address = address;
    memcpy(request.data, data, length);
    request.crc = CalculateCRC((UINT8*)&request, MAX_BOOT_REQUEST_SIZE - MAX_BOOT_CRC_SIZE);

    memcpy(buffer + 1, &request, MAX_BOOT_REQUEST_SIZE);
    result = WriteFile(hDevice, buffer, EP_MAX_SIZE, &bytesWrite, NULL);

#if DEBUG
    printf("Data write (%d byte): ", bytesWrite);
    for (DWORD i = 0; i < bytesWrite; ++i)
        printf("%02X ", buffer[i]);
    printf("\n");
#endif /* DEBUG */

    return result;
}

BOOLEAN ReceiveResponse(UINT8* data, UINT8 length)
{
    BOOL result = FALSE;
    DWORD bytesRead = 0;
    BOOT_PACKET_RES_T response = { 0 };
    UINT8 buffer[EP_MAX_SIZE] = { 0 };

    while (1) {
        result = ReadFile(hDevice, buffer, EP_MAX_SIZE, &bytesRead, NULL);

        if (result)
        {
#if DEBUG
            printf("Data received (%d byte): ", bytesRead);
            for (DWORD i = 0; i < bytesRead; ++i)
                printf("%02X ", buffer[i]);
            printf("\n");
#endif /* DEBUG */

            memcpy(&response, buffer + 1, sizeof(response));

            if (response.header == 0x424C4443)
            {
                if (response.status == BOOT_RES_ACK)
                {
                    if (data)
                    {
                        memcpy(data, response.data, length);
                    }

                    return TRUE;
                }
            }

            return FALSE;
        }
        else
        {
            printf("Data error: %lu\n", GetLastError());
            break;
        }
    }

    return FALSE;
}

VOID BootReset(VOID)
{
    printf("Reset system....\r\n");
    SendRequest(BOOT_REQ_CMD_RESET, 0, 0, NULL);
}

VOID BootMemAddress(UINT32 Address)
{
    SendRequest(BOOT_REQ_CMD_ADDRESS, Address, 0, NULL);
    ReceiveResponse(NULL, 0);
}

VOID BootMemErase(UINT32 Size)
{
    UINT32 cnt = 0;
    UINT32 address = 0;
    UINT32 pages = 0;
    BOOL result = FALSE;

    printf("Start erase at address 0x%08X\r\n", PROGRAM_START_ADDRESS);

    pages = Size / FLASH_PAGE_SIZE + 1;

    print_progress(0, pages - 1);

    for (UINT32 page = 0; page < pages; ++page)
    {
        cnt = 0;

        do
        {
            if (cnt == 10) return;
            ++cnt;
            result = SendRequest(BOOT_REQ_CMD_ERASE, page, 0, NULL);
            if (!result) continue;
            result = ReceiveResponse(NULL, 0);
        } while (!result);

        print_progress(page, pages - 1);
    }

    printf("\r\n\r\n");
}

VOID BootMemWrite(UINT8* image, UINT32 Size)
{
    UINT32 i = 0;
    UINT32 cnt = 0;
    UINT32 blocks = 0;
    UINT32 nonblock = 0;
    UINT32 address = 0;
    BOOL result = FALSE;

    printf("Start write program at address 0x%08X\r\n", PROGRAM_START_ADDRESS);

    blocks = Size / MAX_BOOT_BUFFER_SIZE;
    nonblock = Size % MAX_BOOT_BUFFER_SIZE;

    print_progress(0, blocks + 1);

    for (i = 0; i < blocks; ++i)
    {
        cnt = 0;
        address = PROGRAM_START_ADDRESS + MAX_BOOT_BUFFER_SIZE * i;

        do
        {
            if (cnt == 10) return;
            ++cnt;
            result = SendRequest(
                BOOT_REQ_CMD_WRITE,
                address,
                MAX_BOOT_BUFFER_SIZE,
                image + (MAX_BOOT_BUFFER_SIZE * i));
            if (!result) continue;
            result = ReceiveResponse(NULL, 0);
        } while (!result);

        print_progress(i, blocks + 1);
    }

    cnt = 0;
    address = PROGRAM_START_ADDRESS + MAX_BOOT_BUFFER_SIZE * blocks;

    do
    {
        if (cnt == 10) return;
        ++cnt;
        result = SendRequest(
            BOOT_REQ_CMD_WRITE,
            address,
            nonblock,
            image + (MAX_BOOT_BUFFER_SIZE * blocks));
        if (!result) continue;
        result = ReceiveResponse(NULL, 0);
    } while (!result);

    print_progress(blocks + 1, blocks + 1);
    printf("\r\n\r\n");
}

VOID BootMemRead(UINT8* image, UINT32 Size)
{
    UINT32 i = 0;
    UINT32 cnt = 0;
    UINT32 blocks = 0;
    UINT32 nonblock = 0;
    UINT32 address = 0;
    BOOL result = FALSE;

    printf("Start read program at address 0x%08X\r\n", PROGRAM_START_ADDRESS);

    blocks = Size / MAX_BOOT_BUFFER_SIZE;
    nonblock = Size % MAX_BOOT_BUFFER_SIZE;

    print_progress(0, blocks + 1);

    for (i = 0; i < blocks; ++i)
    {
        cnt = 0;
        address = PROGRAM_START_ADDRESS + MAX_BOOT_BUFFER_SIZE * i;

        do
        {
            if (cnt == 10) return;
            ++cnt;
            result = SendRequest(BOOT_REQ_CMD_READ, address, 0, NULL);
            if (!result) continue;
            result = ReceiveResponse(image + (MAX_BOOT_BUFFER_SIZE * i), MAX_BOOT_BUFFER_SIZE);
        } while (!result);

        print_progress(i, blocks + 1);
    }

    cnt = 0;
    address = PROGRAM_START_ADDRESS + MAX_BOOT_BUFFER_SIZE * blocks;

    do
    {
        if (cnt == 10) return;
        ++cnt;
        result = SendRequest(BOOT_REQ_CMD_READ, address, 0, NULL);
        if (!result) continue;
        result = ReceiveResponse(image + (MAX_BOOT_BUFFER_SIZE * blocks), nonblock);
    } while (!result);

    print_progress(blocks + 1, blocks + 1);
    printf("\r\n\r\n");
}
