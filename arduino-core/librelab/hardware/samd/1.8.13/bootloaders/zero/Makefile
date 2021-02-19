# Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.
# Copyright (c) 2015 Arduino LLC.  All right reserved.
# Copyright (c) 2017 Justin Mattair.  All right reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# Begin Config ###############################

BOARD_ID?=bigdot
MCU?=SAML21E18B

# End Config ###############################

NAME=sam_ba_$(BOARD_ID)_$(MCU)
FLASHSIZE=256KB
LINKER_SCRIPT?=bootloader_sam_m0p_$(FLASHSIZE).ld
MCPU=cortex-m0plus

# CORE_VENDOR?=arduino
CORE_VENDOR?=librelab

# -----------------------------------------------------------------------------
# Paths
ifeq ($(OS),Windows_NT)
  # Are we using mingw/msys/msys2/cygwin?
  ifeq ($(TERM),xterm)
    T=$(shell cygpath -u $(LOCALAPPDATA))
    MODULE_PATH?=$(T)/Arduino15/packages/$(CORE_VENDOR)
    MODULE_PATH_ARDUINO?=$(T)/Arduino15/packages/arduino
    RM=rm
    SEP=/
  else
    MODULE_PATH?=$(LOCALAPPDATA)/Arduino15/packages/$(CORE_VENDOR)
    MODULE_PATH_ARDUINO?=$(LOCALAPPDATA)/Arduino15/packages/arduino
    RM=del
    SEP=\\
  endif
else
  UNAME_S := $(shell uname -s)

  ifeq ($(UNAME_S),Linux)
    MODULE_PATH?=$(HOME)/.arduino15/packages/$(CORE_VENDOR)
    MODULE_PATH_ARDUINO?=$(HOME)/.arduino15/packages/arduino
    RM=rm
    SEP=/
  endif

  ifeq ($(UNAME_S),Darwin)
    MODULE_PATH?=$(HOME)/Library/Arduino15/packages/$(CORE_VENDOR)/
    MODULE_PATH_ARDUINO?=$(HOME)/Library/Arduino15/packages/arduino/
    RM=rm
    SEP=/
  endif
endif

PATHSEP=$(strip $(SEP))
ARM_GCC_PATH?=$(MODULE_PATH_ARDUINO)/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-
BUILD_DIR=build

# -----------------------------------------------------------------------------
# Tools
CC=$(ARM_GCC_PATH)gcc
OBJCOPY=$(ARM_GCC_PATH)objcopy
NM=$(ARM_GCC_PATH)nm
SIZE=$(ARM_GCC_PATH)size

# -----------------------------------------------------------------------------
# Compiler options
# eliminate PARANOIA define to save a tiny amount of code (~40B)
CFLAGS_EXTRA=-D__$(MCU)__ -DBOARD_ID_$(BOARD_ID) -DPARANOIA
CFLAGS=-mthumb -mcpu=$(MCPU) -Wall -c -std=gnu99 -ffunction-sections -fdata-sections -nostdlib -nostartfiles --param max-inline-insns-single=500 -Wa,-adhlns=$(<:%.c=$(BUILD_DIR)/%.lst)

ifdef DEBUG
  CFLAGS+=-g3 -O1 -DDEBUG=1
else
  CFLAGS+=-Os -DDEBUG=0
endif

ELF=$(NAME).elf
BIN=$(NAME).bin
HEX=$(NAME).hex


INCLUDES=-I"$(MODULE_PATH_ARDUINO)/tools/CMSIS/4.5.0/CMSIS/Include/" -I"$(MODULE_PATH)/tools/CMSIS-Atmel/1.0.0-mattairtech-2/CMSIS/Device/ATMEL/"

# -----------------------------------------------------------------------------
# Linker options
LDFLAGS=-mthumb -mcpu=$(MCPU) -Wall -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LDFLAGS+=-Wl,--warn-common -Wl,--warn-section-align -Wl,--warn-unresolved-symbols --specs=nano.specs --specs=nosys.specs

# -----------------------------------------------------------------------------
# Source files and objects
ifeq ($(findstring SAMC21,$(MCU)),SAMC21)
SOURCES= \
  board_driver_led.c \
  board_driver_serial.c \
  board_init.c \
  board_startup.c \
  util.c \
  main.c \
  sam_ba_monitor.c \
  sam_ba_serial.c

else
SOURCES= \
  board_driver_led.c \
  board_driver_serial.c \
  board_driver_usb.c \
  board_init.c \
  board_startup.c \
  util.c \
  main.c \
  sam_ba_usb.c \
  sam_ba_cdc.c \
  sam_ba_monitor.c \
  sam_ba_serial.c
endif

OBJECTS=$(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))
DEPS=$(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.d))

ifneq "test$(AVRSTUDIO_EXE_PATH)" "test"
  AS_BUILD=copy_for_atmel_studio
  AS_CLEAN=clean_for_atmel_studio
else
  AS_BUILD=
  AS_CLEAN=
endif


all: print_info $(SOURCES) $(BIN) $(HEX) $(AS_BUILD)

$(ELF): Makefile $(BUILD_DIR) $(OBJECTS)
	@echo ----------------------------------------------------------
	@echo Creating ELF binary
	"$(CC)" -L. -L$(BUILD_DIR) $(LDFLAGS) -Os -Wl,--gc-sections -save-temps -Tlinker_scripts/$(LINKER_SCRIPT) -Wl,-Map,"$(BUILD_DIR)/$(NAME).map" -o "$(BUILD_DIR)/$(ELF)" -Wl,--start-group $(OBJECTS) -lm -Wl,--end-group
	"$(NM)" "$(BUILD_DIR)/$(ELF)" >"$(BUILD_DIR)/$(NAME)_symbols.txt"
	"$(SIZE)" --format=sysv -t -x $(BUILD_DIR)/$(ELF)

$(BIN): $(ELF)
	@echo ----------------------------------------------------------
	@echo Creating flash binary
	"$(OBJCOPY)" -O binary $(BUILD_DIR)/$< $@

$(HEX): $(ELF)
	@echo ----------------------------------------------------------
	@echo Creating flash binary
	"$(OBJCOPY)" -O ihex $(BUILD_DIR)/$< $@

$(BUILD_DIR)/%.o: %.c
	@echo ----------------------------------------------------------
	@echo Compiling $< to $@
	"$(CC)" $(CFLAGS) $(CFLAGS_EXTRA) $(INCLUDES) $< -o $@
	@echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

$(BUILD_DIR):
	@echo ----------------------------------------------------------
	@echo Creating build folder
	-mkdir $(BUILD_DIR)

print_info:
	@echo ----------------------------------------------------------
	@echo Compiling bootloader using
	@echo BASE PATH = $(MODULE_PATH)
	@echo GCC  PATH = $(ARM_GCC_PATH)
#	@echo OS        = $(OS)
#	@echo SHELL     = $(SHELL)
#	@echo TERM      = $(TERM)
#	"$(CC)" -v
#	env

copy_for_atmel_studio: $(BIN) $(HEX)
	@echo ----------------------------------------------------------
	@echo Atmel Studio detected, copying ELF to project root for debug
	cp $(BUILD_DIR)$(PATHSEP)$(ELF) .

clean_for_atmel_studio:
	@echo ----------------------------------------------------------
	@echo Atmel Studio detected, cleaning ELF from project root
	-$(RM) .$(PATHSEP)$(ELF)

clean: $(AS_CLEAN)
	@echo ----------------------------------------------------------
	@echo Cleaning project
	-$(RM) $(BIN)
	-$(RM) $(HEX)
	-$(RM) $(BUILD_DIR)$(PATHSEP)*.*
	-rmdir $(BUILD_DIR)

mostly_clean: $(AS_CLEAN)
	@echo ----------------------------------------------------------
	@echo Cleaning project except bin
	-$(RM) $(HEX)
	-$(RM) $(BUILD_DIR)$(PATHSEP)*.*
	-rmdir $(BUILD_DIR)

.phony: print_info $(BUILD_DIR)
