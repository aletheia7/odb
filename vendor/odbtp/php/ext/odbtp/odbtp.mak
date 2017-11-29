# Module details
MODULE_NAME = php_odbtp
MODULE_DESC = "PHP $(PHP_VERSION_STR) - ODBTP $(VERSION_STR) Extension"

#include the common settings
include $(PROJECT_ROOT)/netware/common.mif

CP = cp -afv

# Source files
C_SRC = start.c \
        php_odbtp.c \
        odbtp.c \
        sockutil.c

CPP_SRC_NODIR = $(notdir $(CPP_SRC))
C_SRC_NODIR = $(notdir $(C_SRC))
SRC_DIR = $(dir $(CPP_SRC) $(C_SRC))

ifndef ODBTP_SDK
	ODBTP_SDK = ../../..
endif

# Library files
LIBRARY = 

# Destination directories and files
#OBJ_DIR = $(BUILD)_$(APACHE_VER)
#FINAL_DIR = $(OBJ_DIR)
OBJECTS  = $(addprefix $(OBJ_DIR)/,$(CPP_SRC_NODIR:.c=.obj) $(C_SRC_NODIR:.c=.obj))
DEPDS  = $(addprefix $(OBJ_DIR)/,$(CPP_SRC_NODIR:.c=.d) $(C_SRC_NODIR:.c=.d))

# Include the version info retrieved from the source files
-include $(OBJ_DIR)/version.inc

# Binary file
ifndef BINARY
	BINARY=$(FINAL_DIR)/$(MODULE_NAME).nlm
endif

# Compile flags
C_FLAGS += -DHAVE_ODBTP=1 -DCOMPILE_DL_ODBTP=1
ifdef ODBTP_MSSQL
C_FLAGS += -DODBTP_MSSQL=1
endif

C_FLAGS += -I. $(PHP_INCLUDES) $(NW_INCLUDES)
C_FLAGS += -I$(ODBTP_SDK)

# Hack to make the module export now uppercase
AWKCMD = awk 'BEGIN {print toupper(ARGV[1])}'
MODULE_EXPN := $(shell $(AWKCMD) $(MODULE_NAME))

# Dependencies
MODULE = LibC    \
         $(PHPNLM)

IMPORT = @$(SDK_DIR)/imports/libc.imp \
         @$(PHPIMP)

EXPORT = ($(MODULE_EXPN)) get_module
API =  OutputToScreen

# Virtual paths
vpath %.cpp .
vpath %.c . $(PROJECT_ROOT)/netware
vpath %.c $(ODBTP_SDK)
vpath %.obj $(OBJ_DIR)


all: prebuild project

.PHONY: all

prebuild:
	@if not exist $(OBJ_DIR) md $(OBJ_DIR)

project: $(BINARY)
ifdef ODBTP_MSSQL
	@$(CP) $(BINARY) $(basename $(BINARY))_mssql.nlm
endif
	@echo Build complete.

$(OBJ_DIR)/%.d: %.cpp
	@echo Building Dependencies for $(<F)
	@$(CC) -M $< $(C_FLAGS) -o $@

$(OBJ_DIR)/%.d: %.c
	@echo Building Dependencies for $(<F)
	@$(CC) -M $< $(C_FLAGS) -o $@

$(OBJ_DIR)/%.obj: %.cpp
	@echo Compiling $?...
	@$(CC) $< $(C_FLAGS) -o $@

$(OBJ_DIR)/%.obj: %.c
	@echo Compiling $?...
	@$(CC) $< $(C_FLAGS) -o $@

$(OBJ_DIR)/version.inc: $(ODBTP_SDK)/odbtp.h get_ver.awk prebuild
	@echo Creating $@
	@awk -f get_ver.awk $< > $@


#$(BINARY): $(DEPDS) $(OBJECTS)
$(BINARY): $(OBJECTS)
	@echo Import $(IMPORT) > $(basename $@).def
ifdef API
	@echo Import $(API) >> $(basename $@).def
endif
	@echo Module $(MODULE) >> $(basename $@).def
ifdef EXPORT
	@echo Export $(EXPORT) >> $(basename $@).def
endif
	@echo AutoUnload >> $(basename $@).def
ifeq '$(BUILD)' 'debug'
	@echo Debug >> $(basename $@).def
endif
	@echo Flag_On 0x00000008 >> $(basename $@).def
	@echo Start _LibCPrelude >> $(basename $@).def
	@echo Exit _LibCPostlude >> $(basename $@).def

	$(MPKTOOL) $(XDCFLAGS) $(basename $@).xdc
	@echo xdcdata $(basename $@).xdc >> $(basename $@).def

	@echo Linking $@...
	@echo $(LD_FLAGS) -commandfile $(basename $@).def > $(basename $@).link
	@echo $(PRELUDE) $(LIBRARY) $(OBJECTS) >> $(basename $@).link

	@$(LINK) @$(basename $@).link


#include the commontail settings
include $(PROJECT_ROOT)/netware/commontail.mif


