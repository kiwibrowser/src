// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_glue;

import android.net.Uri;

import com.android.webview.chromium.SharedWebViewChromium;

import org.chromium.android_webview.AwContents;
import org.chromium.support_lib_boundary.VisualStateCallbackBoundaryInterface;
import org.chromium.support_lib_boundary.WebMessageBoundaryInterface;
import org.chromium.support_lib_boundary.WebViewProviderBoundaryInterface;
import org.chromium.support_lib_boundary.util.BoundaryInterfaceReflectionUtil;

import java.lang.reflect.InvocationHandler;

/**
 * Support library glue version of WebViewChromium.
 */
class SupportLibWebViewChromium implements WebViewProviderBoundaryInterface {
    private final SharedWebViewChromium mSharedWebViewChromium;

    public SupportLibWebViewChromium(SharedWebViewChromium sharedWebViewChromium) {
        mSharedWebViewChromium = sharedWebViewChromium;
    }

    @Override
    public void insertVisualStateCallback(long requestId, InvocationHandler callbackInvoHandler) {
        final VisualStateCallbackBoundaryInterface visualStateCallback =
                BoundaryInterfaceReflectionUtil.castToSuppLibClass(
                        VisualStateCallbackBoundaryInterface.class, callbackInvoHandler);

        mSharedWebViewChromium.insertVisualStateCallback(
                requestId, new AwContents.VisualStateCallback() {
                    @Override
                    public void onComplete(long requestId) {
                        visualStateCallback.onComplete(requestId);
                    }
                });
    }

    @Override
    public /* WebMessagePort */ InvocationHandler[] createWebMessageChannel() {
        return SupportLibWebMessagePortAdapter.fromMessagePorts(
                mSharedWebViewChromium.createWebMessageChannel());
    }

    @Override
    public void postMessageToMainFrame(
            /* WebMessage */ InvocationHandler message, Uri targetOrigin) {
        WebMessageBoundaryInterface messageBoundaryInterface =
                BoundaryInterfaceReflectionUtil.castToSuppLibClass(
                        WebMessageBoundaryInterface.class, message);
        mSharedWebViewChromium.postMessageToMainFrame(messageBoundaryInterface.getData(),
                targetOrigin.toString(),
                SupportLibWebMessagePortAdapter.toMessagePorts(
                        messageBoundaryInterface.getPorts()));
    }
}
