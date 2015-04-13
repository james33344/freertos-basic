TARGET = main
.DEFAULT_GOAL = all

CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
CFLAGS = -fno-common -O0 \
	 -std=c99 \
	 -gdwarf-2 -ffreestanding -g3 \
	 -mcpu=cortex-m3 -mthumb \
	 -Wall -Werror \
	 -Tmain.ld -nostartfiles \
	 -DUSER_NAME=\"$(USER)\"

ARCH = CM3
VENDOR = ST
PLAT = STM32F10x

LIBDIR = .
CODEBASE = freertos
CMSIS_LIB2 = $(CODEBASE)/libraries/CMSIS/$(ARCH)
CMSIS_LIB = $(CODEBASE)/libraries/CMSIS
STM32_LIB2 = $(CODEBASE)/libraries/STM32F10x_StdPeriph_Driver
CMSIS_LIB_DEVICE=$(CMSIS_LIB)/Device/ST/STM32F4xx
STM32_LIB=$(CODEBASE)/libraries/STM32F4xx_StdPeriph_Driver

CMSIS_PLAT_SRC = $(CMSIS_LIB2)/DeviceSupport/$(VENDOR)/$(PLAT)

FREERTOS_SRC = $(CODEBASE)/libraries/FreeRTOS
FREERTOS_INC = $(FREERTOS_SRC)/include/                                       
FREERTOS_PORT_INC = $(FREERTOS_SRC)/portable/GCC/ARM_$(ARCH)/

OUTDIR = build
SRCDIR = src\
         $(STM32_LIB)/src \
	 $(FREERTOS_SRC) 
#         $(CMSIS_PLAT_SRC) 
#         $(CMSIS_LIB2)/CoreSupport 
#         $(STM32_LIB2)/src \

INCDIR = include \
		 $(CMSIS_LIB)/Include \
         $(STM32_LIB)/inc \
		 $(CMSIS_LIB_DEVICE)/Include \
	 $(FREERTOS_INC) \
	 $(FREERTOS_PORT_INC) 
#         $(CMSIS_LIB2)/CoreSupport \
#         $(CMSIS_PLAT_SRC)
 #        $(STM32_LIB2)/inc 
INCLUDES = $(addprefix -I,$(INCDIR))
DATDIR = data
TOOLDIR = tool
TMPDiR = output

FIBSRC = fib.s

HEAP_IMPL = heap_ww
SRC = $(wildcard $(addsuffix /*.c,$(SRCDIR))) \
      $(wildcard $(addsuffix /*.s,$(SRCDIR))) \
      $(FREERTOS_SRC)/portable/MemMang/$(HEAP_IMPL).c \
      $(FREERTOS_SRC)/portable/GCC/ARM_CM3/port.c \
	  $(CMSIS_LIB_DEVICE)/Source/Templates/gcc_ride7/startup_stm32f429_439xx.s \
      $(CMSIS_LIB_DEVICE)/Source/Templates/system_stm32f4xx.c \
	  $(FIBSRC)
#      fib_r.s \
#	  fib_r2.s \
#      $(CMSIS_PLAT_SRC)/startup/gcc_ride7/startup_stm32f10x_md.s
OBJ := $(addprefix $(OUTDIR)/,$(patsubst %.s,%.o,$(SRC:.c=.o)))
DEP = $(OBJ:.o=.o.d)
DAT =
 

MAKDIR = mk
MAK = $(wildcard $(MAKDIR)/*.mk)

include $(MAK)


all: $(OUTDIR)/$(TARGET).bin $(OUTDIR)/$(TARGET).lst

$(OUTDIR)/$(TARGET).bin: $(OUTDIR)/$(TARGET).elf
	@echo "    OBJCOPY "$@
	@$(CROSS_COMPILE)objcopy -Obinary $< $@

$(OUTDIR)/$(TARGET).lst: $(OUTDIR)/$(TARGET).elf
	@echo "    LIST    "$@
	@$(CROSS_COMPILE)objdump -S $< > $@

$(OUTDIR)/$(TARGET).elf: $(OBJ) $(DAT) 
	@echo "    LD      "$@
	@echo "    MAP     "$(OUTDIR)/$(TARGET).map
	@$(CROSS_COMPILE)gcc $(CFLAGS) -Wl,-Map=$(OUTDIR)/$(TARGET).map -o $@ $^

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<

$(OUTDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<

clean:
	rm -rf $(OUTDIR) $(TMPDIR)
flash:
	st-flash write $(OUTDIR)/main.bin 0x8000000

run: 
	@echo " Debuggin..."
	@$(CROSS_COMPILE)gdb $(OUTDIR)/main.bin \
		-ex 'target remote :3333' \
		-ex 'monitor reset halt' \
		-ex 'load' \
		-ex 'monitor arm semihosting enable' \
		-ex 'continue'

rundbg:
	@echo " Debuggin..."
	@$(CROSS_COMPILE)gdb $(OUTDIR)/main.elf \
		-ex 'target remote :3333' \
		-ex 'monitor reset halt' \
		-ex 'load' \
		-ex 'monitor arm semihosting enable' \
	@echo " Start to dgb!!"


openocd:
	openocd -f board/stm32f4discovery.cfg


#openocd_flash:
#	openocd \
	-f board/stm32f429discovery.cfg \
	-c "init" \
	-c "reset init" \
	-c "flash probe 0" \
	-c "flash info 0" \
	-c "flash write_image erase $(OUTDIR)/main.elf  0x08000000" \
	-c "reset run" -c shutdown


-include $(DEP)



