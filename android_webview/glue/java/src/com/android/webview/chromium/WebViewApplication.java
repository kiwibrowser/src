// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.app.Application;

/**
 * Application subclass for standalone WebView, to allow installing the font preloading workaround
 * in renderer processes before the problematic code in ActivityThread runs.
 *
 * DO NOT ADD ANYTHING ELSE HERE! WebView's application subclass is only used in renderer processes
 * and in the WebView APK's own services. None of this code runs in an application which simply uses
 * WebView. This only exists because this is the only point where the font preloading workaround can
 * be installed early enough to be effective.
 */
public class WebViewApplication extends Application {
    @Override
    public void onCreate() {
        // Don't add anything else here! See above.
        super.onCreate();
        FontPreloadingWorkaround.maybeInstallWorkaround(this);
    }
}
