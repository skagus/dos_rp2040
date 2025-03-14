
################ Top configuration.
PRJ_TOP = .
OBJDIR = out
TARGET = $(OBJDIR)/guest

OPTI = -O0
MCU = cortex-m3

VER_STRING := $$(date_time)_$(OPTI)

BUILD_STRICT = FALSE
BUILD_PRINT = TRUE

LINK_SCRIPT = arm.ld

################  Define
DEFINE =

# STM32F103C8T6 <-- MD performance.

################  Include.
# Add relative path from $(PRJ_TOP)
PRJ_INC = .

# Add absolue path. (ex. c:/lib/inc)
EXT_INC =

################  Library directory.
# Add relative path from $(PRJ_TOP)
PRJ_LIB_DIR =

# Add absolute path. (ex. c:/lib/)
EXT_LIB_DIR = 

LIB_FILE =

################ source files ##############
# Source file들은 project TOP 에서의 위치를 나타낸다.
CSRC = \
	main.c

CPPSRC =

ASRC =

DATA =

########## TARGET #############
all: elf bin lst hex size

test:
	echo $(VER_STRING)

include rules.mk
