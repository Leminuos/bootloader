#include "bootloader.h"
#include "string.h"
#include "stm32_driver_flash.h"
#include "stm32_driver_tim.h"
#include "stm32_driver_spi.h"
#include "stm32_hal_usb.h"
#include "spiflash.h"
#include "debug.h"
#include "uECC.h"
#include "sha-256.h"

static const char* TAG = STRING_TAG;
static firmware_info_t info;
static uint8_t spi_hash[HASH_MAX_LEN];
static uint8_t data_hash[HASH_MAX_LEN];
static uint8_t signature[SIG_MAX_LEN];
static uint8_t spi_buffer[BUFF_MAX_LEN];

typedef void (*app_entry_t)(void);

static uint32_t CalculateCRC(uint8_t* data, uint32_t length)
{
    uint32_t i, j;
    uint32_t crc = 0xFFFFFFFF;

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

static int compare_hash(uint8_t* str1, uint8_t* str2)
{
    for (uint8_t i = 0; i < HASH_MAX_LEN; ++i)
    {
        if (str1[i] != str2[i])
        {
            return 0;
        }
    }

    return 1;
}

static int fw_verify(uint32_t fw_address)
{
    int ret = 0;
    uint32_t addr = 0;
    uint32_t num_chunks = 0;
    uint32_t last_chunk_size = 0;
    struct Sha_256 sha_256;
    const struct uECC_Curve_t *curve = uECC_secp256r1();

    addr = fw_address;
    memset(data_hash, 0, sizeof(data_hash));
    memset(spi_hash, 0, sizeof(spi_hash));
    memset(signature, 0, sizeof(signature));

    W25QXX_ReadData(addr, (uint8_t*)&info, sizeof(info));
    DEBUG(LOG_INFO, TAG, "version: %d.%d", info.version_major, info.version_minor);
    DEBUG(LOG_INFO, TAG, "header size: %08X", info.header_size);
    DEBUG(LOG_INFO, TAG, "image size: %08X", info.image_size);
    DEBUG(LOG_INFO, TAG, "image address: %08X", info.image_address);
    DEBUG(LOG_INFO, TAG, "entry point: %08X", info.entry_point);

    calc_sha_256(data_hash, (const void*)&info, sizeof(info));
    addr = addr + sizeof(info);
    W25QXX_ReadData(addr, spi_hash, HASH_MAX_LEN);
    ret = compare_hash(spi_hash, data_hash);
    if (ret == 0)
    {
        DEBUG(LOG_ERROR, TAG, "Compare header hash failed");
        return ret;
    }

    addr = addr + HASH_MAX_LEN;
    W25QXX_ReadData(addr, signature, SIG_MAX_LEN);
    ret = uECC_verify(public_key, spi_hash, HASH_MAX_LEN, signature, curve);
    if (ret == 0)
    {
        DEBUG(LOG_ERROR, TAG, "Verify header signature failed");
        return ret;
    }

    sha_256_init(&sha_256, data_hash);
    num_chunks = info.image_size / BUFF_MAX_LEN;
    last_chunk_size = info.image_size % BUFF_MAX_LEN;
    addr = fw_address + info.image_address;

    while (num_chunks-- > 0)
    {
        memset(spi_buffer, 0, BUFF_MAX_LEN);
        W25QXX_ReadData(addr, spi_buffer, BUFF_MAX_LEN);
        sha_256_write(&sha_256, spi_buffer, BUFF_MAX_LEN);
        addr = addr + BUFF_MAX_LEN;
    }

    memset(spi_buffer, 0, BUFF_MAX_LEN);
    W25QXX_ReadData(addr, spi_buffer, last_chunk_size);
    sha_256_write(&sha_256, spi_buffer, last_chunk_size);
    addr = addr + last_chunk_size;
    (void)sha_256_close(&sha_256);
    W25QXX_ReadData(addr, spi_hash, HASH_MAX_LEN);
    ret = compare_hash(spi_hash, data_hash);
    if (ret == 0)
    {
        DEBUG(LOG_ERROR, TAG, "Compare image hash failed");
        return ret;
    }

    addr = addr + HASH_MAX_LEN;
    W25QXX_ReadData(addr, signature, SIG_MAX_LEN);
    ret = uECC_verify(public_key, spi_hash, HASH_MAX_LEN, signature, curve);
    if (ret == 0)
    {
        DEBUG(LOG_ERROR, TAG, "Verify header signature failed");
        return ret;
    }

    return ret;
}

void RAM_WriteProgram(uint32_t address, uint32_t size, uint8_t* src)
{
    uint32_t i = 0;
    volatile uint8_t* dst = (volatile uint8_t*) address;

    for (i = 0; i < size; ++i) *dst++ = *src++;
}

void RAM_ReadProgram(uint32_t address, uint32_t size, uint8_t* dst)
{
    uint32_t i = 0;
    volatile uint8_t* src = (volatile uint8_t*) address;

    for (i = 0; i < size; ++i) *dst++ = *src++;
}

static boot_state_t HandleHidRequest(uint8_t* data)
{
    uint32_t            i = 0;
    uint32_t            crc = 0;
    boot_packet_res_t   res = {0};
    boot_packet_req_t*  req = NULL;
    boot_state_t        state = BOOT_SUCCESS;

    req = (boot_packet_req_t*)data;
    memset(&res, 0, sizeof(res));
    res.header = req->header;

    // Check header
    if (req->header != BOOT_HEADER)
    {
        res.status = BOOT_RES_NACK;
        res.length = 0;
        state = INVALID_HEADER_ERR;
        goto end;
    }

    // Check length
    if (req->length > MAX_BOOT_BUFFER_SIZE)
    {
        res.status = BOOT_RES_NACK;
        res.length = 0;
        state = INVALID_LENGTH_ERR;
        goto end;
    }

    // Check CRC
    crc = CalculateCRC(data, MAX_BOOT_REQUEST_SIZE - MAX_BOOT_CRC_SIZE);
    if (crc != req->crc)
    {
        res.status = BOOT_RES_NACK;
        res.length = 0;
        state = INVALID_DATA_ERR;
        goto end;
    }

    switch (req->command)
    {    
    case BOOT_REQ_CMD_ERASE:
        W25QXX_EraseSector(req->address);
        res.status = BOOT_RES_ACK;
        res.length = 0;

        break;

    case BOOT_REQ_CMD_READ:
        for (i = 0; i < MAX_BOOT_BUFFER_SIZE; ++i) W25QXX_ReadByte(req->address + i, &res.data[i]);
        res.status = BOOT_RES_ACK;
        res.length = MAX_BOOT_BUFFER_SIZE;

        break;

    case BOOT_REQ_CMD_WRITE:
        for (i = 0; i < req->length; ++i) W25QXX_WriteByte(req->address + i, req->data[i]);
        res.status = BOOT_RES_ACK;
        res.length = 0;
        
        break;

    case BOOT_REQ_CMD_RESET:
        NVIC_SystemReset();
        break;
    
    case BOOT_REQ_CMD_VERSION:
        break;
    
    default:
        break;
    }

end:
    memset(data, 0, HID_MAX_SIZE_REPORT);
    memcpy(data, &res, sizeof(res));
    return state;
}

void UpdateFirmware(void)
{
    uint8_t data[HID_MAX_SIZE_REPORT];
    boot_state_t state = BOOT_SUCCESS;

    // Enable timer
    TIM2->DIER.BITS.UIE = 0x01;
    TIM2->CR1.BITS.CEN = 0x01;

    DEBUG(LOG_DEBUG, TAG, "Update firmware");

    // Process bootloader commands
    while (1)
    {
        // Receive data from USB
        if (HID_ReceiveReport(data) > 0)
        {
            state = HandleHidRequest(data);

            if (state == BOOT_SUCCESS)
            {
                // Send response back to host
                HID_SendReport(data);
            }
        }
    }
}

int LoadFirmware(uint32_t fw_address)
{
    uint32_t page = 0;
    uint32_t start_page = 0;
    uint32_t spi_addr = 0;
    uint32_t chip_addr = 0;
    uint32_t num_pages = 0;
    uint32_t last_num_pages = 0;

    if (fw_verify(fw_address) == 0)
    {
        return 0;
    }

    DEBUG(LOG_INFO, TAG, "Verify successfully");

    num_pages = info.image_size / BUFF_MAX_LEN;
    last_num_pages = info.image_size % BUFF_MAX_LEN;
    spi_addr = fw_address + info.image_address;
    chip_addr = info.entry_point;

    if (chip_addr >= FLASH_START_ADDRESS && chip_addr <= (FLASH_START_ADDRESS + FLASH_MAX_SIZE))
    {
        start_page = (chip_addr - FLASH_START_ADDRESS) / BUFF_MAX_LEN;
        DEBUG(LOG_DEBUG, TAG, "Load firmware into flash from page %d", start_page);

        for (page = start_page; page <= (num_pages + start_page); ++page)
            FLash_PageErase(page);
    
        for (page = 0; page < num_pages; ++page)
        {
            memset(spi_buffer, 0, BUFF_MAX_LEN);
            W25QXX_ReadData(spi_addr, spi_buffer, BUFF_MAX_LEN);
            spi_addr = spi_addr + BUFF_MAX_LEN;
            Flash_WriteProgram(chip_addr, BUFF_MAX_LEN, spi_buffer);
            chip_addr = chip_addr + BUFF_MAX_LEN;
        }

        memset(spi_buffer, 0, BUFF_MAX_LEN);
        W25QXX_ReadData(spi_addr, spi_buffer, last_num_pages);
        Flash_WriteProgram(chip_addr, last_num_pages, spi_buffer);
    }
    else if (chip_addr >= RAM_START_ADDRESS && chip_addr <= (RAM_START_ADDRESS + RAM_SIZE_MAX))
    {
        DEBUG(LOG_DEBUG, TAG, "Load firmware into ram");

        for (page = 0; page < num_pages; ++page)
        {
            memset(spi_buffer, 0, BUFF_MAX_LEN);
            W25QXX_ReadData(spi_addr, spi_buffer, BUFF_MAX_LEN);
            spi_addr = spi_addr + BUFF_MAX_LEN;
            RAM_WriteProgram(chip_addr, BUFF_MAX_LEN, spi_buffer);
            chip_addr = chip_addr + BUFF_MAX_LEN;
        }

        memset(spi_buffer, 0, BUFF_MAX_LEN);
        W25QXX_ReadData(spi_addr, spi_buffer, last_num_pages);
        RAM_WriteProgram(chip_addr, last_num_pages, spi_buffer);
    }
    else
    {
        DEBUG(LOG_ERROR, TAG, "Entry point failed");
    }

    return 1;
}

void JumpToApplication(void)
{
    DEBUG(LOG_DEBUG, TAG, "Jump to address -> 0x%08X", info.entry_point);

    delay(20);

    /* Disable all interrupt */
    __disable_irq();

    /* Reset peripheral register */
    RCC->APB1RSTR.REG = 0xFFFFFFFF;
    RCC->APB1RSTR.REG = 0x00000000;

    RCC->APB2RSTR.REG = 0xFFFFFFFF;
    RCC->APB2RSTR.REG = 0x00000000;

    /* Turn off System tick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;

    /* Clear Pending Interrupt Request*/
    SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);
    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* Set main stack pointer */
    __set_MSP(*((volatile uint32_t*) info.entry_point));

    __DMB();
    SCB->VTOR = info.entry_point;
    __DSB();

    /* Jump to application */
    app_entry_t reset_handle = (app_entry_t)(*(volatile uint32_t*) (info.entry_point + 4));
    
    __enable_irq();
    
    reset_handle();
}

void BootloaderInit(void)
{
    uint8_t id[2];

    // Initialize USB
    USB_PowerOnReset();

    // Init timer
    TimerConfig();

    SPI_Init(SPI2);

    W25QXX_GetDeviceID(id);

    DEBUG(LOG_INFO, TAG, "Flash ID: 0x%02X%02X", id[0], id[1]);
}
