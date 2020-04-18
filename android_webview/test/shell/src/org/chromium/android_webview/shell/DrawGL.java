// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.shell;

/**
 * Provides an entry point to the native draw functor.
 */
public class DrawGL {
    public static void drawGL(long drawGL, long viewContext, int width, int height,
            int scrollX, int scrollY, int mode) {
        nativeDrawGL(drawGL, viewContext, width, height, scrollX, scrollY, mode);
    }

    private static native void nativeDrawGL(long drawGL, long viewContext,
            int width, int height, int scrollX, int scrollY, int mode);
}
