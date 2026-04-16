############################################
# Project
############################################

TARGET = f429_loop
BUILD = Build

############################################
# Toolchain
############################################

PREFIX = arm-none-eabi

CC = $(PREFIX)-gcc
AS = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
SIZE = $(PREFIX)-size
GDB = gdb-multiarch

############################################
# MCU
############################################

CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT = -mfloat-abi=hard

MCU = $(CPU) -mthumb $(FPU) $(FLOAT)

############################################
# Sources
############################################

C_SOURCES = $(shell find Core/Src -name "*.c") \
	    $(shell find Drivers/STM32F4xx_HAL_Driver/Src -name "*.c")

ASM_SOURCES = Core/Startup/startup_stm32f429zitx.s

############################################
# Includes
############################################

INCLUDES = -I./Core/Inc \
	   -I./Drivers/STM32F4xx_HAL_Driver/Inc \
	   -I./Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	   -I./Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	   -I./Drivers/CMSIS/Include 


############################################
# C flags
############################################

DEBUG	= -Wall -O0 -g3 -DDEBUG
DEFS	= -DUSE_HAL_DRIVER -DSTM32F429xx
FUNCT	= -ffunction-sections -fdata-sections -fstack-usage -fcyclomatic-complexity
DEPFLG	= -MMD -MP -MF $(@:.o=.d) -MT $@

CFLAGS = $(MCU) -std=gnu11 $(FUNCT) $(DEBUG) $(DEFS) $(INCLUDES)


############################################
# Linker
############################################

LDSCRIPT = STM32F429ZITX_FLASH.ld

LDFLAGS = -T$(LDSCRIPT) $(MCU) -Wl,--gc-sections -static \
	  --specs=nano.specs --specs=nosys.specs -Wl,-Map=$(BUILD)/$(TARGET).map

############################################
# Objects
############################################

OBJECTS = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))

DEPS = $(OBJECTS:.o=.d)

############################################
# Build rules
############################################

all: bin


############################################
# Compile C
############################################

$(BUILD)/%.o: Core/Src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: Drivers/STM32F4xx_HAL_Driver/Src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

############################################
# Compile ASM
############################################

$(BUILD)/%.o: Core/Startup/%.s
	$(AS) $(MCU) -c $< -o $@

############################################
# Link
############################################

$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

############################################
# Binary formats
############################################

bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex

############################################
# Flash (OpenOCD)
############################################

flash-ocd: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program $(BUILD)/$(TARGET).elf verify reset exit"
flash: bin
	st-flash --connect-under-reset write $(BUILD)/$(TARGET).bin 0x08000000

############################################
# Debug
############################################

debug:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

gdb:
	$(GDB) $(BUILD)/$(TARGET).elf

attach:
	$(GDB) -ex "target remote localhost:3333" $(BUILD)/$(TARGET).elf

readline: Core/Src/readline.c
	gcc -Wall -DEXECUTABLE -o $@ $<

crc: Core/Src/crc16.c
	gcc -Wall -DEXECUTABLE -o $@ $<

led: Core/Src/led.c
	gcc -Wall -DEXECUTABLE -o $@ $<


############################################
# Clean
############################################

clean:
	rm -rf $(BUILD)/* readline crc

############################################
# Dependencies
############################################

-include $(DEPS)

