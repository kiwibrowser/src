// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import android.content.Context;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * JNI wrapper class for calls from ChromecastConfigAndroid.
 */
@JNINamespace("chromecast::android")
public final class ChromecastConfigAndroid {

    private static CastSettingsManager sSettingsManager;

    public static void initializeForBrowser(Context context) {
        sSettingsManager = CastSettingsManager.createCastSettingsManager(
                context, new CastSettingsManager.OnSettingChangedListener() {
                    @Override
                    public void onSendUsageStatsChanged(boolean enabled) {
                        nativeSetSendUsageStatsEnabled(enabled);
                    }
                });
    }

    @CalledByNative
    public static boolean canSendUsageStats() {
        return sSettingsManager.isSendUsageStatsEnabled();
    }

    @CalledByNative
    public static void setSendUsageStats(boolean enabled) {
        sSettingsManager.setSendUsageStatsEnabled(enabled);
    }

    private static native void nativeSetSendUsageStatsEnabled(boolean enabled);
}
