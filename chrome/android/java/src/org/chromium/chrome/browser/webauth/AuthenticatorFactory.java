// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webauth;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.services.service_manager.InterfaceFactory;
import org.chromium.webauth.mojom.Authenticator;

/**
 * Factory class registered to create Authenticators upon request.
 */
public class AuthenticatorFactory implements InterfaceFactory<Authenticator> {
    private final RenderFrameHost mRenderFrameHost;

    public AuthenticatorFactory(RenderFrameHost renderFrameHost) {
        mRenderFrameHost = renderFrameHost;
    }

    @Override
    public Authenticator createImpl() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.WEB_AUTH)) {
            return null;
        }

        if (mRenderFrameHost == null) return null;
        return new AuthenticatorImpl(mRenderFrameHost);
    }
}
