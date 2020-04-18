// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.webview_shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Trace;
import android.webkit.CookieManager;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/**
 * This activity is designed for Telemetry testing of WebView.
 */
public class TelemetryActivity extends Activity {
    static final String DEFAULT_START_UP_TRACE_TAG = "WebViewStartupInterval";
    static final String DEFAULT_LOAD_URL_TRACE_TAG = "WebViewBlankUrlLoadInterval";
    static final String DEFAULT_START_UP_AND_LOAD_URL_TRACE_TAG =
            "WebViewStartupAndLoadBlankUrlInterval";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setTitle(
                getResources().getString(R.string.title_activity_telemetry));

        Intent intent = getIntent();
        final String startUpTraceTag = intent.getStringExtra("WebViewStartUpTraceTag");
        final String loadUrlTraceTag = intent.getStringExtra("WebViewLoadUrlTraceTag");
        final String startUpAndLoadUrlTraceTag =
                intent.getStringExtra("WebViewStartUpAndLoadUrlTraceTag");

        Trace.beginSection(startUpTraceTag == null ? DEFAULT_START_UP_AND_LOAD_URL_TRACE_TAG
                                                   : startUpAndLoadUrlTraceTag);
        Trace.beginSection(startUpTraceTag == null ? DEFAULT_START_UP_TRACE_TAG : startUpTraceTag);
        WebView webView = new WebView(this);
        setContentView(webView);
        Trace.endSection();

        CookieManager.setAcceptFileSchemeCookies(true);
        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(true);
        settings.setDomStorageEnabled(true);
        settings.setMediaPlaybackRequiresUserGesture(false);
        String userAgentString = intent.getStringExtra("userAgent");
        if (userAgentString != null) {
            settings.setUserAgentString(userAgentString);
        }

        webView.setWebViewClient(new WebViewClient() {
            @SuppressWarnings("deprecation") // because we support api level 19 and up.
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                return false;
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                Trace.endSection();
                Trace.endSection();
            }
        });

        Trace.beginSection(loadUrlTraceTag == null ? DEFAULT_LOAD_URL_TRACE_TAG : loadUrlTraceTag);
        webView.loadUrl("about:blank");
    }
}
