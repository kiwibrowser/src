// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

abstract class GraphicsUtils {
    public static long getDrawSWFunctionTable() {
        return nativeGetDrawSWFunctionTable();
    }

    public static long getDrawGLFunctionTable() {
        return nativeGetDrawGLFunctionTable();
    }

    private static native long nativeGetDrawSWFunctionTable();
    private static native long nativeGetDrawGLFunctionTable();
}
