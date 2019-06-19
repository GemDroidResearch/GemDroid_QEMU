# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# this turns off the suffix rules built into make
.SUFFIXES:

# this turns off the RCS / SCCS implicit rules of GNU Make
% : RCS/%,v
% : RCS/%
% : %,v
% : s.%
% : SCCS/s.%

# If a rule fails, delete $@.
.DELETE_ON_ERROR:

# shared definitions
ifeq ($(strip $(SHOW)$(V)),)
define pretty
@echo $1
endef
hide := @
else
define pretty
endef
hide :=
endif

# Return the parent directory of a given path.
# $1: path
parent-dir = $(dir $1)

# Return the directory of the current Makefile / Android.mk.
my-dir = $(call parent-dir,$(lastword $(MAKEFILE_LIST)))

# Return the directory containing the intermediate files for a given
# kind of executable
# $1 = type (EXECUTABLES, STATIC_LIBRARIES or SHARED_LIBRARIES).
# $2 = module name
# $3 = ignored
#
intermediates-dir-for = $(OBJS_DIR)/intermediates/$(2)

# Return the name of a given host tool, based on the value of
# LOCAL_HOST_BUILD. If the variable is defined, return $(BUILD_$1),
# otherwise return $(HOST_$1).
# $1: Tool name (e.g. CC, LD, etc...)
#
local-host-tool = $(if $(strip $(LOCAL_HOST_BUILD)),$(BUILD_$1),$(MY_$1))
local-host-exe = $(call local-host-tool,EXEEXT)
local-host-dll = $(call local-host-tool,DLLEXT)

local-host-define = $(if $(strip $(LOCAL_$1)),,$(eval LOCAL_$1 := $$(call local-host-tool,$1)))

# Return the directory containing the intermediate files for the current
# module. LOCAL_MODULE must be defined before calling this.
local-intermediates-dir = $(OBJS_DIR)/intermediates/$(LOCAL_MODULE)

# Return $1, except if LOCAL_MODULE_BITS is 64, where $2 is returned.
local-bits-choice = $(strip $(if $(filter 64,$(LOCAL_MODULE_BITS)),$2,$1))

local-library-path = $(OBJS_DIR)/$(call local-bits-choice,libs,libs64)/$(1).a
local-executable-path = $(OBJS_DIR)/$(1)$(call local-host-tool,EXEEXT)
local-shared-library-path = $(OBJS_DIR)/$(call local-bits-choice,lib,lib64)/$(1)$(call local-host-tool,DLLEXT)

# Expand to a shell statement that changes the runtime library search path.
# Note that this is only used for Qt-related stuff, and on Windows, the
# Windows libraries are placed under bin/ instead of lib/ so there is no
# point in changing the PATH variable.
set-host-library-search-path = $(call set-host-library-search-path-$(HOST_OS),$1)
set-host-library-search-path-linux = LD_LIBRARY_PATH=$1
set-host-library-search-path-darwin = DYLD_LIBRARY_PATH=$1
set-host-library-search-path-windows =

# Toolchain control support.
# It's possible to switch between the regular toolchain and the host one
# in certain cases.

# Compile a C source file
#
define  compile-c-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(SRC:%.c=%.o)
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile a C++ source file
#
define  compile-cxx-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(SRC:%$(LOCAL_CPP_EXTENSION)=%.o)
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CXX    := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CXX) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile an Objective-C source file
#
define  compile-objc-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%.m=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(LOCAL_PATH)/$$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(LOCAL_PATH)/$$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Compile a generated C source files#
#
define compile-generated-c-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%.c=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CC     := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

define compile-generated-cxx-source
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%$$(LOCAL_CPP_EXTENSION)=%.o))
LOCAL_OBJECTS += $$(OBJ)
DEPENDENCY_DIRS += $$(dir $$(OBJ))
$$(OBJ): PRIVATE_CFLAGS := $$(LOCAL_CFLAGS) -I$$(LOCAL_PATH) -I$$(LOCAL_OBJS_DIR)
$$(OBJ): PRIVATE_CXX    := $$(LOCAL_CC)
$$(OBJ): PRIVATE_OBJ    := $$(OBJ)
$$(OBJ): PRIVATE_MODULE := $$(LOCAL_MODULE)
$$(OBJ): PRIVATE_SRC    := $$(SRC)
$$(OBJ): PRIVATE_SRC0   := $$(SRC)
$$(OBJ): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile: $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC0)"
	$(hide) $$(PRIVATE_CXX) $$(PRIVATE_CFLAGS) -c -o $$(PRIVATE_OBJ) -MMD -MP -MF $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_SRC)
	$(hide) $$(BUILD_SYSTEM)/mkdeps.sh $$(PRIVATE_OBJ) $$(PRIVATE_OBJ).d.tmp $$(PRIVATE_OBJ).d
endef

# Install a file
#
define install-target
SRC:=$(1)
DST:=$(2)
$$(DST): PRIVATE_SRC := $$(SRC)
$$(DST): PRIVATE_DST := $$(DST)
$$(DST): PRIVATE_DST_NAME := $$(notdir $$(DST))
$$(DST): PRIVATE_SRC_NAME := $$(SRC)
$$(DST): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Install: $$(PRIVATE_DST_NAME) <= $$(PRIVATE_SRC_NAME)"
	$(hide) cp -f $$(PRIVATE_SRC) $$(PRIVATE_DST)
install: $$(DST)
endef

# for now, we only use prebuilt SDL libraries, so copy them
define copy-prebuilt-lib
_SRC := $(1)
_SRC1 := $$(notdir $$(_SRC))
_DST := $$(LIBS_DIR)/$$(_SRC1)
LIBRARIES += $$(_DST)
$$(_DST): PRIVATE_DST := $$(_DST)
$$(_DST): PRIVATE_SRC := $$(_SRC)
$$(_DST): $$(_SRC)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Prebuilt: $$(PRIVATE_DST)"
	$(hide) cp -f $$(PRIVATE_SRC) $$(PRIVATE_DST)
endef

define  create-dir
$(1):
	mkdir -p $(1)
endef

define transform-generated-source
@echo "Generated: $(PRIVATE_MODULE) <= $<"
@mkdir -p $(dir $@)
$(hide) $(PRIVATE_CUSTOM_TOOL)
endef

# Generate DLL symbol file
#
# NOTE: The file is always named foo.def
#
define generate-symbol-file
SRC:=$(1)
OBJ:=$$(LOCAL_OBJS_DIR)/$$(notdir $$(SRC:%.entries=%.def))
LOCAL_GENERATED_SYMBOL_FILE:=$$(OBJ)
$$(OBJ): PRIVATE_SRC := $$(SRC)
$$(OBJ): PRIVATE_DST := $$(OBJ)
$$(OBJ): PRIVATE_MODE := $$(GEN_ENTRIES_MODE_$(HOST_OS))
$$(OBJ): $$(SRC)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Generate symbol file: $$(notdir $$(PRIVATE_DST))"
	$(hide) android/scripts/gen-entries.py --mode=$$(PRIVATE_MODE) --output=$$(PRIVATE_DST) $$(PRIVATE_SRC)
endef

GEN_ENTRIES_MODE_darwin := _symbols
GEN_ENTRIES_MODE_windows := def
GEN_ENTRIES_MODE_linux := sym

EXPORTED_SYMBOL_LIST_windows :=
EXPORTED_SYMBOL_LIST_darwin := -Wl,-exported_symbols_list,
EXPORTED_SYMBOL_LIST_linux := -Wl,--version-script=

symbol-file-linker-flags = $(EXPORTED_SYMBOL_LIST_$(HOST_OS))$1

# Generate and compile source file through the Qt 'moc' tool
# NOTE: This expects QT_MOC_TOOL to be defined.
define compile-qt-moc-source
SRC:=$(1)
MOC_SRC:=$$(LOCAL_OBJS_DIR)/moc_$$(notdir $$(SRC:%.h=%$$(LOCAL_CPP_EXTENSION)))
ifeq (,$$(strip $$(QT_MOC_TOOL)))
$$(error QT_MOC_TOOL is not defined when trying to generate $$(MOC_SRC) !!)
endif

$$(MOC_SRC): PRIVATE_SRC := $$(SRC)
$$(MOC_SRC): PRIVATE_DST := $$(MOC_SRC)
$$(MOC_SRC): $$(SRC) $$(MOC_TOOL)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Qt moc: $$(notdir $$(PRIVATE_DST)) <-- $$(PRIVATE_SRC)"
	$(hide) $$(QT_MOC_TOOL) -o $$(PRIVATE_DST) $$(PRIVATE_SRC)

$$(eval $$(call compile-generated-cxx-source,$$(MOC_SRC)))
endef

# Generate and compile a Qt resource source file through the 'rcc' tool.
# NOTE: This expects QT_RCC_TOOL to be defined.
define compile-qt-resources
SRC := $(1)
RCC_SRC := $$(LOCAL_OBJS_DIR)/rcc_$$(notdir $$(SRC:%.qrc=%$$(LOCAL_CPP_EXTENSION)))
ifeq (,$$(strip $$(QT_RCC_TOOL)))
$$(error QT_RCC_TOOL is not defined when trying to generate $$(RCC_SRC) !!)
endif
$$(RCC_SRC): PRIVATE_SRC := $$(SRC)
$$(RCC_SRC): PRIVATE_DST := $$(RCC_SRC)
$$(RCC_SRC): PRIVATE_NAME := $$(notdir $$(SRC:%.qrc=%))
$$(RCC_SRC): $$(SRC) $$(QT_RCC_TOOL)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Qt rcc: $$(notdir $$(PRIVATE_DST)) <-- $$(PRIVATE_SRC)"
	$(hide) $$(QT_RCC_TOOL) -o $$(PRIVATE_DST) --name $$(PRIVATE_NAME) $$(PRIVATE_SRC)

$$(eval $$(call compile-generated-cxx-source,$$(RCC_SRC)))
endef

# Process a Qt .ui source file through the 'uic' tool to generate a header.
# NOTE: This expects QT_UIC_TOOL and QT_UIC_TOOL_LDPATH to be defined.
define compile-qt-uic-source
SRC := $(1)
UIC_SRC := $$(LOCAL_OBJS_DIR)/ui_$$(notdir $$(SRC:%.ui=%.h))
ifeq (,$$(strip $$(QT_UIC_TOOL)))
$$(error QT_UIC_TOOL is not defined when trying to generate $$(UIC_SRC) !!)
endif
ifeq (,$$(strip $$(QT_UIC_TOOL_LDPATH)))
$$(error QT_UIC_TOOL_LDPATH is not defined when trying to generate $$(UIC_SRC) !!)
endif
$$(UIC_SRC): PRIVATE_SRC := $$(SRC)
$$(UIC_SRC): PRIVATE_DST := $$(UIC_SRC)
$$(UIC_SRC): $$(SRC) $$(QT_UIC_TOOL)
	@mkdir -p $$(dir $$(PRIVATE_DST))
	@echo "Qt uic: $$(notdir $$(PRIVATE_DST)) <-- $$(PRIVATE_SRC)"
	$(hide) $$(call set-host-library-search-path,$$(QT_UIC_TOOL_LDPATH)) $$(QT_UIC_TOOL) -o $$(PRIVATE_DST) $$(PRIVATE_SRC)

LOCAL_GENERATED_SOURCES += $$(UIC_SRC)
endef
