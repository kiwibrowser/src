// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.os.Bundle;
import android.os.SystemClock;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

/**
 * WebAPK's main Activity.
 */
public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        long activityStartTime = SystemClock.elapsedRealtime();
        super.onCreate(savedInstanceState);

        Bundle metadata = WebApkUtils.readMetaData(this);
        if (metadata == null) {
            finish();
            return;
        }

        String overrideUrl = getOverrideUrl();
        String startUrl = (overrideUrl != null) ? overrideUrl
                                                : metadata.getString(WebApkMetaDataKeys.START_URL);
        if (startUrl == null) {
            finish();
            return;
        }

        startUrl = WebApkUtils.rewriteIntentUrlIfNecessary(startUrl, metadata);

        boolean overrideSpecified = (overrideUrl != null);
        int source = getIntent().getIntExtra(WebApkConstants.EXTRA_SOURCE,
                overrideSpecified ? WebApkConstants.SHORTCUT_SOURCE_EXTERNAL_INTENT
                                  : WebApkConstants.SHORTCUT_SOURCE_UNKNOWN);
        // The override URL is non null when the WebAPK is launched from a deep link. The WebAPK
        // should navigate to the URL in the deep link even if the WebAPK is already open.
        boolean forceNavigation = getIntent().getBooleanExtra(
                WebApkConstants.EXTRA_FORCE_NAVIGATION, overrideSpecified);

        HostBrowserLauncher launcher = new HostBrowserLauncher(
                this, getIntent(), startUrl, source, forceNavigation, activityStartTime);
        launcher.selectHostBrowserAndLaunch(() -> finish());
    }

    /** Retrieves URL from the intent's data. Returns null if a URL could not be retrieved. */
    private String getOverrideUrl() {
        String overrideUrl = getIntent().getDataString();
        if (overrideUrl != null
                && (overrideUrl.startsWith("https:") || overrideUrl.startsWith("http:"))) {
            return overrideUrl;
        }
        return null;
    }
}
