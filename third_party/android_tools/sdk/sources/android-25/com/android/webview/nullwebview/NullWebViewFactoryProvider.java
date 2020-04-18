/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.webview.nullwebview;

import android.content.Context;
import android.webkit.CookieManager;
import android.webkit.GeolocationPermissions;
import android.webkit.ServiceWorkerController;
import android.webkit.TokenBindingService;
import android.webkit.WebIconDatabase;
import android.webkit.WebStorage;
import android.webkit.WebView;
import android.webkit.WebViewDatabase;
import android.webkit.WebViewDelegate;
import android.webkit.WebViewFactoryProvider;
import android.webkit.WebViewProvider;

public class NullWebViewFactoryProvider implements WebViewFactoryProvider {

    public NullWebViewFactoryProvider(WebViewDelegate delegate) {
    }

    @Override
    public WebViewFactoryProvider.Statics getStatics() {
        throw new UnsupportedOperationException();
    }

    @Override
    public WebViewProvider createWebView(WebView webView, WebView.PrivateAccess privateAccess) {
        throw new UnsupportedOperationException();
    }

    @Override
    public GeolocationPermissions getGeolocationPermissions() {
        throw new UnsupportedOperationException();
    }

    @Override
    public CookieManager getCookieManager() {
        throw new UnsupportedOperationException();
    }

    @Override
    public TokenBindingService getTokenBindingService() {
        throw new UnsupportedOperationException();
    }

    @Override
    public ServiceWorkerController getServiceWorkerController() {
        throw new UnsupportedOperationException();
    }

    @Override
    public WebIconDatabase getWebIconDatabase() {
        throw new UnsupportedOperationException();
    }

    @Override
    public WebStorage getWebStorage() {
        throw new UnsupportedOperationException();
    }

    @Override
    public WebViewDatabase getWebViewDatabase(Context context) {
        throw new UnsupportedOperationException();
    }
}
