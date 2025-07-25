#ifndef __MEM_H__
#define __MEM_H__

#include <windows.h>
#include <stdio.h>

/* Config */
#define SUPPORT_HID                 1
#define SUPPORT_CDC                 0
#define DEBUG                       0

/* Size */
#define MAX_BOOT_BUFFER_SIZE        48
#define MAX_BOOT_REQUEST_SIZE       (MAX_BOOT_BUFFER_SIZE + 14)
#define MAX_BOOT_RESPONSE_SIZE      (MAX_BOOT_BUFFER_SIZE + 10)
#define MAX_BOOT_CRC_SIZE           4
#define HID_MAX_SIZE_REPORT         0x41
#define CDC_MAX_SIZE_BUFFER         0x41
#define FLASH_START_ADDRESS         0x08000000
#define FLASH_MAX_SIZE              0x10000
#define RAM_START_ADDRESS           0x20000000
#define RAM_SIZE_MAX                0x5000
#define FLASH_SECTOR_SIZE           4096

/* Request command */
#define BOOT_REQ_CMD_ERASE          0x00
#define BOOT_REQ_CMD_READ           0x01
#define BOOT_REQ_CMD_WRITE          0x02
#define BOOT_REQ_CMD_RESET          0xFE
#define BOOT_REQ_CMD_VERSION        0xFF

/* Respond status */
#define BOOT_RES_NACK               0x00
#define BOOT_RES_ACK                0x01
#define BOOT_RES_VALID              0x02

#pragma pack(1)
typedef struct {
    UINT32    header;
    UINT8     command;
    UINT32    address;
    UINT8     length;
    UINT8     data[MAX_BOOT_BUFFER_SIZE];
    UINT32    crc;
}  BOOT_PACKET_REQ_T;

typedef struct {
    UINT32    header;
    UINT8     status;
    UINT8     length;
    UINT8     data[MAX_BOOT_BUFFER_SIZE];
    UINT32    crc;
}  BOOT_PACKET_RES_T;
#pragma pack(pop)

extern VOID BootReset(VOID);
extern VOID BootMemErase(UINT32 Address, UINT32 Size);
extern VOID BootMemRead(UINT32 Address, UINT8* image, UINT32 Size);
extern VOID BootMemWrite(UINT32 Address, UINT8* Image, UINT32 Size);

#endif /* __MEM_H__ */

