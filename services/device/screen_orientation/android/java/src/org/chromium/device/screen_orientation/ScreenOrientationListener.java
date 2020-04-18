// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.screen_orientation;

import android.provider.Settings;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.ui.display.DisplayAndroid;

/**
 * Android implementation details for device::ScreenOrientationListenerAndroid.
 */
@JNINamespace("device")
class ScreenOrientationListener {
    @CalledByNative
    static void startAccurateListening() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                DisplayAndroid.startAccurateListening();
            }
        });
    }

    @CalledByNative
    static void stopAccurateListening() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                DisplayAndroid.stopAccurateListening();
            }
        });
    }

    @CalledByNative
    static boolean isAutoRotateEnabledByUser() {
        return Settings.System.getInt(ContextUtils.getApplicationContext().getContentResolver(),
                       Settings.System.ACCELEROMETER_ROTATION, 0)
                == 1;
    }
}
