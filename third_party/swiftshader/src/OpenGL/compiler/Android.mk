LOCAL_PATH:= $(call my-dir)

COMMON_C_INCLUDES := \
	bionic \
	$(LOCAL_PATH)/../../../include \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../../ \
	$(LOCAL_PATH)/../../Renderer/ \
	$(LOCAL_PATH)/../../Common/ \
	$(LOCAL_PATH)/../../Shader/ \
	$(LOCAL_PATH)/../../Main/

# Marshmallow does not have stlport, but comes with libc++ by default
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 23 && echo PreMarshmallow),PreMarshmallow)
COMMON_C_INCLUDES += \
	$(LOCAL_PATH)/../../../third_party/stlport-cpp11-extension/ \
	external/stlport/stlport/ \
	external/stlport/
endif

COMMON_CFLAGS := \
	-DLOG_TAG=\"swiftshader_compiler\" \
	-Wall \
	-Werror \
	-Wno-format \
	-Wno-sign-compare \
	-Wno-unneeded-internal-declaration \
	-Wno-unused-const-variable \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-Wno-implicit-exception-spec-mismatch \
	-Wno-overloaded-virtual \
	-Wno-attributes \
	-Wno-unknown-attributes \
	-Wno-unknown-warning-option \
	-fno-operator-names \
	-msse2 \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-std=c++11 \
	-DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifneq (16,${PLATFORM_SDK_VERSION})
COMMON_CFLAGS += -Xclang -fuse-init-array
else
COMMON_CFLAGS += -D__STDC_INT64__
endif

COMMON_SRC_FILES := \
	preprocessor/DiagnosticsBase.cpp \
	preprocessor/DirectiveHandlerBase.cpp \
	preprocessor/DirectiveParser.cpp \
	preprocessor/ExpressionParser.cpp \
	preprocessor/Input.cpp \
	preprocessor/Lexer.cpp \
	preprocessor/Macro.cpp \
	preprocessor/MacroExpander.cpp \
	preprocessor/Preprocessor.cpp \
	preprocessor/Token.cpp \
	preprocessor/Tokenizer.cpp \
	AnalyzeCallDepth.cpp \
	Compiler.cpp \
	debug.cpp \
	Diagnostics.cpp \
	DirectiveHandler.cpp \
	glslang_lex.cpp \
	glslang_tab.cpp \
	InfoSink.cpp \
	Initialize.cpp \
	InitializeParseContext.cpp \
	IntermTraverse.cpp \
	Intermediate.cpp \
	intermOut.cpp \
	ossource_posix.cpp \
	OutputASM.cpp \
	parseConst.cpp \
	ParseHelper.cpp \
	PoolAlloc.cpp \
	SymbolTable.cpp \
	TranslatorASM.cpp \
	util.cpp \
	ValidateLimitations.cpp \
	ValidateSwitch.cpp \

# liblog_headers is introduced from O MR1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 27 && echo OMR1),OMR1)
COMMON_HEADER_LIBRARIES := liblog_headers
else
COMMON_HEADER_LIBRARIES :=
endif

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := swiftshader_compiler_release
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)
LOCAL_CFLAGS += \
	$(COMMON_CFLAGS) \
	-ffunction-sections \
	-fdata-sections \
	-DANGLE_DISABLE_TRACE
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := swiftshader_compiler_debug
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)

LOCAL_CFLAGS += \
	$(COMMON_CFLAGS) \
	-UNDEBUG \
	-g \
	-O0

LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
include $(BUILD_STATIC_LIBRARY)
