// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_glue;

import org.chromium.android_webview.AwContentsClient.AwWebResourceRequest;
import org.chromium.support_lib_boundary.WebResourceRequestBoundaryInterface;

/**
 * Adapter between AwWebResourceRequest and WebResourceRequestBoundaryInterface.
 */
public class SupportLibWebResourceRequest implements WebResourceRequestBoundaryInterface {
    private final AwWebResourceRequest mAwRequest;

    SupportLibWebResourceRequest(AwWebResourceRequest request) {
        mAwRequest = request;
    }

    @Override
    public boolean isRedirect() {
        return mAwRequest.isRedirect;
    }
}
