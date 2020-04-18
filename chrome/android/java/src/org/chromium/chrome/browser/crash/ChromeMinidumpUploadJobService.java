// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.crash;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.PersistableBundle;

import org.chromium.components.minidump_uploader.MinidumpUploadJobService;
import org.chromium.components.minidump_uploader.MinidumpUploader;
import org.chromium.components.minidump_uploader.MinidumpUploaderImpl;

/**
 * Class that interacts with the Android JobScheduler to upload minidumps at appropriate times.
 */
@TargetApi(Build.VERSION_CODES.M)
public class ChromeMinidumpUploadJobService extends MinidumpUploadJobService {
    @Override
    protected MinidumpUploader createMinidumpUploader(PersistableBundle permissions) {
        return new MinidumpUploaderImpl(new ChromeMinidumpUploaderDelegate(this, permissions));
    }
}
