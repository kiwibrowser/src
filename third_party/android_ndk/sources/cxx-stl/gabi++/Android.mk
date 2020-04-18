LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/sources.mk

ifeq (,$(GABIXX_FORCE_REBUILD))

  include $(CLEAR_VARS)
  LOCAL_MODULE:= gabi++_shared
  LOCAL_SRC_FILES:= libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
  # For armeabi*, choose thumb mode unless LOCAL_ARM_MODE := arm
  ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
  ifneq (arm,$(LOCAL_ARM_MODE))
  LOCAL_SRC_FILES:= libs/$(TARGET_ARCH_ABI)/thumb/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
  endif
  endif
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti exceptions
  LOCAL_CFLAGS := -Wall -Werror
  include $(PREBUILT_SHARED_LIBRARY)

  include $(CLEAR_VARS)
  LOCAL_MODULE:= gabi++_static
  LOCAL_SRC_FILES:= libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
  # For armeabi*, choose thumb mode unless LOCAL_ARM_MODE := arm
  ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
  ifneq (arm,$(LOCAL_ARM_MODE))
  LOCAL_SRC_FILES:= libs/$(TARGET_ARCH_ABI)/thumb/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
  endif
  endif
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti exceptions
  LOCAL_CFLAGS := -Wall -Werror
  include $(PREBUILT_STATIC_LIBRARY)

else # ! GABIXX_FORCE_REBUILD

  # Shared version of the library
  # Note that the module is named libgabi++_shared to avoid
  # any conflict with any potential system library named libgabi++
  #
  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_shared
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_SRC_FILES:= $(libgabi++_src_files)
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti exceptions
  include $(BUILD_SHARED_LIBRARY)

  # And now the static version
  #
  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_static
  LOCAL_SRC_FILES:= $(libgabi++_src_files)
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti exceptions
  include $(BUILD_STATIC_LIBRARY)

endif # ! GABIXX_FORCE_REBUILD
