// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.services;

import android.content.Context;

import org.chromium.android_webview.command_line.CommandLineUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;

/**
 * Does initialization common to all WebView services.
 */
@VisibleForTesting
public class ServiceInit {
    private static boolean sInitDone;

    @VisibleForTesting
    public static void setPrivateDataDirectorySuffix() {
        // This is unrelated to the PathUtils directory set in WebView proper, because this code
        // runs only in the service process.
        PathUtils.setPrivateDataDirectorySuffix("webview");
    }

    public static void init(Context appContext) {
        ThreadUtils.assertOnUiThread();
        if (sInitDone) return;
        ContextUtils.initApplicationContext(appContext);
        // In Monochrome, ChromeApplication.attachBaseContext() will set Chrome's command line.
        // initCommandLine() overwrites this with WebView's command line.
        CommandLineUtil.initCommandLine();
        setPrivateDataDirectorySuffix();
        sInitDone = true;
    }
}
