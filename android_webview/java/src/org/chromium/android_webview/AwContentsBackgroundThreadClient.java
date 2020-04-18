// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.HashMap;

/**
 * Delegate for handling callbacks. All methods are called on the background thread.
 * "Background" means something that isn't UI or IO.
 */
@JNINamespace("android_webview")
public abstract class AwContentsBackgroundThreadClient {

    public abstract AwWebResourceResponse shouldInterceptRequest(
            AwContentsClient.AwWebResourceRequest request);

    // Protected methods ---------------------------------------------------------------------------

    @CalledByNative
    private AwWebResourceResponse shouldInterceptRequestFromNative(String url, boolean isMainFrame,
            boolean hasUserGesture, String method, String[] requestHeaderNames,
            String[] requestHeaderValues) {
        AwContentsClient.AwWebResourceRequest request =
                new AwContentsClient.AwWebResourceRequest();
        request.url = url;
        request.isMainFrame = isMainFrame;
        request.hasUserGesture = hasUserGesture;
        request.method = method;
        request.requestHeaders = new HashMap<String, String>(requestHeaderNames.length);
        for (int i = 0; i < requestHeaderNames.length; ++i) {
            request.requestHeaders.put(requestHeaderNames[i], requestHeaderValues[i]);
        }
        return shouldInterceptRequest(request);
    }
}
