/* Copyright (c) 2002-2013 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.util.Arrays;


/**
 * JMake needs to run against old versions of Java, that may not have JAXB's
 * javax.xml.bind.DatatypeConverter. And we don't want JMake to depend on third-party external libraries,
 * especially not just for this.  So we implement a lightweight Base64 converter here ourselves.

 * Note that sun.misc.BASE64Encoder is not official API and can go away at any time. Plus it inserts
 * line breaks into its emitted string, which is not what we want. So we can't use that either.
 */

public class Base64 {
    // The easiest way to grok this code is to think of Base64 as the following chain of
    // conversions (ignoring padding issues):
    // 3 bytes -> 24 bits -> 4 6-bit nibbles -> 4 indexes from 0-63 -> 4 characters.
    private static final char[] indexToDigit =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/".toCharArray();
    private static final int[] digitToIndex = new int[128];
    static {
        assert(indexToDigit.length == 64);
        Arrays.fill(digitToIndex, -1);
        for (int i = 0; i < indexToDigit.length; i++) digitToIndex[(int)indexToDigit[i]] = i;
    }

    private Base64() {}

    public static char[] encode(byte[] in) {
        char[] ret = new char[(in.length + 2) / 3 * 4];
        int p = 0;
        int i = 0;
        while (i < in.length) {
            // Lowest 24 bits count.
            int bits = (in[i++] & 0xff) << 16 | (i < in.length ? in[i++] & 0xff : 0) << 8 | (i < in.length ? in[i++] & 0xff : 0);
            ret[p++] = indexToDigit[(bits & 0xfc0000) >> 18];
            ret[p++] = indexToDigit[(bits & 0x3f000) >> 12];
            ret[p++] = indexToDigit[(bits & 0xfc0) >> 6];
            ret[p++] = indexToDigit[bits & 0x3f];
        }
        assert(p == ret.length);
        int padding = (3 - in.length % 3) % 3;
        for (int j = ret.length - padding; j < ret.length; j++) ret[j] = '=';
        return ret;
    }

    public static byte[] decode(char[] in) {
        if (in.length % 4 != 0) throw new IllegalArgumentException("Base64-encoded string must be of length that is a multiple of 4.");
        int len = in.length;
        while(len > 0 && in[len - 1] == '=') len--;
        int padding = in.length - len;
        byte[] ret = new byte[in.length / 4 * 3 - padding];
        int i = 0;
        int p = 0;
        while (i < len) {
            char c0 = in[i++];
            char c1 = in[i++];
            char c2 = i < len ? in[i++] : 'A';
            char c3 = i < len ? in[i++] : 'A';
            if (c0 > 127 || c1 > 127 || c2 > 127 || c3 > 127) throw new IllegalArgumentException("Invalid Base64 digit in: " + c0 + c1 + c2 + c3);
            int n0 = digitToIndex[c0];
            int n1 = digitToIndex[c1];
            int n2 = digitToIndex[c2];
            int n3 = digitToIndex[c3];
            if (n0 < 0 || n1 < 0 || n2 < 0 || n3 < 0) throw new IllegalArgumentException("Invalid Base64 digit in: " + c0 + c1 + c2 + c3);
            int bits = (n0 << 18) | (n1 << 12) | (n2 << 6) | n3;
            ret[p++] = (byte)((bits & 0xff0000) >> 16);
            if (p < ret.length) ret[p++] = (byte)((bits & 0xff00) >> 8);
            if (p < ret.length) ret[p++] = (byte)(bits & 0xff);
        }
        return ret;
    }
}
