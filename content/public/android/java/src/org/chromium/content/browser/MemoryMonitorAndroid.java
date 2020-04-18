// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.ActivityManager;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.res.Configuration;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Android implementation of MemoryMonitor.
 */
@JNINamespace("content")
class MemoryMonitorAndroid {
    private static final String TAG = "MemoryMonitorAndroid";
    private static final ActivityManager.MemoryInfo sMemoryInfo =
            new ActivityManager.MemoryInfo();
    private static ComponentCallbacks2 sCallbacks;

    private MemoryMonitorAndroid() {
    }

    /**
     * Get the current MemoryInfo from ActivityManager and invoke the native
     * callback to populate the MemoryInfo.
     *
     * @param outPtr A native output pointer to populate MemoryInfo. This is
     * passed back to the native callback.
     */
    @CalledByNative
    private static void getMemoryInfo(long outPtr) {
        ActivityManager am =
                (ActivityManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.ACTIVITY_SERVICE);
        try {
            am.getMemoryInfo(sMemoryInfo);
        } catch (RuntimeException e) {
            // RuntimeException can be thrown when the system is going to
            // restart. Pass arbitrary values to the callback.
            Log.e(TAG,
                    "Failed to get memory info due to a runtime exception: %s",
                    e);
            sMemoryInfo.availMem = 1;
            sMemoryInfo.lowMemory = true;
            sMemoryInfo.threshold = 1;
            sMemoryInfo.totalMem = 1;
        }
        nativeGetMemoryInfoCallback(
                sMemoryInfo.availMem, sMemoryInfo.lowMemory,
                sMemoryInfo.threshold, sMemoryInfo.totalMem, outPtr);
    }

    /**
     * Register ComponentCallbacks2 to receive memory pressure signals.
     *
     */
    @CalledByNative
    private static void registerComponentCallbacks() {
        sCallbacks = new ComponentCallbacks2() {
                @Override
                public void onTrimMemory(int level) {
                    if (level != ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN) {
                        nativeOnTrimMemory(level);
                    }
                }
                @Override
                public void onLowMemory() {
                    // Don't support old onLowMemory().
                }
                @Override
                public void onConfigurationChanged(Configuration config) {
                }
            };
        ContextUtils.getApplicationContext().registerComponentCallbacks(sCallbacks);
    }

    private static native void nativeGetMemoryInfoCallback(
            long availMem, boolean lowMemory,
            long threshold, long totalMem, long outPtr);

    private static native void nativeOnTrimMemory(int level);
}
