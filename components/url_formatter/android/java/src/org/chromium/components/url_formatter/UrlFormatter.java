// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.url_formatter;

import android.text.TextUtils;

import org.chromium.base.annotations.JNINamespace;

import java.net.URI;

/**
 * Wrapper for utilities in url_formatter.
 */
@JNINamespace("url_formatter::android")
public final class UrlFormatter {
    /**
     * Refer to url_formatter::FixupURL.
     *
     * Given a URL-like string, returns a real URL or null. For example:
     *  - "google.com" -> "http://google.com/"
     *  - "about:" -> "chrome://version/"
     *  - "//mail.google.com:/" -> "file:///mail.google.com:/"
     *  - "..." -> null
     */
    public static String fixupUrl(String uri) {
        return TextUtils.isEmpty(uri) ? null : nativeFixupUrl(uri);
    }

    /**
     * Builds a String representation of <code>uri</code> suitable for display to the user, omitting
     * the scheme, the username and password, and trailing slash on a bare hostname.
     *
     * The IDN hostname is turned to Unicode if the Unicode representation is deemed safe.
     * For more information, see <code>url_formatter::FormatUrl(const GURL&)</code>.
     *
     * Some examples:
     *  - "http://user:password@example.com/" -> "example.com"
     *  - "https://example.com/path" -> "example.com/path"
     *  - "http://www.xn--frgbolaget-q5a.se" -> "www.färgbolaget.se"
     *
     * @param uri URI to format.
     * @return Formatted URL.
     */
    public static String formatUrlForDisplayOmitScheme(String uri) {
        return nativeFormatUrlForDisplayOmitScheme(uri);
    }

    /**
     * Builds a String representation of <code>uri</code> suitable for display to the user,
     * omitting the HTTP scheme, the username and password, trailing slash on a bare hostname,
     * and converting %20 to spaces.
     *
     * The IDN hostname is turned to Unicode if the Unicode representation is deemed safe.
     * For more information, see <code>url_formatter::FormatUrl(const GURL&)</code>.
     *
     * Example:
     *  - "http://user:password@example.com/%20test" -> "example.com/ test"
     *  - "http://user:password@example.com/" -> "example.com"
     *  - "http://www.xn--frgbolaget-q5a.se" -> "www.färgbolaget.se"
     *
     * @param uri URI to format.
     * @return Formatted URL.
     */
    public static String formatUrlForDisplayOmitHTTPScheme(String uri) {
        return nativeFormatUrlForDisplayOmitHTTPScheme(uri);
    }

    /**
     * Builds a String representation of <code>uri</code> suitable for copying to the clipboard.
     * It does not omit any components, and it performs normal escape decoding. Spaces are left
     * escaped. The IDN hostname is turned to Unicode if the Unicode representation is deemed safe.
     * For more information, see <code>url_formatter::FormatUrl(const GURL&)</code>.
     *
     * @param uri URI to format.
     * @return Formatted URL.
     */
    public static String formatUrlForCopy(String uri) {
        return nativeFormatUrlForCopy(uri);
    }

    /**
     * Builds a String that strips down the URL to its scheme, host, and port.
     * @param uri URI to break down.
     * @param showScheme Whether or not to show the scheme.  If the URL can't be parsed, this value
     *                   is ignored.
     * @return Stripped-down String containing the essential bits of the URL, or the original URL if
     *         it fails to parse it.
     */
    public static String formatUrlForSecurityDisplay(URI uri, boolean showScheme) {
        return formatUrlForSecurityDisplay(uri.toString(), showScheme);
    }

    /**
     * Builds a String that strips down |uri| to its scheme, host, and port.
     * @param uri The URI to break down.
     * @param showScheme Whether or not to show the scheme.  If the URL can't be parsed, this value
     *                   is ignored.
     * @return Stripped-down String containing the essential bits of the URL, or the original URL if
     *         it fails to parse it.
     */
    public static String formatUrlForSecurityDisplay(String uri, boolean showScheme) {
        if (showScheme) {
            return nativeFormatUrlForSecurityDisplay(uri);
        } else {
            return nativeFormatUrlForSecurityDisplayOmitScheme(uri);
        }
    }

    private static native String nativeFixupUrl(String url);
    private static native String nativeFormatUrlForDisplayOmitScheme(String url);
    private static native String nativeFormatUrlForDisplayOmitHTTPScheme(String url);
    private static native String nativeFormatUrlForCopy(String url);
    private static native String nativeFormatUrlForSecurityDisplay(String url);
    private static native String nativeFormatUrlForSecurityDisplayOmitScheme(String url);
}
