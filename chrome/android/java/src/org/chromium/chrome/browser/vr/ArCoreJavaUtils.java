// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr;

import android.content.Context;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Provides ARCore classes access to java-related app functionality.
 */
@JNINamespace("vr")
public class ArCoreJavaUtils {
    private static final int MIN_SDK_VERSION = Build.VERSION_CODES.N;

    /**
     * Gets the current application context.
     *
     * @return Context The application context.
     */
    @CalledByNative
    private static Context getApplicationContext() {
        return ContextUtils.getApplicationContext();
    }

    /**
     * Determines whether ARCore's SDK should be loaded. Currently, this only
     * depends on the OS version, but could be more sophisticated.
     *
     * @return true if the SDK should be loaded.
     */
    @CalledByNative
    private static boolean shouldLoadArCoreSdk() {
        return Build.VERSION.SDK_INT >= MIN_SDK_VERSION;
    }
}
