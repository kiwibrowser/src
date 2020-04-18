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

# Check that LOCAL_MODULE is defined, then restore its LOCAL_XXXX values
$(call assert-defined,LOCAL_MODULE)
$(call module-restore-locals,$(LOCAL_MODULE))

# As in build-module.mk, eval sucks. Manually unstash the cflags variations to
# preserve -Werror=#warnings.
LOCAL_CFLAGS := $(__ndk_modules.$(LOCAL_MODULE).CFLAGS)
LOCAL_CONLYFLAGS := $(__ndk_modules.$(LOCAL_MODULE).CONLYFLAGS)
LOCAL_CPPFLAGS := $(__ndk_modules.$(LOCAL_MODULE).CPPFLAGS)
LOCAL_CXXFLAGS := $(__ndk_modules.$(LOCAL_MODULE).CXXFLAGS)

# For now, only support target (device-specific modules).
# We may want to introduce support for host modules in the future
# but that is too experimental for now.
#
my := TARGET_

# LOCAL_MAKEFILE must also exist and name the Android.mk that
# included the module build script.
#
$(call assert-defined,LOCAL_MAKEFILE LOCAL_BUILD_SCRIPT LOCAL_BUILT_MODULE)

# A list of LOCAL_XXX variables that are ignored for static libraries.
# Print a warning if they are present inside a module definition to let
# the user know this won't do what he/she expects.
not_in_static_libs := \
    LOCAL_LDFLAGS \
    LOCAL_LDLIBS \
    LOCAL_ALLOW_UNDEFINED_SYMBOLS

ifeq ($(call module-get-class,$(LOCAL_MODULE)),STATIC_LIBRARY)
$(foreach _notvar,$(not_in_static_libs),\
    $(if $(strip $($(_notvar))),\
        $(call __ndk_info,WARNING:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): $(_notvar) is always ignored for static libraries)\
    )\
)
endif

# Some developers like to add library names (e.g. -lfoo) to LOCAL_LDLIBS
# and LOCAL_LDFLAGS directly. This is very fragile and can lead to broken
# builds and other nasty surprises, because it doesn't tell ndk-build
# that the corresponding module depends on these files. Emit a warning
# when we detect this case.
libs_in_ldflags := $(filter -l% %.so %.a,$(LOCAL_LDLIBS) $(LOCAL_LDFLAGS))

# Since the above will glob anything ending in .so or .a, we need to filter out
# any cases of -Wl,--exclude-libs since we use that to hide symbols in STLs.
libs_in_ldflags := \
    $(filter-out -Wl$(comma)--exclude-libs$(comma)%,$(libs_in_ldflags))

# Remove the system libraries we know about from the warning, it's ok
# (and actually expected) to link them with -l<name>.
system_libs := \
    EGL \
    GLESv1_CM \
    GLESv2 \
    GLESv3 \
    OpenMAXAL \
    OpenSLES \
    aaudio \
    android \
    atomic \
    c \
    camera2ndk \
    dl \
    jnigraphics \
    log \
    m \
    mediandk \
    stdc++ \
    vulkan \
    z \

libs_in_ldflags := $(filter-out $(addprefix -l,$(system_libs)), $(libs_in_ldflags))

ifneq (,$(strip $(libs_in_ldflags)))
  $(call __ndk_info,WARNING:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): non-system libraries in linker flags: $(libs_in_ldflags))
  $(call __ndk_info,    This is likely to result in incorrect builds. Try using LOCAL_STATIC_LIBRARIES)
  $(call __ndk_info,    or LOCAL_SHARED_LIBRARIES instead to list the library dependencies of the)
  $(call __ndk_info,    current module)
endif

include $(BUILD_SYSTEM)/import-locals.mk

# Check for LOCAL_THIN_ARCHIVE / APP_THIN_ARCHIVE and print a warning if
# it is defined for non-static library modules.
thin_archive := $(strip $(LOCAL_THIN_ARCHIVE))
ifdef thin_archive
ifneq (STATIC_LIBRARY,$(call module-get-class,$(LOCAL_MODULE)))
    $(call __ndk_info,WARNING:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_THIN_ARCHIVE is for building static libraries)
endif
endif

ifndef thin_archive
    thin_archive := $(strip $(NDK_APP_THIN_ARCHIVE))
endif
# Print a warning if the value is not 'true', 'false' or empty.
ifneq (,$(filter-out true false,$(thin_archive)))
    $(call __ndk_info,WARNING:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): Invalid LOCAL_THIN_ARCHIVE value '$(thin_archive)' ignored!)
    thin_archive :=
endif

#
# Ensure that 'make <module>' and 'make clean-<module>' work
#
.PHONY: $(LOCAL_MODULE)
$(LOCAL_MODULE): $(LOCAL_BUILT_MODULE)

cleantarget := clean-$(LOCAL_MODULE)-$(TARGET_ARCH_ABI)
.PHONY: $(cleantarget)
clean: $(cleantarget)

$(cleantarget): PRIVATE_ABI         := $(TARGET_ARCH_ABI)
$(cleantarget): PRIVATE_MODULE      := $(LOCAL_MODULE)
ifneq ($(LOCAL_BUILT_MODULE_NOT_COPIED),true)
$(cleantarget): PRIVATE_CLEAN_FILES := $(LOCAL_BUILT_MODULE) \
                                       $($(my)OBJS)
else
$(cleantarget): PRIVATE_CLEAN_FILES := $($(my)OBJS)
endif
$(cleantarget)::
	$(call host-echo-build-step,$(PRIVATE_ABI),Clean) "$(PRIVATE_MODULE) [$(PRIVATE_ABI)]"
	$(hide) $(call host-rmdir,$(PRIVATE_CLEAN_FILES))

ifeq ($(NDK_APP_DEBUGGABLE),true)
$(NDK_APP_GDBSETUP): PRIVATE_SRC_DIRS += $(LOCAL_C_INCLUDES) $(LOCAL_PATH)
endif

# list of generated object files
LOCAL_OBJECTS :=

# list of generated object files from RS files, subset of LOCAL_OBJECTS
LOCAL_RS_OBJECTS :=

# always define ANDROID when building binaries
#
LOCAL_CFLAGS := -DANDROID $(LOCAL_CFLAGS)
LOCAL_CFLAGS += -D__ANDROID_API__=$(TARGET_PLATFORM_LEVEL)

#
# Add the default system shared libraries to the build
#
ifeq ($(LOCAL_SYSTEM_SHARED_LIBRARIES),none)
  LOCAL_SHARED_LIBRARIES += $(TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES)
else
  LOCAL_SHARED_LIBRARIES += $(LOCAL_SYSTEM_SHARED_LIBRARIES)
endif

#
# Check LOCAL_CPP_EXTENSION
#
bad_cpp_extensions := $(strip $(filter-out .%,$(LOCAL_CPP_EXTENSION)))
ifdef bad_cpp_extensions
    $(call __ndk_info,WARNING: Invalid LOCAL_CPP_EXTENSION values: $(bad_cpp_extensions))
    LOCAL_CPP_EXTENSION := $(filter $(bad_cpp_extensions),$(LOCAL_CPP_EXTENSIONS))
endif
LOCAL_CPP_EXTENSION := $(strip $(LOCAL_CPP_EXTENSION))
ifeq ($(LOCAL_CPP_EXTENSION),)
  # Match the default GCC C++ extensions.
  LOCAL_CPP_EXTENSION := $(default-c++-extensions)
endif
LOCAL_RS_EXTENSION := $(default-rs-extensions)

LOCAL_LDFLAGS += -Wl,--build-id

#
# If LOCAL_ALLOW_UNDEFINED_SYMBOLS is not true, the linker will allow the generation
# of a binary that uses undefined symbols.
#
ifneq ($(LOCAL_ALLOW_UNDEFINED_SYMBOLS),true)
  LOCAL_LDFLAGS += $($(my)NO_UNDEFINED_LDFLAGS)
endif

# Toolchain by default disallows generated code running from the heap and stack.
# If LOCAL_DISABLE_NO_EXECUTE is true, we allow that
#
ifeq ($(LOCAL_DISABLE_NO_EXECUTE),true)
  LOCAL_CFLAGS += $($(my)DISABLE_NO_EXECUTE_CFLAGS)
  LOCAL_LDFLAGS += $($(my)DISABLE_NO_EXECUTE_LDFLAGS)
else
  LOCAL_CFLAGS += $($(my)NO_EXECUTE_CFLAGS)
  LOCAL_LDFLAGS += $($(my)NO_EXECUTE_LDFLAGS)
endif

# Toolchain by default provides relro and GOT protections.
# If LOCAL_DISABLE_RELRO is true, we disable the protections.
#
ifeq ($(LOCAL_DISABLE_RELRO),true)
  LOCAL_LDFLAGS += $($(my)DISABLE_RELRO_LDFLAGS)
else
  LOCAL_LDFLAGS += $($(my)RELRO_LDFLAGS)
endif

# We enable shared text relocation warnings by default. These are not allowed in
# current versions of Android (android-21 for LP64 ABIs, android-23 for LP32
# ABIs).
LOCAL_LDFLAGS += -Wl,--warn-shared-textrel

# We enable fatal linker warnings by default.
# If LOCAL_DISABLE_FATAL_LINKER_WARNINGS is true, we don't enable this check.
ifneq ($(LOCAL_DISABLE_FATAL_LINKER_WARNINGS),true)
  LOCAL_LDFLAGS += -Wl,--fatal-warnings
endif

# By default, we protect against format string vulnerabilities
# If LOCAL_DISABLE_FORMAT_STRING_CHECKS is true, we disable the protections.
ifeq ($(LOCAL_DISABLE_FORMAT_STRING_CHECKS),true)
  LOCAL_CFLAGS += $($(my)DISABLE_FORMAT_STRING_CFLAGS)
else
  LOCAL_CFLAGS += $($(my)FORMAT_STRING_CFLAGS)
endif

# enable PIE for executable beyond certain API level, unless "-static"
ifneq (,$(filter true,$(NDK_APP_PIE) $(TARGET_PIE)))
  ifeq ($(call module-get-class,$(LOCAL_MODULE)),EXECUTABLE)
    ifeq (,$(filter -static,$(TARGET_LDFLAGS) $(LOCAL_LDFLAGS) $(NDK_APP_LDFLAGS)))
      # x86 and x86_64 use large model pic, whereas everything else uses small
      # model. In the past we've always used -fPIE, but the LLVMgold plugin (for
      # LTO) complains if the models are mismatched.
      ifneq (,$(filter x86 x86_64,$(TARGET_ARCH_ABI)))
        LOCAL_CFLAGS += -fPIE
        LOCAL_LDFLAGS += -fPIE -pie
      else
        LOCAL_CFLAGS += -fpie
        LOCAL_LDFLAGS += -fpie -pie
      endif
    endif
  endif
endif

# http://b.android.com/222239
# http://b.android.com/220159 (internal http://b/31809417)
# x86 devices have stack alignment issues.
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_CFLAGS += -mstackrealign
endif

# https://github.com/android-ndk/ndk/issues/297
ifeq ($(TARGET_ARCH_ABI),x86)
    ifneq (,$(call lt,$(APP_PLATFORM_LEVEL),17))
        ifeq ($(NDK_TOOLCHAIN_VERSION),4.9)
            LOCAL_CFLAGS += -mstack-protector-guard=global
        endif
    endif
endif

#
# The original Android build system allows you to use the .arm prefix
# to a source file name to indicate that it should be defined in either
# 'thumb' or 'arm' mode, depending on the value of LOCAL_ARM_MODE
#
# First, check LOCAL_ARM_MODE, it should be empty, 'thumb' or 'arm'
# We make the default 'thumb'
#
LOCAL_ARM_MODE := $(strip $(LOCAL_ARM_MODE))
ifdef LOCAL_ARM_MODE
  ifneq ($(words $(LOCAL_ARM_MODE)),1)
      $(call __ndk_info,   LOCAL_ARM_MODE in $(LOCAL_MAKEFILE) must be one word, not '$(LOCAL_ARM_MODE)')
      $(call __ndk_error, Aborting)
  endif
  # check that LOCAL_ARM_MODE is defined to either 'arm' or 'thumb'
  $(if $(filter-out thumb arm, $(LOCAL_ARM_MODE)),\
      $(call __ndk_info,   LOCAL_ARM_MODE must be defined to either 'arm' or 'thumb' in $(LOCAL_MAKEFILE) not '$(LOCAL_ARM_MODE)')\
      $(call __ndk_error, Aborting)\
  )
  my_link_arm_mode := $(LOCAL_ARM_MODE)
else
  my_link_arm_mode := thumb
endif

# As a special case, the original Android build system
# allows one to specify that certain source files can be
# forced to build in ARM mode by using a '.arm' suffix
# after the extension, e.g.
#
#  LOCAL_SRC_FILES := foo.c.arm
#
# to build source file $(LOCAL_PATH)/foo.c as ARM
#

$(call clear-all-src-tags)

# As a special extension, the NDK also supports the .neon extension suffix
# to indicate that a single file can be compiled with ARM NEON support
# We must support both foo.c.neon and foo.c.arm.neon here
#
# Also, if LOCAL_ARM_NEON is set to 'true', force Neon mode for all source
# files
#

neon_sources  := $(filter %.neon,$(LOCAL_SRC_FILES))
neon_sources  := $(neon_sources:%.neon=%)

LOCAL_ARM_NEON := $(strip $(LOCAL_ARM_NEON))
ifdef LOCAL_ARM_NEON
  $(if $(filter-out true false,$(LOCAL_ARM_NEON)),\
    $(call __ndk_info,LOCAL_ARM_NEON must be defined either to 'true' or 'false' in $(LOCAL_MAKEFILE), not '$(LOCAL_ARM_NEON)')\
    $(call __ndk_error,Aborting) \
  )
endif
ifeq ($(LOCAL_ARM_NEON),true)
  neon_sources += $(LOCAL_SRC_FILES:%.neon=%)
  # tag the precompiled header with 'neon' tag if it exists
  ifneq (,$(LOCAL_PCH))
    $(call tag-src-files,$(LOCAL_PCH),neon)
  endif
endif

neon_sources := $(strip $(neon_sources))
ifdef neon_sources
  ifeq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a arm64-v8a x86 x86_64),)
    $(call __ndk_info,NEON support is only available for armeabi-v7a, arm64-v8a, x86, and x86_64 ABIs)
    $(call __ndk_info,Please add checks against TARGET_ARCH_ABI in $(LOCAL_MAKEFILE))
    $(call __ndk_error,Aborting)
  endif
  $(call tag-src-files,$(neon_sources:%.arm=%),neon)
endif

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%.neon=%)

# strip the .arm suffix from LOCAL_SRC_FILES
# and tag the relevant sources with the 'arm' tag
#
arm_sources     := $(filter %.arm,$(LOCAL_SRC_FILES))
arm_sources     := $(arm_sources:%.arm=%)
thumb_sources   := $(filter-out %.arm,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%.arm=%)

ifeq ($(LOCAL_ARM_MODE),arm)
    arm_sources := $(LOCAL_SRC_FILES)
    # tag the precompiled header with 'arm' tag if it exists
    ifneq (,$(LOCAL_PCH))
        $(call tag-src-files,$(LOCAL_PCH),arm)
    endif
endif
ifeq ($(LOCAL_ARM_MODE),thumb)
    arm_sources := $(empty)
endif
$(call tag-src-files,$(arm_sources),arm)

# tag debug if APP_OPTIM is 'debug'
#
ifeq ($(APP_OPTIM),debug)
    $(call tag-src-files,$(LOCAL_SRC_FILES),debug)
    ifneq (,$(LOCAL_PCH))
        $(call tag-src-files,$(LOCAL_PCH),debug)
    endif
endif

# add PCH to LOCAL_SRC_FILES so that TARGET-process-src-files-tags could process it
ifneq (,$(LOCAL_PCH))
    LOCAL_SRC_FILES += $(LOCAL_PCH)
endif

# Process all source file tags to determine toolchain-specific
# target compiler flags, and text.
#
$(call TARGET-process-src-files-tags)

# now remove PCH from LOCAL_SRC_FILES to prevent getting NDK warning about
# unsupported source file extensions
ifneq (,$(LOCAL_PCH))
    LOCAL_SRC_FILES := $(filter-out $(LOCAL_PCH),$(LOCAL_SRC_FILES))
endif

# only call dump-src-file-tags during debugging
#$(dump-src-file-tags)

LOCAL_DEPENDENCY_DIRS :=

# all_source_patterns contains the list of filename patterns that correspond
# to source files recognized by our build system
ifneq ($(filter x86 x86_64, $(TARGET_ARCH_ABI)),)
all_source_extensions := .c .s .S .asm $(LOCAL_CPP_EXTENSION) $(LOCAL_RS_EXTENSION)
else
all_source_extensions := .c .s .S $(LOCAL_CPP_EXTENSION) $(LOCAL_RS_EXTENSION)
endif
all_source_patterns   := $(foreach _ext,$(all_source_extensions),%$(_ext))
all_cpp_patterns      := $(foreach _ext,$(LOCAL_CPP_EXTENSION),%$(_ext))
all_rs_patterns       := $(foreach _ext,$(LOCAL_RS_EXTENSION),%$(_ext))

unknown_sources := $(strip $(filter-out $(all_source_patterns),$(LOCAL_SRC_FILES)))
ifdef unknown_sources
    $(call __ndk_info,WARNING: Unsupported source file extensions in $(LOCAL_MAKEFILE) for module $(LOCAL_MODULE))
    $(call __ndk_info,  $(unknown_sources))
endif

# LOCAL_OBJECTS will list all object files corresponding to the sources
# listed in LOCAL_SRC_FILES, in the *same* order.
#
LOCAL_OBJECTS := $(LOCAL_SRC_FILES)
$(foreach _ext,$(all_source_extensions),\
    $(eval LOCAL_OBJECTS := $$(LOCAL_OBJECTS:%$(_ext)=%$$(TARGET_OBJ_EXTENSION)))\
)
LOCAL_OBJECTS := $(filter %$(TARGET_OBJ_EXTENSION),$(LOCAL_OBJECTS))
LOCAL_OBJECTS := $(subst ../,__/,$(LOCAL_OBJECTS))
LOCAL_OBJECTS := $(subst :,_,$(LOCAL_OBJECTS))
LOCAL_OBJECTS := $(foreach _obj,$(LOCAL_OBJECTS),$(LOCAL_OBJS_DIR)/$(_obj))

LOCAL_RS_OBJECTS := $(filter $(all_rs_patterns),$(LOCAL_SRC_FILES))
$(foreach _ext,$(LOCAL_RS_EXTENSION),\
    $(eval LOCAL_RS_OBJECTS := $$(LOCAL_RS_OBJECTS:%$(_ext)=%$$(TARGET_OBJ_EXTENSION)))\
)
LOCAL_RS_OBJECTS := $(filter %$(TARGET_OBJ_EXTENSION),$(LOCAL_RS_OBJECTS))
LOCAL_RS_OBJECTS := $(subst ../,__/,$(LOCAL_RS_OBJECTS))
LOCAL_RS_OBJECTS := $(subst :,_,$(LOCAL_RS_OBJECTS))
LOCAL_RS_OBJECTS := $(foreach _obj,$(LOCAL_RS_OBJECTS),$(LOCAL_OBJS_DIR)/$(_obj))

# If the module has any kind of C++ features, enable them in LOCAL_CPPFLAGS
#
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),rtti))
    LOCAL_CPPFLAGS += -frtti
endif
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),exceptions))
    LOCAL_CPPFLAGS += -fexceptions
endif

# If we're using the 'system' STL and use rtti or exceptions, then
# automatically link against the GNU libsupc++ for now.
#
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),rtti exceptions))
    ifeq (system,$(NDK_APP_STL))
      LOCAL_LDLIBS := $(LOCAL_LDLIBS) $(call host-path,$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI)/libsupc++$(TARGET_LIB_EXTENSION))
    endif
endif

# Set include patch for renderscript


ifneq ($(LOCAL_RENDERSCRIPT_INCLUDES_OVERRIDE),)
    LOCAL_RENDERSCRIPT_INCLUDES := $(LOCAL_RENDERSCRIPT_INCLUDES_OVERRIDE)
else
    LOCAL_RENDERSCRIPT_INCLUDES := \
        $(RENDERSCRIPT_PLATFORM_HEADER)/scriptc \
        $(RENDERSCRIPT_TOOLCHAIN_HEADER) \
        $(LOCAL_RENDERSCRIPT_INCLUDES)
endif

# Only enable the compatibility path when LOCAL_RENDERSCRIPT_COMPATIBILITY is defined.
RS_COMPAT :=
ifeq ($(LOCAL_RENDERSCRIPT_COMPATIBILITY),true)
    RS_COMPAT := true
endif


# Build PCH

get-pch-name = $(strip \
    $(subst ../,__/,\
        $(eval __pch := $1)\
        $(eval __pch := $(__pch:%.h=%.precompiled.h))\
        $(__pch)\
    ))

ifneq (,$(LOCAL_PCH))
    # Build PCH into obj directory
    LOCAL_BUILT_PCH := $(call get-pch-name,$(LOCAL_PCH))

    # Clang whines about a "c-header" (.h rather than .hpp) being used in C++
    # mode (note that we use compile-cpp-source to build the header).
    LOCAL_SRC_FILES_TARGET_CFLAGS.$(LOCAL_PCH) += -x c++-header

    # Build PCH
    $(call compile-cpp-source,$(LOCAL_PCH),$(LOCAL_BUILT_PCH).gch)

    # The PCH must be compiled the same way as the sources (thumb vs arm, neon
    # vs non-neon must match). This means that we'd have to generate a PCH for
    # each combination of foo.c.arm and foo.c.neon (do we allow
    # foo.c.arm.neon?).
    #
    # Since files with those source tags should be the minority, precompiling
    # that header might be a net loss compared to just using it normally. As
    # such, we only use the PCH for the default compilation mode for the module.
    #
    # See https://github.com/android-ndk/ndk/issues/14
    TAGS_TO_FILTER :=

    # If neon is off, strip out .neon files.
    ifneq (true,$(LOCAL_ARM_NEON))
        TAGS_TO_FILTER += neon
    endif

    # If we're building thumb, strip out .arm files.
    ifneq (arm,$(LOCAL_ARM_MODE))
        TAGS_TO_FILTER += arm
    endif

    # There is no .thumb. No need to filter them out if we're building ARM.

    allowed_src := $(foreach src,$(filter $(all_cpp_patterns),$(LOCAL_SRC_FILES)),\
        $(if $(filter $(TAGS_TO_FILTER),$(LOCAL_SRC_FILES_TAGS.$(src))),,$(src))\
    )
    # All files without tags depend on PCH
    $(foreach src,$(allowed_src),\
        $(eval $(LOCAL_OBJS_DIR)/$(call get-object-name,$(src)) : $(LOCAL_OBJS_DIR)/$(LOCAL_BUILT_PCH).gch)\
    )
    # Make sure those files are built with PCH
    $(call add-src-files-target-cflags,$(allowed_src),-Winvalid-pch -include $(LOCAL_OBJS_DIR)/$(LOCAL_BUILT_PCH))

    # Insert PCH dir at beginning of include search path
    LOCAL_C_INCLUDES := \
        $(LOCAL_OBJS_DIR) \
        $(LOCAL_C_INCLUDES)
endif

# Build the sources to object files
#

# Include RenderScript headers if rs files are found.
ifneq ($(filter $(all_rs_patterns),$(LOCAL_SRC_FILES)),)
    LOCAL_C_INCLUDES += \
        $(RENDERSCRIPT_PLATFORM_HEADER) \
        $(RENDERSCRIPT_PLATFORM_HEADER)/cpp \
        $(TARGET_OBJS)/$(LOCAL_MODULE)
endif

$(foreach src,$(filter %.c,$(LOCAL_SRC_FILES)), $(call compile-c-source,$(src),$(call get-object-name,$(src))))
$(foreach src,$(filter %.S %.s,$(LOCAL_SRC_FILES)), $(call compile-s-source,$(src),$(call get-object-name,$(src))))
$(foreach src,$(filter $(all_cpp_patterns),$(LOCAL_SRC_FILES)),\
    $(call compile-cpp-source,$(src),$(call get-object-name,$(src)))\
)

$(foreach src,$(filter $(all_rs_patterns),$(LOCAL_SRC_FILES)),\
    $(call compile-rs-source,$(src),$(call get-rs-scriptc-name,$(src)),$(call get-rs-bc-name,$(src)),$(call get-rs-so-name,$(src)),$(call get-object-name,$(src)),$(RS_COMPAT))\
)

ifneq ($(filter x86 x86_64, $(TARGET_ARCH_ABI)),)
$(foreach src,$(filter %.asm,$(LOCAL_SRC_FILES)), $(call compile-asm-source,$(src),$(call get-object-name,$(src))))
endif

#
# The compile-xxx-source calls updated LOCAL_OBJECTS and LOCAL_DEPENDENCY_DIRS
#
ALL_DEPENDENCY_DIRS += $(sort $(LOCAL_DEPENDENCY_DIRS))
CLEAN_OBJS_DIRS     += $(LOCAL_OBJS_DIR)

#
# Handle the static and shared libraries this module depends on
#

# If LOCAL_LDLIBS contains anything like -l<library> then
# prepend a -L$(SYSROOT_LINK)/usr/lib to it to ensure that the linker
# looks in the right location
#
ifneq ($(filter -l%,$(LOCAL_LDLIBS)),)
    LOCAL_LDLIBS := -L$(call host-path,$(SYSROOT_LINK)/usr/lib) $(LOCAL_LDLIBS)
    ifneq ($(filter x86_64 mips64,$(TARGET_ARCH_ABI)),)
        LOCAL_LDLIBS := -L$(call host-path,$(SYSROOT_LINK)/usr/lib64) $(LOCAL_LDLIBS)
    endif
endif

my_ldflags := $(TARGET_LDFLAGS) $(LOCAL_LDFLAGS) $(NDK_APP_LDFLAGS)
ifneq ($(filter armeabi%,$(TARGET_ARCH_ABI)),)
    my_ldflags += $(TARGET_$(my_link_arm_mode)_LDFLAGS)
endif

# When LOCAL_SHORT_COMMANDS is defined to 'true' we are going to write the
# list of all object files and/or static/shared libraries that appear on the
# command line to a file, then use the @<listfile> syntax to invoke it.
#
# This allows us to link or archive a huge number of stuff even on Windows
# with its puny 8192 max character limit on its command-line.
#
LOCAL_SHORT_COMMANDS := $(strip $(LOCAL_SHORT_COMMANDS))
ifndef LOCAL_SHORT_COMMANDS
    LOCAL_SHORT_COMMANDS := $(strip $(NDK_APP_SHORT_COMMANDS))
endif

$(call generate-file-dir,$(LOCAL_BUILT_MODULE))

$(LOCAL_BUILT_MODULE): PRIVATE_OBJECTS := $(LOCAL_OBJECTS)
$(LOCAL_BUILT_MODULE): PRIVATE_LIBGCC := $(TARGET_LIBGCC)
$(LOCAL_BUILT_MODULE): PRIVATE_LIBGCC := $(TARGET_LIBATOMIC)

$(LOCAL_BUILT_MODULE): PRIVATE_LD := $(TARGET_LD)
$(LOCAL_BUILT_MODULE): PRIVATE_LDFLAGS := $(my_ldflags)
$(LOCAL_BUILT_MODULE): PRIVATE_LDLIBS  := $(LOCAL_LDLIBS) $(TARGET_LDLIBS)

$(LOCAL_BUILT_MODULE): PRIVATE_NAME := $(notdir $(LOCAL_BUILT_MODULE))
$(LOCAL_BUILT_MODULE): PRIVATE_CXX := $(TARGET_CXX)
$(LOCAL_BUILT_MODULE): PRIVATE_CC := $(TARGET_CC)
$(LOCAL_BUILT_MODULE): PRIVATE_SYSROOT_LINK := $(SYSROOT_LINK)

ifeq ($(call module-get-class,$(LOCAL_MODULE)),STATIC_LIBRARY)

#
# This is a static library module, things are very easy. We only need
# to build the object files and archive them with 'ar'. Note that module
# dependencies can be ignored here, i.e. if the module depends on other
# static or shared libraries, there is no need to actually build them
# before, so don't add Make dependencies to them.
#
# In other words, consider the following graph:
#
#     libfoo.so -> libA.a ->libB.a
#
# then libA.a and libB.a can be built in parallel, only linking libfoo.so
# depends on their completion.
#

ar_objects := $(call host-path,$(LOCAL_OBJECTS))

ifeq ($(LOCAL_SHORT_COMMANDS),true)
    $(call ndk_log,Building static library module '$(LOCAL_MODULE)' with linker list file)
    ar_list_file := $(LOCAL_OBJS_DIR)/archiver.list
    $(call generate-list-file,\
        $(call escape-backslashes,$(ar_objects)),$(ar_list_file))
    ar_objects   := @$(call host-path,$(ar_list_file))
    $(LOCAL_BUILT_MODULE): $(ar_list_file)
endif

# Compute 'ar' flags. Thin archives simply require 'T' here.
ar_flags := $(TARGET_ARFLAGS)
ifeq (true,$(thin_archive))
    $(call ndk_log,$(TARGET_ARCH_ABI):Building static library '$(LOCAL_MODULE)' as thin archive)
    ar_flags := $(ar_flags)T
endif

$(LOCAL_BUILT_MODULE): PRIVATE_ABI := $(TARGET_ARCH_ABI)
$(LOCAL_BUILT_MODULE): PRIVATE_AR := $(TARGET_AR) $(ar_flags) $(TARGET_AR_FLAGS)
$(LOCAL_BUILT_MODULE): PRIVATE_AR_OBJECTS := $(ar_objects)
$(LOCAL_BUILT_MODULE): PRIVATE_BUILD_STATIC_LIB := $(cmd-build-static-library)

$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	$(call host-echo-build-step,$(PRIVATE_ABI),StaticLibrary) "$(PRIVATE_NAME)"
	$(hide) $(call host-rm,$@)
	$(hide) $(PRIVATE_BUILD_STATIC_LIB)

ALL_STATIC_LIBRARIES += $(LOCAL_BUILT_MODULE)

endif

ifneq (,$(filter SHARED_LIBRARY EXECUTABLE,$(call module-get-class,$(LOCAL_MODULE))))

#
# This is a shared library or an executable, so computing dependencies properly is
# crucial. The general rule to apply is the following:
#
#   - collect the list of all static libraries that need to be part
#     of the link, and in the right order. To do so, get the transitive
#     closure of LOCAL_STATIC_LIBRARIES and LOCAL_WHOLE_STATIC_LIBRARIES
#     and ensure they are ordered topologically.
#
#  - collect the list of all shared libraries that need to be part of
#    the link. This is the transitive closure of the list of
#    LOCAL_SHARED_LIBRARIES for the module and all its dependent static
#    libraries identified in the step above. Of course, need to be
#    ordered topologically too.
#
#  - add Make dependencies to ensure that all these libs are built
#    before the module itself too.
#
# A few quick examples:
#
#    main.exe -> libA.a -> libB.a -> libfoo.so -> libC.a
#
#      static_libs(main.exe) = libA.a libB.a  (i.e. no libC.a)
#      shared_libs(main.exe) = libfoo.so
#      static_libs(libfoo.so) = libC.a
#
#    main.exe -> libA.a ---> libB.a
#                  |           ^
#                  v           |
#                libC.a  ------
#
#      static_libs(main.exe) = libA.a libC.a libB.a
#             (i.e. libB.a must appear after all libraries that depend on it).
#
all_libs := $(call module-get-link-libs,$(LOCAL_MODULE))
shared_libs := $(call module-filter-shared-libraries,$(all_libs))
static_libs := $(call module-filter-static-libraries,$(all_libs))
whole_static_libs := $(call module-extract-whole-static-libs,$(LOCAL_MODULE),$(static_libs))
static_libs := $(filter-out $(whole_static_libs),$(static_libs))
all_defined_libs := $(shared_libs) $(static_libs) $(whole_static_libs)
undefined_libs := $(filter-out $(all_defined_libs),$(all_libs))

ifdef undefined_libs
    $(call __ndk_warning,Module $(LOCAL_MODULE) depends on undefined modules: $(undefined_libs))

    # https://github.com/android-ndk/ndk/issues/208
    # ndk-build didn't used to fail the build for a missing dependency. This
    # seems to have always been the behavior, so there's a good chance that
    # there are builds out there that depend on this behavior (as of right now,
    # anything using libc++ on ARM has this problem because of libunwind).
    #
    # By default we will abort in this situation because this is so completely
    # broken. A user may define APP_ALLOW_MISSING_DEPS to "true" in their
    # Application.mk or on the command line to revert to the old, broken
    # behavior.
    ifneq ($(APP_ALLOW_MISSING_DEPS),true)
        $(call __ndk_error,Aborting (set APP_ALLOW_MISSING_DEPS=true to allow missing dependencies))
    endif
endif

$(call -ndk-mod-debug,module $(LOCAL_MODULE) [$(LOCAL_BUILT_MODULE)])
$(call -ndk-mod-debug,.  all_libs='$(all_libs)')
$(call -ndk-mod-debug,.  shared_libs='$(shared_libs)')
$(call -ndk-mod-debug,.  static_libs='$(static_libs)')
$(call -ndk-mod-debug,.  whole_static_libs='$(whole_static_libs)')

shared_libs       := $(call map,module-get-built,$(shared_libs))\
                     $(TARGET_PREBUILT_SHARED_LIBRARIES)
static_libs       := $(call map,module-get-built,$(static_libs))
whole_static_libs := $(call map,module-get-built,$(whole_static_libs))

$(call -ndk-mod-debug,.  built_shared_libs='$(shared_libs)')
$(call -ndk-mod-debug,.  built_static_libs='$(static_libs)')
$(call -ndk-mod-debug,.  built_whole_static_libs='$(whole_static_libs)')

# The list of object/static/shared libraries passed to the linker when
# building shared libraries and executables. order is important.
#
# Cannot use immediate evaluation because PRIVATE_LIBGCC may not be defined at this point.
linker_objects_and_libraries = $(strip $(call TARGET-get-linker-objects-and-libraries,\
    $(LOCAL_OBJECTS), \
    $(static_libs), \
    $(whole_static_libs), \
    $(shared_libs)))

ifeq ($(LOCAL_SHORT_COMMANDS),true)
    $(call ndk_log,Building ELF binary module '$(LOCAL_MODULE)' with linker list file)
    linker_options   := $(linker_objects_and_libraries)
    linker_list_file := $(LOCAL_OBJS_DIR)/linker.list
    linker_objects_and_libraries := @$(call host-path,$(linker_list_file))
    $(call generate-list-file,$(linker_options),$(linker_list_file))
    $(LOCAL_BUILT_MODULE): $(linker_list_file)
endif

$(LOCAL_BUILT_MODULE): $(shared_libs) $(static_libs) $(whole_static_libs)
$(LOCAL_BUILT_MODULE): PRIVATE_ABI := $(TARGET_ARCH_ABI)
$(LOCAL_BUILT_MODULE): PRIVATE_LINKER_OBJECTS_AND_LIBRARIES := $(linker_objects_and_libraries)
$(LOCAL_BUILT_MODULE): PRIVATE_STATIC_LIBRARIES := $(static_libs)
$(LOCAL_BUILT_MODULE): PRIVATE_WHOLE_STATIC_LIBRARIES := $(whole_static_libs)
$(LOCAL_BUILT_MODULE): PRIVATE_SHARED_LIBRARIES := $(shared_libs)

endif

#
# If this is a shared library module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),SHARED_LIBRARY)
$(LOCAL_BUILT_MODULE): PRIVATE_BUILD_SHARED_LIB := $(cmd-build-shared-library)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	$(call host-echo-build-step,$(PRIVATE_ABI),SharedLibrary) "$(PRIVATE_NAME)"
	$(hide) $(PRIVATE_BUILD_SHARED_LIB)

ALL_SHARED_LIBRARIES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is an executable module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),EXECUTABLE)
$(LOCAL_BUILT_MODULE): PRIVATE_ABI := $(TARGET_ARCH_ABI)
$(LOCAL_BUILT_MODULE): PRIVATE_BUILD_EXECUTABLE := $(cmd-build-executable)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	$(call host-echo-build-step,$(PRIVATE_ABI),Executable) "$(PRIVATE_NAME)"
	$(hide) $(PRIVATE_BUILD_EXECUTABLE)

ALL_EXECUTABLES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is a copyable prebuilt module
#
ifeq ($(call module-is-copyable,$(LOCAL_MODULE)),$(true))
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	$(call host-echo-build-step,$(PRIVATE_ABI),Prebuilt) "$(PRIVATE_NAME) <= $(call pretty-dir,$(dir $<))"
	$(hide) $(call host-cp,$<,$@)
endif

#
# If this is an installable module
#
ifeq ($(call module-is-installable,$(LOCAL_MODULE)),$(true))
$(LOCAL_INSTALLED): PRIVATE_ABI         := $(TARGET_ARCH_ABI)
$(LOCAL_INSTALLED): PRIVATE_NAME        := $(notdir $(LOCAL_BUILT_MODULE))
$(LOCAL_INSTALLED): PRIVATE_SRC         := $(LOCAL_BUILT_MODULE)
$(LOCAL_INSTALLED): PRIVATE_DST_DIR     := $(NDK_APP_DST_DIR)
$(LOCAL_INSTALLED): PRIVATE_DST         := $(LOCAL_INSTALLED)
$(LOCAL_INSTALLED): PRIVATE_STRIP       := $(TARGET_STRIP)
$(LOCAL_INSTALLED): PRIVATE_STRIP_CMD   := $(call cmd-strip, $(PRIVATE_DST))
$(LOCAL_INSTALLED): PRIVATE_OBJCOPY     := $(TARGET_OBJCOPY)
$(LOCAL_INSTALLED): PRIVATE_OBJCOPY_CMD := $(call cmd-add-gnu-debuglink, $(PRIVATE_DST), $(PRIVATE_SRC))

$(LOCAL_INSTALLED): $(LOCAL_BUILT_MODULE) clean-installed-binaries
	$(call host-echo-build-step,$(PRIVATE_ABI),Install) "$(PRIVATE_NAME) => $(call pretty-dir,$(PRIVATE_DST))"
	$(hide) $(call host-install,$(PRIVATE_SRC),$(PRIVATE_DST))
	$(hide) $(PRIVATE_STRIP_CMD)

#$(hide) $(PRIVATE_OBJCOPY_CMD)

$(call generate-file-dir,$(LOCAL_INSTALLED))

endif
