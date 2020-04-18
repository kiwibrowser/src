// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webauth;

import org.chromium.webauth.mojom.GetAssertionAuthenticatorResponse;
import org.chromium.webauth.mojom.MakeCredentialAuthenticatorResponse;

/**
 * Callback for receiving responses from an internal handler.
 */
public interface HandlerResponseCallback {
    /**
     * Interface that returns the response from a request to register a
     * credential with an authenticator.
     */
    void onRegisterResponse(Integer status, MakeCredentialAuthenticatorResponse response);

    /**
     * Interface that returns the response from a request to use a credential
     * to produce a signed assertion.
     */
    void onSignResponse(Integer status, GetAssertionAuthenticatorResponse response);

    /**
     * Interface that returns any errors from either register or sign requests.
     */
    void onError(Integer status);
}
