// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include "android_webview/public/browser/draw_gl.h"

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  return JNI_VERSION_1_4;
}

// This code goes into its own dynamic library, so we cannot depend on
// any other components like base.
JNIEXPORT void JNICALL
    Java_org_chromium_android_1webview_shell_DrawGL_nativeDrawGL(
        JNIEnv*,
        jclass,
        jlong draw_gl,
        jlong view,
        jint width,
        jint height,
        jint scroll_x,
        jint scroll_y,
        jint mode) {
   AwDrawGLInfo draw_info;
   draw_info.mode = static_cast<AwDrawGLInfo::Mode>(mode);
   draw_info.version = kAwDrawGLInfoVersion;
   draw_info.is_layer = false;
   draw_info.width = width;
   draw_info.height = height;
   draw_info.clip_left = 0;
   draw_info.clip_top = 0;
   draw_info.clip_bottom = height;
   draw_info.clip_right = width;
   draw_info.transform[0] = 1.0;
   draw_info.transform[1] = 0.0;
   draw_info.transform[2] = 0.0;
   draw_info.transform[3] = 0.0;

   draw_info.transform[4] = 0.0;
   draw_info.transform[5] = 1.0;
   draw_info.transform[6] = 0.0;
   draw_info.transform[7] = 0.0;

   draw_info.transform[8] = 0.0;
   draw_info.transform[9] = 0.0;
   draw_info.transform[10] = 1.0;
   draw_info.transform[11] = 0.0;

   draw_info.transform[12] = -scroll_x;
   draw_info.transform[13] = -scroll_y;
   draw_info.transform[14] = 0.0;
   draw_info.transform[15] = 1.0;
   AwDrawGLFunction* draw_func = reinterpret_cast<AwDrawGLFunction*>(draw_gl);
   draw_func(view, &draw_info, 0);
}

}
