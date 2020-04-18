// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webauth;

import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsStatics;
import org.chromium.mojo.system.MojoException;
import org.chromium.webauth.mojom.Authenticator;
import org.chromium.webauth.mojom.AuthenticatorStatus;
import org.chromium.webauth.mojom.GetAssertionAuthenticatorResponse;
import org.chromium.webauth.mojom.MakeCredentialAuthenticatorResponse;
import org.chromium.webauth.mojom.PublicKeyCredentialCreationOptions;
import org.chromium.webauth.mojom.PublicKeyCredentialRequestOptions;

/**
 * Android implementation of the authenticator.mojom interface.
 */
public class AuthenticatorImpl implements Authenticator, HandlerResponseCallback {
    private final RenderFrameHost mRenderFrameHost;
    private final WebContents mWebContents;

    /** Ensures only one request is processed at a time. */
    boolean mIsOperationPending = false;

    private org.chromium.mojo.bindings.Callbacks
            .Callback2<Integer, MakeCredentialAuthenticatorResponse> mMakeCredentialCallback;
    private org.chromium.mojo.bindings.Callbacks
            .Callback2<Integer, GetAssertionAuthenticatorResponse> mGetAssertionCallback;

    /**
     * Builds the Authenticator service implementation.
     *
     * @param renderFrameHost The host of the frame that has invoked the API.
     */
    public AuthenticatorImpl(RenderFrameHost renderFrameHost) {
        assert renderFrameHost != null;
        mRenderFrameHost = renderFrameHost;
        mWebContents = WebContentsStatics.fromRenderFrameHost(renderFrameHost);
    }

    @Override
    public void makeCredential(
            PublicKeyCredentialCreationOptions options, MakeCredentialResponse callback) {
        mMakeCredentialCallback = callback;
        if (mIsOperationPending) {
            onError(AuthenticatorStatus.PENDING_REQUEST);
            return;
        }

        mIsOperationPending = true;
        onError(AuthenticatorStatus.NOT_IMPLEMENTED);
    }

    @Override
    public void getAssertion(
            PublicKeyCredentialRequestOptions options, GetAssertionResponse callback) {
        mGetAssertionCallback = callback;
        if (mIsOperationPending) {
            onError(AuthenticatorStatus.PENDING_REQUEST);
            return;
        }

        mIsOperationPending = true;
        onError(AuthenticatorStatus.NOT_IMPLEMENTED);
    }

    /**
     * Callbacks for receiving responses from the internal handlers.
     */
    @Override
    public void onRegisterResponse(Integer status, MakeCredentialAuthenticatorResponse response) {
        assert mMakeCredentialCallback != null;
        mMakeCredentialCallback.call(status, response);
        close();
    }

    @Override
    public void onSignResponse(Integer status, GetAssertionAuthenticatorResponse response) {
        assert mGetAssertionCallback != null;
        mGetAssertionCallback.call(status, response);
        close();
    }

    @Override
    public void onError(Integer status) {
        assert(mMakeCredentialCallback != null || mGetAssertionCallback != null);
        if (mMakeCredentialCallback != null) {
            mMakeCredentialCallback.call(status, null);
        } else if (mGetAssertionCallback != null) {
            mGetAssertionCallback.call(status, null);
        }
        close();
    }

    @Override
    public void close() {
        mIsOperationPending = false;
        mMakeCredentialCallback = null;
        mGetAssertionCallback = null;
    }

    @Override
    public void onConnectionError(MojoException e) {
        close();
    }
}
