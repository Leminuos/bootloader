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
#define BOOT_REQ_CMD_RESET          0x03
#define BOOT_REQ_CMD_COMMON         0x80

/* Respond status */
#define BOOT_RES_SUCCESS            0x00
#define BOOT_RES_CRC_ERR            0x05
#define BOOT_RES_PARAM_ERR          0x06
#define BOOT_RES_LENGTH_ERR         0x07
#define BOOT_RES_WRITE_ERR          0x08
#define BOOT_RES_READ_ERR           0x09
#define BOOT_RES_ERASE_ERR          0x0A

#pragma pack(1)
typedef struct {
    UINT32    header;
    UINT8     command;
    UINT8     length;
    UINT16    reserved;
    UINT32    offset;
    UINT8     data[MAX_BOOT_BUFFER_SIZE];
    UINT32    crc;
}  BOOT_PACKET_REQ_T;

typedef struct {
    UINT32    header;
    UINT8     status;
    UINT8     length;
    UINT16    reserved;
    UINT8     data[MAX_BOOT_BUFFER_SIZE];
    UINT32    crc;
}  BOOT_PACKET_RES_T;
#pragma pack(pop)

extern VOID BootReset(VOID);
extern VOID BootMemErase(UINT32 Offset, UINT32 Size);
extern VOID BootMemRead(UINT32 Offset, UINT8* image, UINT32 Size);
extern VOID BootMemWrite(UINT32 Offset, UINT8* Image, UINT32 Size);

#endif /* __MEM_H__ */

