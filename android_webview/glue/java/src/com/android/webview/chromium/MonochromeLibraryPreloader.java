// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Context;
import android.webkit.WebViewFactory;

import org.chromium.base.library_loader.NativeLibraryPreloader;

/**
 * The library preloader for Monochrome for sharing native library's relro
 * between Chrome and WebView.
 */
public class MonochromeLibraryPreloader extends NativeLibraryPreloader {

    @Override
    public int loadLibrary(Context context) {
        return WebViewFactory.loadWebViewNativeLibraryFromPackage(context.getPackageName(),
                getClass().getClassLoader());
    }
}
