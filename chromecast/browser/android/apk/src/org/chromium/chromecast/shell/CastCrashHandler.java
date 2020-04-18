// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chromecast.base.ChromecastConfigAndroid;

/**
 * JNI wrapper class for accessing CastCrashHandler.
 */
@JNINamespace("chromecast")
public final class CastCrashHandler {
    private static final String TAG = "cr_CastCrashHandler";

    @CalledByNative
    public static void initializeUploader(String crashDumpPath, String uuid,
            String applicationFeedback, boolean uploadCrashToStaging, boolean periodicUpload) {
        CastCrashUploader uploader = CastCrashUploaderFactory.createCastCrashUploader(
                crashDumpPath, uuid, applicationFeedback, uploadCrashToStaging);
        if (ChromecastConfigAndroid.canSendUsageStats()) {
            if (periodicUpload) {
                uploader.startPeriodicUpload();
            } else {
                uploader.uploadOnce();
            }
        } else {
            Log.d(TAG, "Removing crash dumps instead of uploading");
            uploader.removeCrashDumps();
        }
    }
}
