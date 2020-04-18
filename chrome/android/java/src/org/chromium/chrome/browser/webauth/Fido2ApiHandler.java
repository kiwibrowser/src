// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webauth;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.webauth.mojom.PublicKeyCredentialCreationOptions;
import org.chromium.webauth.mojom.PublicKeyCredentialRequestOptions;

/**
 * Android implementation of the Authenticator service defined in
 * components/webauth/authenticator.mojom.
 */
public class Fido2ApiHandler {
    private static Fido2ApiHandler sInstance;

    /**
     * @return The Fido2ApiHandler for use during the lifetime of the browser process.
     */
    public static Fido2ApiHandler getInstance() {
        ThreadUtils.checkUiThread();
        if (sInstance == null) {
            sInstance = AppHooks.get().createFido2ApiHandler();
        }
        return sInstance;
    }

    protected void makeCredential(
            PublicKeyCredentialCreationOptions options, HandlerResponseCallback callback) {}

    protected void getAssertion(
            PublicKeyCredentialRequestOptions options, HandlerResponseCallback callback) {}
}
