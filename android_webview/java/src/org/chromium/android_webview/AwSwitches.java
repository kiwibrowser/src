// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

/**
 * Contains command line switches that are specific to Android WebView.
 */
public final class AwSwitches {
    // Indicate that renderers are running in a sandbox. Enables
    // kInProcessGPU and sets kRendererProcessLimit to 1.
    // Native switch kWebViewSandboxedRenderer.
    public static final String WEBVIEW_SANDBOXED_RENDERER = "webview-sandboxed-renderer";

    // Enables safebrowsing functionality in webview.
    // Native switch kWebViewEnableSafeBrowsingSupport.
    public static final String WEBVIEW_ENABLE_SAFEBROWSING_SUPPORT =
            "webview-enable-safebrowsing-support";

    // Disables safebrowsing functionality in webview.
    // Native switch kWebViewDisableSafeBrowsingSupport.
    public static final String WEBVIEW_DISABLE_SAFEBROWSING_SUPPORT =
            "webview-disable-safebrowsing-support";

    // Enables SafeBrowsing and causes WebView to treat all resources as malicious. Use care: this
    // will block all resources from loading.
    // No native switch.
    public static final String WEBVIEW_SAFEBROWSING_BLOCK_ALL_RESOURCES =
            "webview-safebrowsing-block-all-resources";

    // Enables variations AB testing experiments in webview.
    // Native switch kEnableWebViewVariations.
    public static final String ENABLE_WEBVIEW_VARIATIONS = "enable-webview-variations";

    // Do not instantiate this class.
    private AwSwitches() {}
}
