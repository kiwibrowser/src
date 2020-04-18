// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webview_shell;

import android.app.Activity;
import android.os.Bundle;
import android.os.SystemClock;
import android.webkit.WebView;

/**
 * This activity is designed for startup time testing of the WebView.
 */
public class StartupTimeActivity extends Activity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setTitle(
                getResources().getString(R.string.title_activity_startup_time));

        long t1 = SystemClock.elapsedRealtime();
        WebView webView = new WebView(this);
        setContentView(webView);
        long t2 = SystemClock.elapsedRealtime();
        android.util.Log.i("WebViewShell", "WebViewStartupTimeMillis=" + (t2 - t1));
    }

}

