// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import android.content.Context;
import android.os.Environment;
import android.os.StatFs;
import android.util.Pair;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.CollectionUtil;
import org.chromium.base.LocaleUtils;
import org.chromium.base.annotations.JNINamespace;

import java.io.File;
import java.util.Map;

/** Grabs feedback about the current system. */
@JNINamespace("chrome::android")
public class SystemInfoFeedbackSource extends AsyncFeedbackSourceAdapter<StatFs> {
    // AsyncFeedbackSourceAdapter implementation.
    @Override
    protected StatFs doInBackground(Context context) {
        File directory = Environment.getDataDirectory();
        if (!directory.exists()) return null;

        return new StatFs(directory.getPath());
    }

    @Override
    public Map<String, String> getFeedback() {
        Map<String, String> feedback = CollectionUtil.newHashMap(
                Pair.create("CPU Architecture", nativeGetCpuArchitecture()),
                Pair.create(
                        "Available Memory (MB)", Integer.toString(nativeGetAvailableMemoryMB())),
                Pair.create("Total Memory (MB)", Integer.toString(nativeGetTotalMemoryMB())),
                Pair.create("GPU Vendor", nativeGetGpuVendor()),
                Pair.create("GPU Model", nativeGetGpuModel()),
                Pair.create("UI Locale", LocaleUtils.getDefaultLocaleString()));

        StatFs statFs = getResult();
        if (statFs != null) {
            long blockSize = ApiCompatibilityUtils.getBlockSize(statFs);
            long availSpace =
                    ApiCompatibilityUtils.getAvailableBlocks(statFs) * blockSize / 1024 / 1024;
            long totalSpace = ApiCompatibilityUtils.getBlockCount(statFs) * blockSize / 1024 / 1024;

            feedback.put("Available Storage (MB)", Long.toString(availSpace));
            feedback.put("Total Storage (MB)", Long.toString(totalSpace));
        }

        return feedback;
    }

    private static native String nativeGetCpuArchitecture();
    private static native String nativeGetGpuVendor();
    private static native String nativeGetGpuModel();
    private static native int nativeGetAvailableMemoryMB();
    private static native int nativeGetTotalMemoryMB();
}