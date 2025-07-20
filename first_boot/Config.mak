include HAL/Makefile
include APP/Makefile
include Core/Makefile
include Driver/Makefile
include Library/Makefile
include Bootloader/Makefile

TARGET    	= first_boot
# debug build?
DEBUG 		= 1
TOOL_DIR    = C:/GNU_Toolchain/bin
LDSCRIPT	= ./LinkerScript.ld
BUILD_DIR  	= ./Build
FILE_PATH	= ./Build/$(TARGET).hex
STLINK_DIR	= C:/"Program Files (x86)"/STMicroelectronics/"STM32 ST-LINK Utility"/"ST-LINK Utility"

# optimization
OPT 		= -O0