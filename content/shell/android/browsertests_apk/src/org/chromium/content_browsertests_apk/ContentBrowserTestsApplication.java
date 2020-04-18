// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_browsertests_apk;

import android.app.Application;
import android.content.Context;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BuildConfig;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.multidex.ChromiumMultiDexInstaller;

/**
 * A basic content browser tests {@link android.app.Application}.
 */
public class ContentBrowserTestsApplication extends Application {
    static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "content_shell";

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        ContextUtils.initApplicationContext(this);
        // content_browsertests needs secondary dex in order to run EmbeddedTestServer in a
        // privileged process.
        if (BuildConfig.IS_MULTIDEX_ENABLED) {
            ChromiumMultiDexInstaller.install(this);
        }
        // The test harness runs in the main process, and browser in :test_process.
        if (ContextUtils.getProcessName().contains(":test")) {
            PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
            ApplicationStatus.initialize(this);
        }
    }
}
