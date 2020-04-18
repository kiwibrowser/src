# The following definitions are the defaults used by all toolchains.
# This is included in setup-toolchain.mk just before the inclusion
# of the toolchain's specific setup.mk file which can then override
# these definitions.
#

# These flags are used to ensure that a binary doesn't reference undefined
# flags.
TARGET_NO_UNDEFINED_LDFLAGS := -Wl,--no-undefined


# Return the list of object, static libraries and shared libraries as they
# must appear on the final static linker command (order is important).
#
# This can be over-ridden by a specific toolchain. Note that by default
# we always put libgcc _after_ all static libraries and _before_ shared
# libraries. This ensures that any libgcc function used by the final
# executable will be copied into it. Otherwise, it could contain
# symbol references to the same symbols as exported by shared libraries
# and this causes binary compatibility problems when they come from
# system libraries (e.g. libc.so and others).
#
# IMPORTANT: The result must use the host path convention.
#
# $1: object files
# $2: static libraries
# $3: whole static libraries
# $4: shared libraries
#
TARGET-get-linker-objects-and-libraries = \
    $(call host-path, $1) \
    $(call link-whole-archives,$3) \
    $(call host-path, $2) \
    $(PRIVATE_LIBGCC) \
    $(PRIVATE_LIBATOMIC) \
    $(call host-path, $4) \


# These flags are used to enforce the NX (no execute) security feature in the
# generated machine code. This adds a special section to the generated shared
# libraries that instruct the Linux kernel to disable code execution from
# the stack and the heap.
TARGET_NO_EXECUTE_CFLAGS  := -Wa,--noexecstack
TARGET_NO_EXECUTE_LDFLAGS := -Wl,-z,noexecstack

# These flags disable the above security feature
TARGET_DISABLE_NO_EXECUTE_CFLAGS  := -Wa,--execstack
TARGET_DISABLE_NO_EXECUTE_LDFLAGS := -Wl,-z,execstack

# These flags are used to mark certain regions of the resulting
# executable or shared library as being read-only after the dynamic
# linker has run. This makes GOT overwrite security attacks harder to
# exploit.
TARGET_RELRO_LDFLAGS := -Wl,-z,relro -Wl,-z,now

# These flags disable the above security feature
TARGET_DISABLE_RELRO_LDFLAGS := -Wl,-z,norelro -Wl,-z,lazy

# This flag are used to provide compiler protection against format
# string vulnerabilities.
TARGET_FORMAT_STRING_CFLAGS := -Wformat -Werror=format-security

# This flag disables the above security checks
TARGET_DISABLE_FORMAT_STRING_CFLAGS := -Wno-error=format-security

# NOTE: Ensure that TARGET_LIBGCC is placed after all private objects
#       and static libraries, but before any other library in the link
#       command line when generating shared libraries and executables.
#
#       This ensures that all libgcc.a functions required by the target
#       will be included into it, instead of relying on what's available
#       on other libraries like libc.so, which may change between system
#       releases due to toolchain or library changes.
#
define cmd-build-shared-library
$(PRIVATE_CXX) \
    -Wl,-soname,$(notdir $(LOCAL_BUILT_MODULE)) \
    -shared \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE))
endef

# The following -rpath-link= are needed for ld.bfd (default for MIPS) when
# linking executables to supress warning about missing symbol from libraries not
# directly needed. ld.gold (default for ARM and X86) doesn't emulate this buggy
# behavior, and ignores -rpath-link completely.
define cmd-build-executable
$(PRIVATE_CXX) \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
    -Wl,-rpath-link=$(call host-path,$(PRIVATE_SYSROOT_LINK)/usr/lib) \
    -Wl,-rpath-link=$(call host-path,$(TARGET_OUT)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE))
endef

define cmd-build-static-library
$(PRIVATE_AR) $(call host-path,$(LOCAL_BUILT_MODULE)) $(PRIVATE_AR_OBJECTS)
endef

# The strip command is only used for shared libraries and executables.
# It is thus safe to use --strip-unneeded, which is only dangerous
# when applied to static libraries or object files.
cmd-strip = $(PRIVATE_STRIP) --strip-unneeded $(call host-path,$1)

# The command objcopy --add-gnu-debuglink= will be needed for Valgrind
cmd-add-gnu-debuglink = $(PRIVATE_OBJCOPY) --add-gnu-debuglink=$(strip $(call host-path,$2)) $(call host-path,$1)

TARGET_LIBGCC = -lgcc -Wl,--exclude-libs,libgcc.a
TARGET_LIBATOMIC = -latomic -Wl,--exclude-libs,libatomic.a
TARGET_LDLIBS := -lc -lm

#
# IMPORTANT: The following definitions must use lazy assignment because
# the value of TOOLCHAIN_PREFIX or TARGET_CFLAGS can be changed later by
# the toolchain's setup.mk script.
#

LLVM_TOOLCHAIN_PREBUILT_ROOT := $(call get-toolchain-root,llvm)
LLVM_TOOLCHAIN_PREFIX := $(LLVM_TOOLCHAIN_PREBUILT_ROOT)/bin/

ifneq ($(findstring ccc-analyzer,$(CC)),)
    TARGET_CC = $(CC)
else ifeq ($(NDK_TOOLCHAIN_VERSION),4.9)
    TARGET_CC = $(TOOLCHAIN_PREFIX)gcc
else
    TARGET_CC = $(LLVM_TOOLCHAIN_PREFIX)clang$(HOST_EXEEXT)
endif

TARGET_CFLAGS   =
TARGET_CONLYFLAGS =

ifneq ($(findstring c++-analyzer,$(CXX)),)
    TARGET_CXX = $(CXX)
else ifeq ($(NDK_TOOLCHAIN_VERSION),4.9)
    TARGET_CXX = $(TOOLCHAIN_PREFIX)g++
else
    TARGET_CXX = $(LLVM_TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)
endif

TARGET_CXXFLAGS = $(TARGET_CFLAGS) -fno-exceptions -fno-rtti

TARGET_RS_CC    = $(RENDERSCRIPT_TOOLCHAIN_PREFIX)llvm-rs-cc
TARGET_RS_BCC   = $(RENDERSCRIPT_TOOLCHAIN_PREFIX)bcc_compat
TARGET_RS_FLAGS = -Wall -Werror
ifeq (,$(findstring 64,$(TARGET_ARCH_ABI)))
TARGET_RS_FLAGS += -m32
else
TARGET_RS_FLAGS += -m64
endif

TARGET_ASM      = $(HOST_PREBUILT)/yasm
TARGET_ASMFLAGS =

TARGET_LD       = $(TOOLCHAIN_PREFIX)ld
TARGET_LDFLAGS :=

# Use *-gcc-ar instead of *-ar for better LTO support when using GCC.
ifeq (4.9,$(NDK_TOOLCHAIN_VERSION))
    TARGET_AR = $(TOOLCHAIN_PREFIX)gcc-ar
else
    TARGET_AR = $(TOOLCHAIN_PREFIX)ar
endif

TARGET_ARFLAGS := crsD

TARGET_STRIP    = $(TOOLCHAIN_PREFIX)strip

TARGET_OBJCOPY  = $(TOOLCHAIN_PREFIX)objcopy

TARGET_OBJ_EXTENSION := .o
TARGET_LIB_EXTENSION := .a
TARGET_SONAME_EXTENSION := .so
