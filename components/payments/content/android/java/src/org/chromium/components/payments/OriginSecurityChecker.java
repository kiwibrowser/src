// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.payments;

import org.chromium.base.annotations.JNINamespace;

/** Helper for origin security. */
@JNINamespace("payments")
public class OriginSecurityChecker {
    /**
     * Returns true for a valid URL from a secure origin, e.g., http://localhost,
     * file:///home/user/test.html, https://bobpay.com.
     *
     * @param url The URL to check.
     * @return Whether the origin of the URL is secure.
     */
    public static boolean isOriginSecure(String url) {
        return nativeIsOriginSecure(url);
    }

    /**
     * Returns true for a valid URL with a cryptographic scheme, e.g., HTTPS, WSS.
     *
     * @param url The URL to check.
     * @return Whether the scheme of the URL is cryptographic.
     */
    public static boolean isSchemeCryptographic(String url) {
        return nativeIsSchemeCryptographic(url);
    }

    /**
     * Returns true for a valid URL with localhost or file:// scheme origin, e.g., http://localhost,
     * file:///home/user/test.html.
     *
     * @param url The URL to check.
     * @return Whether the URL is localhost or file:// scheme origin.
     */
    public static boolean isOriginLocalhostOrFile(String url) {
        return nativeIsOriginLocalhostOrFile(url);
    }

    private OriginSecurityChecker() {}

    private static native boolean nativeIsOriginSecure(String url);
    private static native boolean nativeIsSchemeCryptographic(String url);
    private static native boolean nativeIsOriginLocalhostOrFile(String url);
}
