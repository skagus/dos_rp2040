
ifeq ($(BUILD_PRINT), TRUE)
	ECHO = 
else
	ECHO = @
endif

################ Generated Files ##############
VER_FILE = version.h
GEN_LDS = $(TARGET).lds

################ Object ##############
OBJ =	$(CSRC:%.c=$(OBJDIR)/%.c.o) \
		$(CPPSRC:%.cpp=$(OBJDIR)/%.p.o) \
		$(ASRC:%.S=$(OBJDIR)/%.s.o) \
		$(DATA:%.bin=$(OBJDIR)/%.b.o)

LST =	$(OBJ:%.o=%.lst)

DEP =	$(OBJ:%.o=%.d)

DEF_OPT = $(patsubst %,-D%,$(DEFINE))

LIB_OPT =	$(patsubst %,-L$(PRJ_TOP)/%,$(PRJ_LIB_DIR)) \
			$(patsubst %,-L%,$(EXT_LIB_DIR)) \
			$(patsubst %,-l%,$(LIB_FILE))

INCLUDE =	$(patsubst %,-I$(PRJ_TOP)/%,$(PRJ_INC)) \
			$(patsubst %,-I%,$(EXT_INC))

DEP_FLAGS = -MMD -MP # dependancy generation.

################  Common Options for C, C++, ASM
FLAGS  = -mcpu=$(MCU)
#FLAGS += -mthumb
FLAGS += -Wa,-adhlns=$(@:.o=.lst) # .o file assembly.
FLAGS += -gdwarf-2 # debug format: dwarf-2

## Optimize ##
FLAGS += $(OPTI)

## compiler configuration ##
FLAGS += -funsigned-char
FLAGS += -funsigned-bitfields
FLAGS += -fshort-enums
FLAGS += -fmessage-length=0 # error message in a line (w/o wrap)
FLAGS += -ffunction-sections
FLAGS += -fdata-sections 
#FLAGS += -fpack-struct  # CAUTION: RISC-V can't handle un-aligned load/save.
#FLAGS += -msave-restore 
#FLAGS += -fno-unit-at-a-time
#FLAGS += -mshort-calls
#FLAGS += --specs=nano.specs
FLAGS += --specs=nosys.specs
FLAGS += -nostdlib
FLAGS += -nolibc
FLAGS += -mfloat-abi=soft
## WARNING ##
FLAGS += -Wcomment # enable warning for cascade comment.
FLAGS += -Wunreachable-code
ifeq ($(BUILD_STRICT),TRUE)
	FLAGS += -Wall
	FLAGS += -Wextra
	FLAGS += -Wstrict-prototypes
	#CFLAGS += -Wundef # for undefined macro evaluation
endif

################  C Options
CFLAGS  = $(FLAGS)
CFLAGS += -std=gnu11 # c89, c99, gnu89, gnu99
CFLAGS += -fstack-usage  # show stack usage for each function.
#CFLAGS += -fcallgraph-info  # make call graph information.
CFLAGS += $(DEP_OPT)
CFLAGS += $(DEF_OPT)
CFLAGS += $(INCLUDE)
#CFLAGS += -fdump-tree-optimized #
#CFLAGS += -fdump-rtl-dfinish  # 
################  C++ Options
CPPFLAGS  = $(FLAGS)
CPPFLAGS += -std=gnu++14
CPPFLAGS += -fstack-usage  # show stack usage for each function.
CPPFLAGS += -fcallgraph-info  # make call graph information.
CPPFLAGS += -fno-exceptions
CPPFLAGS += -fno-rtti
CPPFLAGS += -fno-use-cxa-atexit
CPPFLAGS += $(DEP_OPT)
CPPFLAGS += $(DEF_OPT)
CPPFLAGS += $(INCLUDE)

################  Assembler Options
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
#  -listing-cont-lines: Sets the maximum number of continuation lines of hex 
#       dump that will be displayed for a given single line of source input.
ASFLAGS  = $(FLAGS)
#ASFLAGS += -Wa,-gstabs,--listing-cont-lines=100
ASFLAGS += $(DEP_OPT)
ASFLAGS += $(DEF_OPT)
ASFLAGS += $(INCLUDE)
ASFLAGS += -x assembler-with-cpp 

################  Linker Options
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDS_OPT = -T $(GEN_LDS)
#LDS_OPT = $(patsubst %, -T $(PRJ_TOP)/%, $(LINK_SCRIPT))

LDFLAGS = -g
LDFLAGS += -mcpu=$(MCU)
LDFLAGS += $(OPTI)
LDFLAGS += $(LDS_OPT)
LDFLAGS += $(LIB_OPT)
LDFLAGS += -Xlinker -Map=$(TARGET).map
LDFLAGS += -Xlinker --cref
#LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += --specs=nosys.specs
#LDFLAGS += --specs=nano.specs
LDFLAGS += -nostartfiles
LDFLAGS += -nodefaultlibs
LDFLAGS += -nolibc
LDFLAGS += -nostdlib
#LDFLAGS += -Xlinker --gc-sections
LDFLAGS += -Xlinker --print-memory-usage
#LDFLAGS += -mcmodel=medany
#LDFLAGS += -msmall-data-limit=8
LDFLAGS += -fmessage-length=0
LDFLAGS += -mfloat-abi=soft
#LDFLAGS += -Wl,--verbose

################### Action #################
CC 		= $(GNU_PREFIX)gcc
CPP 	= $(GNU_PREFIX)g++
OBJCOPY	= $(GNU_PREFIX)objcopy
OBJDUMP	= $(GNU_PREFIX)objdump
SIZE	= $(GNU_PREFIX)size
AR		= $(GNU_PREFIX)ar rcs
NM		= $(GNU_PREFIX)nm
GDB		= $(GNU_PREFIX)gdb

SHELL = sh
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp

INFO 	= @echo Making: $@
DIR_CHK = @mkdir -p $(@D)

################ Object file
$(OBJDIR)/%.b.o : $(PRJ_TOP)/%.bin
	$(INFO)
	$(DIR_CHK)
	$(OBJCOPY) -I binary -O elf32 $< $@

$(OBJDIR)/%.c.o : $(PRJ_TOP)/%.c
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(CC) -c $(CFLAGS) $(DEP_FLAGS) $< -o $@ 

$(OBJDIR)/%.p.o : $(PRJ_TOP)/%.cpp
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(CPP) -c $(CPPFLAGS) $(DEP_FLAGS) $< -o $@ 

$(OBJDIR)/%.s.o : $(PRJ_TOP)/%.S
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(CC) -c $(ASFLAGS) $(DEP_FLAGS) $< -o $@

################### File Creation #################

$(GEN_LDS): $(PRJ_TOP)/$(LINK_SCRIPT)
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(CC) -E -P -x c $(CFLAGS) $< > $@

$(PRJ_TOP)/$(VER_FILE): Makefile
	$(INFO)
	@echo "#define VERSION		\"$(VER_STRING)\" > $@

%.elf: $(OBJ) $(GEN_LDS)
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(CC) $(LDFLAGS) $(OBJ) $(LIB_OPT) -o $@ 

%.hex: %.elf
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(OBJCOPY) -O binary $< $@

%.lss: %.elf
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(OBJDUMP) -h -S -z -w $< > $@

%.sym: %.elf
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(NM) -S -l -n --format=sysv $< > $@

%.a: $(OBJ)
	$(INFO)
	$(DIR_CHK)
	$(ECHO)$(AR) $@ $^

################ Actions.
TARGET_ALL=	$(TARGET).elf $(TARGET).a \
			$(TARGET).hex $(TARGET).bin \
			$(TARGET).lss $(TARGET).sym $(TARGET).map

version:
	@echo "#define VERSION		\"$(VER_STRING)\"" > $(PRJ_TOP)/$(VER_FILE)

obj: $(OBJ)
lib: $(TARGET).a
elf: $(TARGET).elf
bin: $(TARGET).bin $(TARGET).hex
lst: $(TARGET).lss $(TARGET).sym

size: $(TARGET).elf
	$(INFO) opt: $(OPTI)
	@$(SIZE) --format=gnu $<
	@$(SIZE) --format=gnu $< > $(TARGET).size
	@$(SIZE) --format=sysv --radix=16 --common $< >> $(TARGET).size


# Display compiler version information.
gccversion : 
	$(ECHO)$(CC) --version

conf :
	@echo GNU_PREFIX : $(GNU_PREFIX)
	@echo ------------------------ Flags ------------------------
	@echo CFLAGS : $(CFLAGS)
	@echo LDFLAGS: $(LDFLAGS)
	@echo ASFLAGS: $(ASFLAGS)
	@echo LD FILE: $(LINK_SCRIPT)
	@echo ------------------------ Files ------------------------
	@echo ASM Files : $(ASMSRC)
	@echo C Files   : $(CSRC)
	@echo C++ Files : $(CPPSRC)

clean: clean_link
	$(REMOVEDIR) $(OBJDIR)

clean_link:
	$(REMOVE) $(TARGET).elf $(TARGET).a \
			$(TARGET).hex $(TARGET).bin \
			$(TARGET).lss $(TARGET).sym \
			$(TARGET).map $(TARGET).size \
			$(TARGET).lds

relink: clean_link all

# Include the dependency files.
-include $(patsubst %.c,$(OBJDIR)/%.c.d,$(CSRC))
-include $(patsubst %.cpp,$(OBJDIR)/%.p.d,$(CPPSRC))
-include $(patsubst %.S,$(OBJDIR)/%.S.d,$(ASRC))

# Listing of phony targets.
.PHONY : all obj elf bin hex lst size gccversion clean version
