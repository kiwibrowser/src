LOCAL_PATH:= $(call my-dir)

COMMON_C_INCLUDES += \
	bionic \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/OpenGL/ \
	$(LOCAL_PATH)

ifdef use_subzero
COMMON_C_INCLUDES += \
	$(LOCAL_PATH)/../third_party/subzero/ \
	$(LOCAL_PATH)/../third_party/llvm-subzero/include/ \
	$(LOCAL_PATH)/../third_party/llvm-subzero/build/Android/include/ \
	$(LOCAL_PATH)/../third_party/subzero/pnacl-llvm/include/
else
COMMON_C_INCLUDES += \
	$(LOCAL_PATH)/../third_party/LLVM/include
endif

# Project Treble is introduced from Oreo MR1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 27 && echo OreoMR1),OreoMR1)
COMMON_SHARED_LIBRARIES := libnativewindow liblog
COMMON_HEADER_LIBRARIES := libhardware_headers libnativebase_headers
COMMON_STATIC_LIBRARIES := libarect
endif

# Marshmallow does not have stlport, but comes with libc++ by default
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 23 && echo PreMarshmallow),PreMarshmallow)
COMMON_C_INCLUDES += external/stlport/stlport
endif

COMMON_SRC_FILES := \
	Common/CPUID.cpp \
	Common/Configurator.cpp \
	Common/DebugAndroid.cpp \
	Common/GrallocAndroid.cpp \
	Common/Half.cpp \
	Common/Math.cpp \
	Common/Memory.cpp \
	Common/Resource.cpp \
	Common/Socket.cpp \
	Common/Thread.cpp \
	Common/Timer.cpp

COMMON_SRC_FILES += \
	Main/Config.cpp \
	Main/FrameBuffer.cpp \
	Main/FrameBufferAndroid.cpp \
	Main/SwiftConfig.cpp

ifdef use_subzero
COMMON_SRC_FILES += \
	Reactor/SubzeroReactor.cpp \
	Reactor/Routine.cpp \
	Reactor/Optimizer.cpp
else
COMMON_SRC_FILES += \
	Reactor/LLVMReactor.cpp \
	Reactor/Routine.cpp \
	Reactor/LLVMRoutine.cpp \
	Reactor/LLVMRoutineManager.cpp
endif

COMMON_SRC_FILES += \
	Renderer/Blitter.cpp \
	Renderer/Clipper.cpp \
	Renderer/Color.cpp \
	Renderer/Context.cpp \
	Renderer/ETC_Decoder.cpp \
	Renderer/Matrix.cpp \
	Renderer/PixelProcessor.cpp \
	Renderer/Plane.cpp \
	Renderer/Point.cpp \
	Renderer/QuadRasterizer.cpp \
	Renderer/Renderer.cpp \
	Renderer/Sampler.cpp \
	Renderer/SetupProcessor.cpp \
	Renderer/Surface.cpp \
	Renderer/TextureStage.cpp \
	Renderer/Vector.cpp \
	Renderer/VertexProcessor.cpp \

COMMON_SRC_FILES += \
	Shader/Constants.cpp \
	Shader/PixelPipeline.cpp \
	Shader/PixelProgram.cpp \
	Shader/PixelRoutine.cpp \
	Shader/PixelShader.cpp \
	Shader/SamplerCore.cpp \
	Shader/SetupRoutine.cpp \
	Shader/Shader.cpp \
	Shader/ShaderCore.cpp \
	Shader/VertexPipeline.cpp \
	Shader/VertexProgram.cpp \
	Shader/VertexRoutine.cpp \
	Shader/VertexShader.cpp \

COMMON_SRC_FILES += \
	OpenGL/common/Image.cpp \
	OpenGL/common/Object.cpp \
	OpenGL/common/MatrixStack.cpp \

COMMON_CFLAGS := \
	-DLOG_TAG=\"swiftshader\" \
	-Wall \
	-Werror \
	-Wno-format \
	-Wno-switch \
	-Wno-unused-local-typedef \
	-Wno-unused-parameter \
	-Wno-unused-value \
	-Wno-unused-variable \
	-Wno-implicit-exception-spec-mismatch \
	-Wno-overloaded-virtual \
	-Wno-non-virtual-dtor \
	-Wno-attributes \
	-Wno-unknown-attributes \
	-Wno-unknown-warning-option \
	-fno-operator-names \
	-msse2 \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) \
	-std=c++11 \
	-DNO_SANITIZE_FUNCTION=

ifneq (16,${PLATFORM_SDK_VERSION})
COMMON_CFLAGS += -Xclang -fuse-init-array
else
COMMON_CFLAGS += -D__STDC_INT64__
endif

# gralloc1 is introduced from N MR1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 25 && echo NMR1),NMR1)
COMMON_CFLAGS += -DHAVE_GRALLOC1
COMMON_C_INCLUDES += \
	system/core/libsync/include \
	system/core/libsync
endif

# Common Subzero defines
COMMON_CFLAGS += -DALLOW_DUMP=0 -DALLOW_TIMERS=0 -DALLOW_LLVM_CL=0 -DALLOW_LLVM_IR=0 -DALLOW_LLVM_IR_AS_INPUT=0 -DALLOW_MINIMAL_BUILD=0 -DALLOW_WASM=0 -DICE_THREAD_LOCAL_HACK=1

# Subzero target
LOCAL_CFLAGS_x86 += -DSZTARGET=X8632
LOCAL_CFLAGS_x86_64 += -DSZTARGET=X8664
LOCAL_CFLAGS_arm += -DSZTARGET=ARM32

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := swiftshader_top_release
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)
LOCAL_CFLAGS := $(COMMON_CFLAGS) -fomit-frame-pointer -ffunction-sections -fdata-sections -DANGLE_DISABLE_TRACE
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_SHARED_LIBRARIES := $(COMMON_SHARED_LIBRARIES)
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
LOCAL_STATIC_LIBRARIES := $(COMMON_STATIC_LIBRARIES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := swiftshader_top_debug
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)
LOCAL_CFLAGS := $(COMMON_CFLAGS) -UNDEBUG -g -O0 -DDEFAULT_THREAD_COUNT=1
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_SHARED_LIBRARIES := $(COMMON_SHARED_LIBRARIES)
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
LOCAL_STATIC_LIBRARIES := $(COMMON_STATIC_LIBRARIES)
include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
