// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.annotations.JNINamespace;

/**
 * Wrapper of native CastMetricsHelper.
 */
@JNINamespace("chromecast::shell")
public final class CastMetricsHelper {
    public static void logMediaPlay() {
        nativeLogMediaPlay();
    }
    private static native void nativeLogMediaPlay();

    public static void logMediaPause() {
        nativeLogMediaPause();
    }
    private static native void nativeLogMediaPause();

}
