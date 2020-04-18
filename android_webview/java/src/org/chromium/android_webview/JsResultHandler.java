// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.base.ThreadUtils;

class JsResultHandler implements JsResultReceiver, JsPromptResultReceiver {
    private AwContentsClientBridge mBridge;
    private final int mId;

    JsResultHandler(AwContentsClientBridge bridge, int id) {
        mBridge = bridge;
        mId = id;
    }

    @Override
    public void confirm() {
        confirm(null);
    }

    @Override
    public void confirm(final String promptResult) {
        ThreadUtils.runOnUiThread(() -> {
            if (mBridge != null) mBridge.confirmJsResult(mId, promptResult);
            mBridge = null;
        });
    }

    @Override
    public void cancel() {
        ThreadUtils.runOnUiThread(() -> {
            if (mBridge != null) mBridge.cancelJsResult(mId);
            mBridge = null;
        });
    }
}
