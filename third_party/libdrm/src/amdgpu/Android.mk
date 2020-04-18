LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Import variables LIBDRM_AMDGPU_FILES, LIBDRM_AMDGPU_H_FILES
include $(LOCAL_PATH)/Makefile.sources

LOCAL_MODULE := libdrm_amdgpu

LOCAL_SHARED_LIBRARIES := libdrm

LOCAL_SRC_FILES := $(LIBDRM_AMDGPU_FILES)

LOCAL_CFLAGS := \
	-DAMDGPU_ASIC_ID_TABLE=\"/vendor/etc/hwdata/amdgpu.ids\" \
	-DAMDGPU_ASIC_ID_TABLE_NUM_ENTRIES=$(shell egrep -ci '^[0-9a-f]{4},.*[0-9a-f]+,' $(LIBDRM_TOP)/data/amdgpu.ids)

LOCAL_REQUIRED_MODULES := amdgpu.ids

include $(LIBDRM_COMMON_MK)
include $(BUILD_SHARED_LIBRARY)
