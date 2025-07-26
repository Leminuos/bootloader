#include "nvram.h"
#include "spiflash.h"
#include "debug.h"
#include <string.h>

static uint8_t buffer[0x1000];

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

static nvram_state_t nvram_load(uint32_t offset, uint8_t* data, uint32_t size)
{
    uint32_t i = 0;
    uint32_t addr = 0;
    FLASH_RET_STS ret;

    for (i = 0; i < size; ++i)
    {
        addr = NVRAM_ADDRESS + offset + i;
        ret = W25QXX_ReadByte(addr, &data[i]);
        if (ret != FLASH_SUCCESS)
        {
            return NVRAM_BUFFER_EMPTY;
        }
    }
    
    return NVRAM_SUCCESS;
}

static nvram_state_t nvram_store(uint32_t offset, uint8_t* data, uint32_t size)
{
    uint32_t i = 0;
    uint32_t addr = 0;
    FLASH_RET_STS ret;

    W25QXX_EraseSector((NVRAM_ADDRESS + offset)/SECTOR_SIZE);

    for (i = 0; i < size; ++i)
    {
        addr = NVRAM_ADDRESS + offset + i;
        ret = W25QXX_WriteByte(addr, data[i]);
        if (ret != FLASH_SUCCESS)
        {
            return NVRAM_BUFFER_EMPTY;
        }
    }
    
    return NVRAM_SUCCESS;
}

static nvram_state_t header_integrity(nvram_header_t* header)
{
    uint32_t size = 0;
    uint32_t crc_expected = 0;

    if (memcmp(header->header, NVRAM_SIGNATURE, sizeof(header->header)) != 0)
        return NVRAM_UNSUPPORTED;

    crc_expected = CalculateCRC((uint8_t*)header, sizeof(nvram_header_t) - 4);

    if (header->checksum != crc_expected)
        return NVRAM_UNSUPPORTED;

    size = header->end_var - header->start_var;

    if ((size & 0x80000000UL) != 0)
        return NVRAM_OUT_OF_MEMORY;

    return NVRAM_SUCCESS;
}

static nvram_state_t var_integrity(nvram_var_t* var, uint32_t* total_var_size)
{
    uint32_t var_size_temp = 0;

    if (memcmp(var->signature, NVRAM_SIGNATURE, sizeof(var->signature)) != 0)
        return NVRAM_UNSUPPORTED;

    if ((var->var_attr & 0x07) == 0)
        return NVRAM_UNSUPPORTED;
    
    var_size_temp = sizeof(nvram_var_t) + var->var_name_size + var->var_length;
    if (var_size_temp + sizeof(uint32_t) > NVRAM_MAX_VAR_SIZE)
        return NVRAM_UNSUPPORTED;
    
    *total_var_size = var_size_temp;
    return NVRAM_SUCCESS;
}

static nvram_state_t find_variable(nvram_header_t* header, nvram_var_t* var_temp, char* var_name, uint32_t* var_offset)
{
    uint32_t spi_offset = 0;
    uint32_t total_var_size = 0;
    uint32_t crc_expected = 0;
    uint32_t crc_stored = 0;

    if (header->end_var == header->start_var)
        return NVRAM_UNSUPPORTED;
    
    spi_offset = header->start_var;
    while (spi_offset < header->end_var)
    {
        nvram_load(spi_offset, (uint8_t*)var_temp, sizeof(nvram_var_t));
        if (var_integrity(var_temp, &total_var_size) != NVRAM_SUCCESS)
            continue;
    
        nvram_load(spi_offset, buffer, total_var_size + sizeof(uint32_t));
        crc_expected = CalculateCRC(buffer, total_var_size);
        memcpy(&crc_stored, &buffer[total_var_size], sizeof(uint32_t));
        if (crc_expected != crc_stored)
            continue;

        if (var_temp->flag == FLAG_VALID)
        {
            if (memcmp(&buffer[sizeof(nvram_var_t)], var_name, var_temp->var_name_size) == 0)
            {
                *var_offset = spi_offset;
                return NVRAM_SUCCESS;
            }
        }
        
        spi_offset = spi_offset + total_var_size + sizeof(uint32_t);
    }

    return NVRAM_NOT_FOUND;
}

nvram_state_t nvram_init(uint8_t force)
{
    uint32_t checksum = 0;
    nvram_state_t  ret = NVRAM_SUCCESS;
    nvram_header_t nvram_header;

    memset(buffer, 0, sizeof(nvram_header));

    if (force == 0)
    {
        ret = nvram_load(0, buffer, sizeof(nvram_header));
        if (ret != NVRAM_SUCCESS)
            return ret;

        memcpy(&nvram_header, buffer, sizeof(nvram_header));
        if (memcmp(nvram_header.header, NVRAM_SIGNATURE, sizeof(nvram_header.header)))
            force = 1;

        checksum = CalculateCRC((uint8_t*)&nvram_header, sizeof(nvram_header) - 4);
        if (checksum != nvram_header.checksum)
            force = 1;
    }

    if (force == 1)
    {
        memcpy(nvram_header.header, NVRAM_SIGNATURE, sizeof(nvram_header.header));
        nvram_header.nvram_address = NVRAM_ADDRESS;
        nvram_header.nvram_size = NVRAM_SIZE;
        nvram_header.header_length = sizeof(nvram_header);
        nvram_header.start_var = sizeof(nvram_header);
        nvram_header.end_var = sizeof(nvram_header);
        nvram_header.reserved = 0xFFFFFFFF;
        nvram_header.checksum = CalculateCRC((uint8_t*)&nvram_header, sizeof(nvram_header) - 4);
        memcpy(buffer, &nvram_header, sizeof(nvram_header));
        ret = nvram_store(0, buffer, sizeof(nvram_header));
    }

    return ret;
}

nvram_state_t get_variable_data(char* var_name, uint8_t* data, uint32_t* data_len)
{
    uint32_t        var_offset = 0;
    uint32_t        var_data_offset = 0;
    nvram_var_t     var_temp = { 0 };
    nvram_header_t  nvram_header = { 0 };
    nvram_state_t   ret = NVRAM_UNSUPPORTED;

    memset(buffer, 0, SECTOR_SIZE);
    nvram_load(0, (uint8_t*)&nvram_header, sizeof(nvram_header_t));
    ret = header_integrity(&nvram_header);
    if (ret != NVRAM_SUCCESS)
        return ret;

    ret = find_variable(&nvram_header, &var_temp, var_name, &var_offset);
    if (ret != NVRAM_SUCCESS) return NVRAM_NOT_FOUND;
    var_data_offset = sizeof(nvram_var_t) + var_temp.var_name_size;
    *data_len = var_temp.var_length;
    memcpy(data, &buffer[var_data_offset], *data_len);

    return NVRAM_SUCCESS;
}

nvram_state_t set_variable_data(char* var_name, uint8_t var_attr, uint8_t* data, uint32_t data_len)
{
    uint8_t         diff_var_len = 0;
    uint32_t        crc_stored = 0;
    uint32_t        base_offset = 0;
    uint32_t        var_offset = 0;
    uint32_t        var_offset_sav = 0;
    uint32_t        store_data_len = 0;
    uint32_t        total_var_size = 0;
    nvram_var_t     var_temp = { 0 };
    nvram_header_t  nvram_header = { 0 };
    nvram_state_t   ret = NVRAM_UNSUPPORTED;

    memset(buffer, 0, SECTOR_SIZE);
    nvram_load(0, (uint8_t*)&nvram_header, sizeof(nvram_header_t));
    ret = header_integrity(&nvram_header);
    if (ret != NVRAM_SUCCESS) return ret;

    ret = find_variable(&nvram_header, &var_temp, var_name, &var_offset);
    if (ret == NVRAM_SUCCESS)
    {
        if (var_temp.var_attr == NVRAM_READONLY_TYPE)
            return NVRAM_UNSUPPORTED;
        
        store_data_len = var_temp.var_length;
        base_offset = (var_offset / SECTOR_SIZE) * SECTOR_SIZE;
        var_offset = var_offset % SECTOR_SIZE;
        total_var_size = var_temp.var_name_size + var_temp.var_length + sizeof(nvram_var_t);
        
        if (store_data_len != data_len)
        {
            diff_var_len = 1;
        }
        else
        {
            nvram_load(base_offset, buffer, SECTOR_SIZE);
            memcpy(&buffer[var_offset], &var_temp, sizeof(nvram_var_t));
            var_offset_sav = var_offset;
            var_offset = var_offset + sizeof(nvram_var_t) + var_temp.var_name_size;
            memcpy(&buffer[var_offset], data, data_len);
            var_offset = var_offset + data_len;
            crc_stored = CalculateCRC(&buffer[var_offset_sav], total_var_size);
            memcpy(&buffer[var_offset], &crc_stored, sizeof(uint32_t));
            nvram_store(base_offset, buffer, SECTOR_SIZE);
        }
    }
    
    if (ret != NVRAM_SUCCESS || diff_var_len == 1)
    {
        if (diff_var_len == 1)
        {
            nvram_load(base_offset, buffer, SECTOR_SIZE);
            var_temp.flag = FLAG_INVALID;
            memcpy(&buffer[var_offset], &var_temp, sizeof(nvram_var_t));
            crc_stored = CalculateCRC(&buffer[var_offset], total_var_size);
            var_offset = var_offset + total_var_size;
            memcpy(&buffer[var_offset], &crc_stored, sizeof(uint32_t));
            nvram_store(base_offset, buffer, SECTOR_SIZE);
        }

        memset(&var_temp, 0, sizeof(nvram_var_t));
        memcpy(var_temp.signature, NVRAM_SIGNATURE, sizeof(var_temp.signature));
        var_temp.var_name_size = strlen(var_name);
        var_temp.var_attr = var_attr;
        var_temp.var_length = data_len;
        var_temp.flag = FLAG_VALID;
        total_var_size = var_temp.var_name_size + var_temp.var_length + sizeof(nvram_var_t);

        base_offset = (nvram_header.end_var / SECTOR_SIZE) * SECTOR_SIZE;
        var_offset = nvram_header.end_var % SECTOR_SIZE;

        if (var_offset + total_var_size + sizeof(uint32_t) <= SECTOR_SIZE)
        {
            nvram_load(base_offset, buffer, SECTOR_SIZE);
            var_offset_sav = var_offset;
            memcpy(&buffer[var_offset], &var_temp, sizeof(nvram_var_t));
            var_offset = var_offset + sizeof(nvram_var_t);
            memcpy(&buffer[var_offset], var_name, var_temp.var_name_size);
            var_offset = var_offset + var_temp.var_name_size;
            memcpy(&buffer[var_offset], data, data_len);
            var_offset = var_offset + var_temp.var_length;
            crc_stored = CalculateCRC(&buffer[var_offset_sav], total_var_size);
            memcpy(&buffer[var_offset], &crc_stored, sizeof(uint32_t));
    
            if ((nvram_header.end_var / SECTOR_SIZE) == 0)
            {
                nvram_header.end_var = nvram_header.end_var + total_var_size + sizeof(uint32_t);
                nvram_header.checksum = CalculateCRC((uint8_t*)&nvram_header, sizeof(nvram_header_t) - sizeof(uint32_t));
                memcpy(buffer, &nvram_header, sizeof(nvram_header));
                nvram_store(base_offset, buffer, SECTOR_SIZE);
            }
            else
            {
                nvram_store(base_offset, buffer, SECTOR_SIZE);
                nvram_load(0, buffer, SECTOR_SIZE);
                nvram_header.end_var = nvram_header.end_var + total_var_size + sizeof(uint32_t);
                nvram_header.checksum = CalculateCRC((uint8_t*)&nvram_header, sizeof(nvram_header_t) - sizeof(uint32_t));
                memcpy(buffer, &nvram_header, sizeof(nvram_header));
                nvram_store(0, buffer, SECTOR_SIZE);
            }
        }
        else
        {

        }
    }

    return NVRAM_SUCCESS;
}

