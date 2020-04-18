// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.net.Uri;
import android.webkit.TokenBindingService;
import android.webkit.TokenBindingService.TokenBindingKey;
import android.webkit.ValueCallback;

import org.chromium.android_webview.AwTokenBindingManager;
import org.chromium.base.Callback;

import java.security.KeyPair;

/**
 * Chromium implementation of TokenBindingManager. The API requires
 * all access to TokenBindingManager to be on UI thread, so we start the
 * chromium engines with this assumption.
 */
public class TokenBindingManagerAdapter extends TokenBindingService {

    private AwTokenBindingManager mTokenBindingManager = new AwTokenBindingManager();
    private WebViewChromiumFactoryProvider mProvider;
    private boolean mEnabled;

    TokenBindingManagerAdapter(WebViewChromiumFactoryProvider provider) {
        mProvider = provider;
    }

    @Override
    public void enableTokenBinding() {
        // We cannot start the chromium engine yet, since doing so would
        // initialize the UrlRequestContextGetter and then it would be too
        // late to enable token binding.
        if (mProvider.hasStarted()) {
            throw new IllegalStateException(
                    "Token binding cannot be enabled after webview creation");
        }
        mEnabled = true;
        mTokenBindingManager.enableTokenBinding();
    }

    @Override
    public void getKey(Uri origin,
                       String[] algorithm,
                       final ValueCallback<TokenBindingKey> callback) {
        startChromiumEngine();
        if (algorithm != null && algorithm.length == 0) {
            throw new IllegalArgumentException("algorithms cannot be empty");
        }
        if (algorithm != null) {
            boolean found = false;
            for (String alg:algorithm) {
                if (alg.equals(TokenBindingService.KEY_ALGORITHM_ECDSAP256)) {
                    found = true; break;
                }
            }
            if (!found) {
                throw new IllegalArgumentException("no supported algorithm found");
            }
        }
        // Only return the KeyPair for now. We retrieve the key from Channel Id
        // store which does not provide a way to set/retrieve the Token
        // Binding algorithms yet.
        Callback<KeyPair> newCallback = new Callback<KeyPair>() {
            @Override
            public void onResult(final KeyPair value) {
                TokenBindingKey key = new TokenBindingKey() {
                    @Override
                    public KeyPair getKeyPair() {
                        return value;
                    }
                    @Override
                    public String getAlgorithm() {
                        return TokenBindingService.KEY_ALGORITHM_ECDSAP256;
                    }
                };
                callback.onReceiveValue(key);
            }
        };
        mTokenBindingManager.getKey(origin, null, newCallback);
    }

    @Override
    public void deleteKey(Uri origin, final ValueCallback<Boolean> callback) {
        startChromiumEngine();
        mTokenBindingManager.deleteKey(origin, CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public void deleteAllKeys(final ValueCallback<Boolean> callback) {
        startChromiumEngine();
        mTokenBindingManager.deleteAllKeys(CallbackConverter.fromValueCallback(callback));
    }

    private void startChromiumEngine() {
        if (!mEnabled) {
            throw new IllegalStateException("Token binding is not enabled");
        }
        // Make sure chromium engine is running.
        mProvider.startYourEngines(false);
    }
}
