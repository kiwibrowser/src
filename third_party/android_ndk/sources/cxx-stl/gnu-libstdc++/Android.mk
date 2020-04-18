LOCAL_PATH := $(call my-dir)

# Compute the compiler flags to export by the module.
# This is controlled by the APP_GNUSTL_FORCE_CPP_FEATURES variable.
# See docs/APPLICATION-MK.html for all details.
#
gnustl_exported_cppflags := $(strip \
  $(if $(filter exceptions,$(APP_GNUSTL_FORCE_CPP_FEATURES)),-fexceptions)\
  $(if $(filter rtti,$(APP_GNUSTL_FORCE_CPP_FEATURES)),-frtti))

# Include path to export
gnustl_exported_c_includes := \
  $(LOCAL_PATH)/4.9/include \
  $(LOCAL_PATH)/4.9/libs/$(TARGET_ARCH_ABI)/include \
  $(LOCAL_PATH)/4.9/include/backward

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_static
LOCAL_SRC_FILES := 4.9/libs/$(TARGET_ARCH_ABI)/libgnustl_static$(TARGET_LIB_EXTENSION)
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(gnustl_exported_c_includes)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_shared
LOCAL_SRC_FILES := 4.9/libs/$(TARGET_ARCH_ABI)/libgnustl_shared$(TARGET_SONAME_EXTENSION)
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(gnustl_exported_c_includes)
LOCAL_EXPORT_LDLIBS := $(call host-path,$(LOCAL_PATH)/4.9/libs/$(TARGET_ARCH_ABI)/libsupc++$(TARGET_LIB_EXTENSION))
include $(PREBUILT_SHARED_LIBRARY)
