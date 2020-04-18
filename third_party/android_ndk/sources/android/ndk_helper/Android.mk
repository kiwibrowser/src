LOCAL_PATH:= $(call my-dir)

$(warning ndk_helper is no longer maintained in the NDK. This copy is left for \
          compatibility purposes only. For an up to date copy, see \
          https://github.com/googlesamples/android-ndk/tree/master/teapots/common/ndk_helper)

include $(CLEAR_VARS)

LOCAL_MODULE:= ndk_helper
LOCAL_SRC_FILES:= JNIHelper.cpp interpolator.cpp tapCamera.cpp gestureDetector.cpp perfMonitor.cpp vecmath.cpp GLContext.cpp shader.cpp gl3stub.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS    := -llog -landroid -lEGL -lGLESv2

LOCAL_STATIC_LIBRARIES := cpufeatures android_native_app_glue

include $(BUILD_STATIC_LIBRARY)

#$(call import-module,android/native_app_glue)
#$(call import-module,android/cpufeatures)
