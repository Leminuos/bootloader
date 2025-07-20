include Config.mak

ifdef TOOL_DIR
CC          = $(TOOL_DIR)/arm-none-eabi-gcc
AS			= $(TOOL_DIR)/arm-none-eabi-gcc -x assembler-with-cpp
CV     		= $(TOOL_DIR)/arm-none-eabi-objcopy
SZ			= $(TOOL_DIR)/arm-none-eabi-size
else
CC			= arm-none-eabi-gcc
AS			= arm-none-eabi-gcc -x assembler-with-cpp
CV			= arm-none-eabi-objcopy
SZ			= arm-none-eabi-size
endif

HEX 		= $(CV) -O ihex
BIN 		= $(CV) -O binary -S

# cpu
CPU 		= -mcpu=cortex-m3

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi

#MCU
MCU 		= $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# AS Defines
AS_DEFS = 

# C Defines
C_DEFS = 

# AS includes
AS_INCLUDES = 

# C Includes
INC_FILES   := $(foreach INC_DIR, $(INC_DIRS), $(wildcard $(INC_DIR)/*.h))
C_INCLUDES	:= $(foreach INC_DIR, $(INC_DIRS), -I$(INC_DIR))

# Compile flags
ASFLAGS     = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CCFLAGS		+= $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CCFLAGS 	+= -MMD -MP -MF"$(@:%.o=%.d)"

# Linker flags
LIBS 		= -lc -lm -lnosys 
LIBDIR 		= 
LDFLAGS		:= $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Object files
OBJ_FILES	= $(patsubst %.c, %.o, $(notdir $(SRC_FILES)))
vpath %.c $(SRC_DIRS)
OBJ_FILES   += $(patsubst %.s, %.o, $(notdir $(ASM_FILES)))
vpath %.s $(ASM_DIRS)
OBJ_PATHS	:= $(foreach OBJ_FILE, $(OBJ_FILES), $(BUILD_DIR)/$(OBJ_FILE))

build: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

rebuild: clean build

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CCFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) -o $@ $<

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR) 
	$(AS) -c $(CCFLAGS) -o $@ $<
 
$(BUILD_DIR)/$(TARGET).elf: $(OBJ_PATHS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

clean:
	rm -f Build/*

print-%:
	@echo $($(subst print-,,$@))

full-erase:
	$(STLINK_DIR)/ST-LINK_CLI.exe -ME

rst:
	$(STLINK_DIR)/ST-LINK_CLI.exe -Rst

program:
	$(STLINK_DIR)/ST-LINK_CLI.exe -P $(FILE_PATH) 0x08000000

upload: build full-erase program rst

help:
	$(STLINK_DIR)/ST-LINK_CLI.exe

# dependencies
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***

