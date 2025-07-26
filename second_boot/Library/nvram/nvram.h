#ifndef __NVRAM__
#define __NVRAM__

#include <stdint.h>

#define NVRAM_SIGNATURE         "NVFW"

#define NVRAM_ADDRESS           0x30000
#define NVRAM_SIZE              0x5000
#define NVRAM_MAX_VAR_SIZE      0x100

/* Variable attribute */
#define NVRAM_READONLY_TYPE     0x03
#define NVRAM_READWRITE_TYPE    0x04

/* Variable flag */
#define FLAG_VALID              0x11
#define FLAG_INVALID            0x22

/* Invalid đối với trường hợp cùng một biến nhưng kích thước khác nhau */

typedef enum {
    NVRAM_SUCCESS   = 0,
    NVRAM_NOT_FOUND,
    NVRAM_CORRUPT_DATA,
    NVRAM_UNSUPPORTED,
    NVRAM_BUFFER_EMPTY,
    NVRAM_ACCESS_DENIED,
    NVRAM_OUT_OF_MEMORY,
    NVRAM_INVALID_PARAMETER
} nvram_state_t;

typedef struct __attribute__((packed)) {
    uint8_t     header[4];
    uint32_t    nvram_address;
    uint32_t    nvram_size;
    uint32_t    header_length;
    uint32_t    start_var;
    uint32_t    end_var;
    uint32_t    reserved;
    uint32_t    checksum;
} nvram_header_t;

/*
 * +---------------+------------+--------+-------+
 * |  nvram_var_t  |  var_name  |  data  |  CRC  | 
 * +---------------+------------+--------+-------+
 */

typedef struct __attribute__((packed)) {
    uint8_t     signature[4];
    uint32_t    var_name_size;
    uint8_t     var_attr;
    uint32_t    var_length;
    uint8_t     flag;
} nvram_var_t;

nvram_state_t nvram_init(uint8_t force);
nvram_state_t get_variable_data(char* var_name, uint8_t* data, uint32_t* data_len);
nvram_state_t set_variable_data(char* var_name, uint8_t var_attr, uint8_t* data, uint32_t data_len);

#endif /* __NVRAM__ */