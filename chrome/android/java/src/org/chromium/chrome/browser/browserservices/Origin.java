// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices;

import android.net.Uri;

/**
 * A class to canonically represent a web origin in Java. In comparison to
 * {@link org.chromium.net.GURLUtils#getOrigin} it can be used before native is loaded and lets us
 * ensure conversion to an origin has been done with the type system.
 *
 * {@link #toString()} does <b>not</b> match {@link org.chromium.net.GURLUtils#getOrigin}. The
 * latter will return a String with a trailing "/". Not having a trailing slash matches RFC
 * behaviour (https://tools.ietf.org/html/rfc6454), it seems that
 * {@link org.chromium.net.GURLUtils#getOrigin} adds it as a bug, but as its result is saved to
 * user's Android Preferences, it is not trivial to change.
 */
public class Origin {
    private static final int HTTP_DEFAULT_PORT = 80;
    private static final int HTTPS_DEFAULT_PORT = 443;

    private final Uri mOrigin;

    /**
     * Constructs a canonical Origin from a String.
     */
    public Origin(String uri) {
        this(Uri.parse(uri));
    }

    /**
     * Constructs a canonical Origin from an Uri.
     */
    public Origin(Uri uri) {
        if (uri == null || uri.getScheme() == null || uri.getAuthority() == null) {
            mOrigin = Uri.EMPTY;
            return;
        }

        // Make explicit ports implicit and remove any user:password.
        int port = uri.getPort();
        if (uri.getScheme().equals("http") && port == HTTP_DEFAULT_PORT) port = -1;
        if (uri.getScheme().equals("https") && port == HTTPS_DEFAULT_PORT) port = -1;

        String authority = uri.getHost();
        if (port != -1) authority += ":" + port;

        Uri origin;
        try {
            origin = uri.normalizeScheme()
                    .buildUpon()
                    .opaquePart("")
                    .fragment("")
                    .path("")
                    .encodedAuthority(authority)
                    .clearQuery()
                    .build();
        } catch (UnsupportedOperationException e) {
            origin = Uri.EMPTY;
        }

        mOrigin = origin;
    }

    /**
     * Returns whether the Origin is valid.
     */
    public boolean isValid() { return !mOrigin.equals(Uri.EMPTY); }

    /**
     * Returns a Uri representing the Origin.
     */
    public Uri uri() {
        return mOrigin;
    }

    @Override
    public int hashCode() {
        return mOrigin.hashCode();
    }

    /**
     * Returns a String representing the Origin.
     */
    @Override
    public String toString() {
        return mOrigin.toString();
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) return true;
        if (other == null || getClass() != other.getClass()) return false;
        return mOrigin.equals(((Origin) other).mOrigin);
    }
}
