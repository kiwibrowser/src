#
# Copyright 2016 The Android Open-Source Project
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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_MODULE := libsubzero
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true

SUBZERO_PATH := ../../third_party/subzero
LLVMDEPENDENCIES_PATH := ../../third_party/llvm-subzero

LOCAL_SRC_FILES := \
	$(SUBZERO_PATH)/src/IceAssembler.cpp \
	$(SUBZERO_PATH)/src/IceCfg.cpp \
	$(SUBZERO_PATH)/src/IceCfgNode.cpp \
	$(SUBZERO_PATH)/src/IceClFlags.cpp \
	$(SUBZERO_PATH)/src/IceELFObjectWriter.cpp \
	$(SUBZERO_PATH)/src/IceELFSection.cpp \
	$(SUBZERO_PATH)/src/IceFixups.cpp \
	$(SUBZERO_PATH)/src/IceGlobalContext.cpp \
	$(SUBZERO_PATH)/src/IceGlobalInits.cpp \
	$(SUBZERO_PATH)/src/IceInst.cpp \
	$(SUBZERO_PATH)/src/IceInstrumentation.cpp \
	$(SUBZERO_PATH)/src/IceIntrinsics.cpp \
	$(SUBZERO_PATH)/src/IceLiveness.cpp \
	$(SUBZERO_PATH)/src/IceLoopAnalyzer.cpp \
	$(SUBZERO_PATH)/src/IceMangling.cpp \
	$(SUBZERO_PATH)/src/IceMemory.cpp \
	$(SUBZERO_PATH)/src/IceOperand.cpp \
	$(SUBZERO_PATH)/src/IceRangeSpec.cpp \
	$(SUBZERO_PATH)/src/IceRegAlloc.cpp \
	$(SUBZERO_PATH)/src/IceRevision.cpp \
	$(SUBZERO_PATH)/src/IceRNG.cpp \
	$(SUBZERO_PATH)/src/IceSwitchLowering.cpp \
	$(SUBZERO_PATH)/src/IceTargetLowering.cpp \
	$(SUBZERO_PATH)/src/IceThreading.cpp \
	$(SUBZERO_PATH)/src/IceTimerTree.cpp \
	$(SUBZERO_PATH)/src/IceTypes.cpp \
	$(SUBZERO_PATH)/src/IceVariableSplitting.cpp

LOCAL_SRC_FILES_x86 += \
	$(SUBZERO_PATH)/src/IceInstX8632.cpp \
	$(SUBZERO_PATH)/src/IceTargetLoweringX8632.cpp
LOCAL_SRC_FILES_x86_64 += \
	$(SUBZERO_PATH)/src/IceInstX8664.cpp \
	$(SUBZERO_PATH)/src/IceTargetLoweringX8664.cpp
LOCAL_SRC_FILES_arm += \
	$(SUBZERO_PATH)/src/IceAssemblerARM32.cpp \
	$(SUBZERO_PATH)/src/IceTargetLoweringARM32.cpp \
	$(SUBZERO_PATH)/src/IceInstARM32.cpp

LOCAL_SRC_FILES += \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/APInt.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Atomic.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/circular_raw_ostream.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/CommandLine.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/ConvertUTF.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/ConvertUTFWrapper.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Debug.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/ErrorHandling.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/FoldingSet.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Hashing.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Host.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/ManagedStatic.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/MemoryBuffer.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Mutex.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/NativeFormatting.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Path.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Process.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Program.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/raw_ostream.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/raw_os_ostream.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/regcomp.c \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/regerror.c \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Regex.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/regexec.c \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/regfree.c \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/regstrlcpy.c \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Signals.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/SmallPtrSet.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/SmallVector.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/StringExtras.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/StringMap.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/StringRef.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/StringSaver.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/TargetParser.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Threading.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Timer.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Triple.cpp \
	$(LLVMDEPENDENCIES_PATH)/lib/Support/Twine.cpp

LOCAL_CPPFLAGS := -std=c++11

LOCAL_CFLAGS += \
	-DLOG_TAG=\"libsubzero\" \
	-Wall \
	-Werror \
	-Wno-error=undefined-var-template \
	-Wno-error=unused-lambda-capture \
	-Wno-unused-parameter \
	-Wno-implicit-exception-spec-mismatch \
	-Wno-overloaded-virtual \
	-Wno-non-virtual-dtor \
	-Wno-unknown-warning-option

ifneq (16,${PLATFORM_SDK_VERSION})
LOCAL_CFLAGS += -Xclang -fuse-init-array
else
LOCAL_CFLAGS += -D__STDC_INT64__
endif

LOCAL_CFLAGS += -fomit-frame-pointer -Os -ffunction-sections -fdata-sections
LOCAL_CFLAGS += -fno-operator-names -msse2 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS

# Common Subzero defines
LOCAL_CFLAGS += -DALLOW_DUMP=0 -DALLOW_TIMERS=0 -DALLOW_LLVM_CL=0 -DALLOW_LLVM_IR=0 -DALLOW_LLVM_IR_AS_INPUT=0 -DALLOW_MINIMAL_BUILD=0 -DALLOW_WASM=0 -DICE_THREAD_LOCAL_HACK=1

# Subzero target
LOCAL_CFLAGS_x86 += -DSZTARGET=X8632
LOCAL_CFLAGS_x86_64 += -DSZTARGET=X8664
LOCAL_CFLAGS_arm += -DSZTARGET=ARM32

# Android's make system also uses NDEBUG, so we need to set/unset it forcefully
# Uncomment for debug ON:
# LOCAL_CFLAGS += -UNDEBUG -g -O0

LOCAL_C_INCLUDES += \
	bionic \
	$(LOCAL_PATH)/$(SUBZERO_PATH)/ \
	$(LOCAL_PATH)/$(LLVMDEPENDENCIES_PATH)/include/ \
	$(LOCAL_PATH)/$(LLVMDEPENDENCIES_PATH)/build/Android/include/ \
	$(LOCAL_PATH)/$(SUBZERO_PATH)/pnacl-llvm/include/

# Marshmallow does not have stlport, but comes with libc++ by default
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 23 && echo PreMarshmallow),PreMarshmallow)
LOCAL_C_INCLUDES += external/stlport/stlport
endif

include $(BUILD_STATIC_LIBRARY)
