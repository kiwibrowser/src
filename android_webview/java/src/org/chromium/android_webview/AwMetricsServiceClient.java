// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Determines user consent and app opt-out for metrics.
 *
 * This requires the following steps:
 * 1) Check the platform's metrics consent setting.
 * 2) Check if the app has opted out.
 * 3) Wait for the native AwMetricsServiceClient to call nativeInitialized.
 * 4) If enabled, inform the native AwMetricsServiceClient via nativeSetHaveMetricsConsent.
 *
 * Step 1 is done asynchronously and the result is passed to setConsentSetting, which does step 2.
 * This happens in parallel with native AwMetricsServiceClient initialization; either
 * nativeInitialized or setConsentSetting might fire first. Whichever fires second should call
 * nativeSetHaveMetricsConsent.
 *
 * Also, pre-loads the client ID for variations. Variations setup begins before native init (so it
 * can't use the native code to load the client ID), reading the client ID and other variations
 * files in the background. Completion of this task blocks WebView startup, so we want to do minimal
 * work; unlike the native loader, this loader does not create a new client ID when none is found.
 */
@JNINamespace("android_webview")
public class AwMetricsServiceClient {
    private static final String TAG = "AwMetricsServiceCli-";

    // Individual apps can use this meta-data tag in their manifest to opt out of metrics
    // reporting. See https://developer.android.com/reference/android/webkit/WebView.html
    private static final String OPT_OUT_META_DATA_STR = "android.webkit.WebView.MetricsOptOut";

    private static boolean sIsClientReady; // Is the native AwMetricsServiceClient initialized?
    private static boolean sShouldEnable; // Have steps 1 and 2 passed?

    // A GUID in text form is composed of 32 hex digits and 4 hyphens. These values must match those
    // in aw_metrics_service_client.cc.
    private static final int GUID_SIZE = 32 + 4;
    private static final String GUID_FILE_NAME = "metrics_guid";

    private static byte[] sClientId;
    private static Object sClientIdLock = new Object();

    private static byte[] readFixedLengthFile(File file, int length) throws IOException {
        if (file.length() != length) {
            throw new IOException("File is not of expected length " + length);
        }
        FileInputStream in = null;
        try {
            in = new FileInputStream(file);
            byte[] buf = new byte[length];
            int read = 0;
            int offset = 0;
            while (offset < length) {
                read = in.read(buf, offset, length - offset);
                if (read < 1) throw new IOException("Premature EOF");
                offset += read;
            }
            return buf;
        } finally {
            if (in != null) in.close();
        }
    }

    /**
     * Load the metrics client ID, if any.
     */
    public static void preloadClientId() {
        File clientIdFile = new File(PathUtils.getDataDirectory(), GUID_FILE_NAME);
        if (!clientIdFile.exists() || clientIdFile.length() != GUID_SIZE) return;
        byte[] clientId = null;
        try {
            clientId = readFixedLengthFile(clientIdFile, GUID_SIZE);
        } catch (IOException e) {
            Log.e(TAG, "Failed to pre-load GUID file " + clientIdFile + " - " + e.getMessage());
            return;
        }
        synchronized (sClientIdLock) {
            sClientId = clientId;
        }
    }

    @CalledByNative
    public static byte[] getPreloadedClientId() {
        synchronized (sClientIdLock) {
            return sClientId;
        }
    }

    private static boolean isAppOptedOut(Context appContext) {
        try {
            ApplicationInfo info = appContext.getPackageManager().getApplicationInfo(
                    appContext.getPackageName(), PackageManager.GET_META_DATA);
            if (info.metaData == null) {
                // null means no such tag was found.
                return false;
            }
            // getBoolean returns false if the key is not found, which is what we want.
            return info.metaData.getBoolean(OPT_OUT_META_DATA_STR);
        } catch (PackageManager.NameNotFoundException e) {
            // This should never happen.
            Log.e(TAG, "App could not find itself by package name!");
            // The conservative thing is to assume the app HAS opted out.
            return true;
        }
    }

    public static void setConsentSetting(Context appContext, boolean userConsent) {
        ThreadUtils.assertOnUiThread();

        if (!userConsent || isAppOptedOut(appContext)) {
            // Metrics defaults to off, so no need to call nativeSetHaveMetricsConsent(false).
            return;
        }

        sShouldEnable = true;
        if (sIsClientReady) {
            nativeSetHaveMetricsConsent(true);
        }
    }

    @CalledByNative
    public static void nativeInitialized() {
        ThreadUtils.assertOnUiThread();
        sIsClientReady = true;
        if (sShouldEnable) {
            nativeSetHaveMetricsConsent(true);
        }
    }

    public static native void nativeSetHaveMetricsConsent(boolean enabled);
}
