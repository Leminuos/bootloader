#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include <stm32f103.h>

/* BLDC - Bootloader Device Controller */
#define BOOT_HEADER                 0x424C4443
#define FLASH_START_ADDRESS         0x08000000
#define FLASH_MAX_SIZE              0x10000
#define RAM_START_ADDRESS           0x20000000
#define RAM_SIZE_MAX                0x5000
#define MAX_BOOT_BUFFER_SIZE        48

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

#define HASH_MAX_LEN                32
#define SIG_MAX_LEN                 64
#define PUB_MAX_LEN                 64
#define BUFF_MAX_LEN                1024
#define SUPPORT_PUBLIC_KEY          0
#define STRING_TAG                  "first boot"

typedef struct __attribute__((packed)) {
    uint32_t    header;
    uint8_t     command;
    uint8_t     length;
    uint16_t    reserved;
    uint32_t    offset;
    uint8_t     data[MAX_BOOT_BUFFER_SIZE];
    uint32_t    checksum;
}  boot_packet_req_t;

typedef struct __attribute__((packed)) {
    uint32_t    header;
    uint8_t     status;
    uint8_t     length;
    uint16_t    reserved;
    uint8_t     data[MAX_BOOT_BUFFER_SIZE];
    uint32_t    checksum;
} boot_packet_res_t;

typedef struct {
    uint32_t header;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t use_pubkey;
    uint32_t header_size;
    uint32_t image_size;
    uint32_t image_address;
    uint32_t entry_point;
} firmware_info_t;

#if SUPPORT_PUBLIC_KEY
extern const uint8_t public_key_temp[];
#endif /* SUPPORT_PUBLIC_KEY */

extern void BootloaderInit(void);
extern void UpdateFirmware(void);
extern void JumpToApplication(void);
extern int LoadFirmware(uint32_t fw_address);

#endif /* __BOOTLOADER_H__ */

