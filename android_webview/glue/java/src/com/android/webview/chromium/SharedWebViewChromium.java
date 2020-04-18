// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.WebViewChromiumRunQueue;
import org.chromium.base.ThreadUtils;
import org.chromium.content_public.browser.MessagePort;

import java.util.concurrent.Callable;

/**
 * This class contains the parts of WebViewChromium that should be shared between the webkit-glue
 * layer and the support library glue layer.
 */
public class SharedWebViewChromium {
    private final WebViewChromiumRunQueue mRunQueue;
    private final WebViewChromiumAwInit mAwInit;
    // The WebView wrapper for ContentViewCore and required browser compontents.
    private AwContents mAwContents;

    public SharedWebViewChromium(WebViewChromiumRunQueue runQueue, WebViewChromiumAwInit awInit) {
        mRunQueue = runQueue;
        mAwInit = awInit;
    }

    public void setAwContentsOnUiThread(AwContents awContents) {
        assert ThreadUtils.runningOnUiThread();

        if (mAwContents != null) {
            throw new RuntimeException(
                    "Cannot create multiple AwContents for the same SharedWebViewChromium");
        }
        mAwContents = awContents;
    }

    public void insertVisualStateCallback(long requestId, AwContents.VisualStateCallback callback) {
        if (checkNeedsPost()) {
            mRunQueue.addTask(new Runnable() {
                @Override
                public void run() {
                    insertVisualStateCallback(requestId, callback);
                }
            });
            return;
        }
        mAwContents.insertVisualStateCallback(requestId, callback);
    }

    public MessagePort[] createWebMessageChannel() {
        mAwInit.startYourEngines(true);
        if (checkNeedsPost()) {
            MessagePort[] ret = mRunQueue.runOnUiThreadBlocking(new Callable<MessagePort[]>() {
                @Override
                public MessagePort[] call() {
                    return createWebMessageChannel();
                }
            });
            return ret;
        }
        return mAwContents.createMessageChannel();
    }

    public void postMessageToMainFrame(
            final String message, final String targetOrigin, final MessagePort[] sentPorts) {
        if (checkNeedsPost()) {
            mRunQueue.addTask(new Runnable() {
                @Override
                public void run() {
                    postMessageToMainFrame(message, targetOrigin, sentPorts);
                }
            });
            return;
        }
        mAwContents.postMessageToFrame(null, message, targetOrigin, sentPorts);
    }

    protected boolean checkNeedsPost() {
        boolean needsPost = !mRunQueue.chromiumHasStarted() || !ThreadUtils.runningOnUiThread();
        if (!needsPost && mAwContents == null) {
            throw new IllegalStateException("AwContents must be created if we are not posting!");
        }
        return needsPost;
    }

    public AwContents getAwContents() {
        return mAwContents;
    }
}
