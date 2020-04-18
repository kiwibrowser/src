// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.net.Uri;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;

/**
 * AwTokenBindingManager manages the token binding protocol.
 *
 * see https://tools.ietf.org/html/draft-ietf-tokbind-protocol-03
 *
 * The token binding protocol can be enabled for browser context
 * separately. However all webviews share the same browser context and do not
 * expose the browser context to the embedder app. As such, there is no way to
 * enable token binding manager for individual webviews.
 */
@JNINamespace("android_webview")
public final class AwTokenBindingManager {
    private static final String TAG = "TokenBindingManager";
    private static final String ELLIPTIC_CURVE = "EC";

    public void enableTokenBinding() {
        nativeEnableTokenBinding();
    }

    public void getKey(Uri origin, String[] spec, Callback<KeyPair> callback) {
        if (callback == null) {
            throw new IllegalArgumentException("callback can't be null");
        }
        nativeGetTokenBindingKey(origin.getHost(), callback);
    }

    public void deleteKey(Uri origin, Callback<Boolean> callback) {
        if (origin == null) {
            throw new IllegalArgumentException("origin can't be null");
        }
        // null callback is allowed
        nativeDeleteTokenBindingKey(origin.getHost(), callback);
    }

    public void deleteAllKeys(Callback<Boolean> callback) {
        // null callback is allowed
        nativeDeleteAllTokenBindingKeys(callback);
    }

    @CalledByNative
    private static void onKeyReady(
            Callback<KeyPair> callback, byte[] privateKeyBytes, byte[] publicKeyBytes) {
        if (privateKeyBytes == null || publicKeyBytes == null) {
            callback.onResult(null);
            return;
        }
        KeyPair keyPair = null;
        try {
            KeyFactory factory = KeyFactory.getInstance(ELLIPTIC_CURVE);
            PrivateKey privateKey =
                    factory.generatePrivate(new PKCS8EncodedKeySpec(privateKeyBytes));
            PublicKey publicKey = factory.generatePublic(new X509EncodedKeySpec(publicKeyBytes));
            keyPair = new KeyPair(publicKey, privateKey);
        } catch (NoSuchAlgorithmException | InvalidKeySpecException ex) {
            Log.e(TAG, "Failed converting key ", ex);
        }
        callback.onResult(keyPair);
    }

    @CalledByNative
    private static void onDeletionComplete(Callback<Boolean> callback) {
        // At present, the native deletion complete callback always succeeds.
        callback.onResult(true);
    }

    private native void nativeEnableTokenBinding();
    private native void nativeGetTokenBindingKey(String host, Callback<KeyPair> callback);
    private native void nativeDeleteTokenBindingKey(String host, Callback<Boolean> callback);
    private native void nativeDeleteAllTokenBindingKeys(Callback<Boolean> callback);
}
