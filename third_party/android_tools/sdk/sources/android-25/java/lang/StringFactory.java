/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package java.lang;

import java.io.Serializable;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Comparator;
import libcore.util.CharsetUtils;
import libcore.util.EmptyArray;

/**
 * Class used to generate strings instead of calling String.&lt;init&gt;.
 *
 * @hide
 */
public final class StringFactory {

    // TODO: Remove once native methods are in place.
    private static final char REPLACEMENT_CHAR = (char) 0xfffd;

    public static String newEmptyString() {
        return newStringFromChars(EmptyArray.CHAR, 0, 0);
    }

    public static String newStringFromBytes(byte[] data) {
        return newStringFromBytes(data, 0, data.length);
    }

    public static String newStringFromBytes(byte[] data, int high) {
        return newStringFromBytes(data, high, 0, data.length);
    }

    public static String newStringFromBytes(byte[] data, int offset, int byteCount) {
        return newStringFromBytes(data, offset, byteCount, Charset.defaultCharset());
    }

    public static native String newStringFromBytes(byte[] data, int high, int offset, int byteCount);

    public static String newStringFromBytes(byte[] data, int offset, int byteCount, String charsetName) throws UnsupportedEncodingException {
        return newStringFromBytes(data, offset, byteCount, Charset.forNameUEE(charsetName));
    }

    public static String newStringFromBytes(byte[] data, String charsetName) throws UnsupportedEncodingException {
        return newStringFromBytes(data, 0, data.length, Charset.forNameUEE(charsetName));
    }

    // TODO: Implement this method natively.
    public static String newStringFromBytes(byte[] data, int offset, int byteCount, Charset charset) {
        if ((offset | byteCount) < 0 || byteCount > data.length - offset) {
            throw new StringIndexOutOfBoundsException(data.length, offset, byteCount);
        }

        char[] value;
        int length;

        // We inline UTF-8, ISO-8859-1, and US-ASCII decoders for speed.
        String canonicalCharsetName = charset.name();
        if (canonicalCharsetName.equals("UTF-8")) {
            byte[] d = data;
            char[] v = new char[byteCount];

            int idx = offset;
            int last = offset + byteCount;
            int s = 0;
outer:
            while (idx < last) {
                byte b0 = d[idx++];
                if ((b0 & 0x80) == 0) {
                    // 0xxxxxxx
                    // Range:  U-00000000 - U-0000007F
                    int val = b0 & 0xff;
                    v[s++] = (char) val;
                } else if (((b0 & 0xe0) == 0xc0) || ((b0 & 0xf0) == 0xe0) ||
                        ((b0 & 0xf8) == 0xf0) || ((b0 & 0xfc) == 0xf8) || ((b0 & 0xfe) == 0xfc)) {
                    int utfCount = 1;
                    if ((b0 & 0xf0) == 0xe0) utfCount = 2;
                    else if ((b0 & 0xf8) == 0xf0) utfCount = 3;
                    else if ((b0 & 0xfc) == 0xf8) utfCount = 4;
                    else if ((b0 & 0xfe) == 0xfc) utfCount = 5;

                    // 110xxxxx (10xxxxxx)+
                    // Range:  U-00000080 - U-000007FF (count == 1)
                    // Range:  U-00000800 - U-0000FFFF (count == 2)
                    // Range:  U-00010000 - U-001FFFFF (count == 3)
                    // Range:  U-00200000 - U-03FFFFFF (count == 4)
                    // Range:  U-04000000 - U-7FFFFFFF (count == 5)

                    if (idx + utfCount > last) {
                        v[s++] = REPLACEMENT_CHAR;
                        continue;
                    }

                    // Extract usable bits from b0
                    int val = b0 & (0x1f >> (utfCount - 1));
                    for (int i = 0; i < utfCount; ++i) {
                        byte b = d[idx++];
                        if ((b & 0xc0) != 0x80) {
                            v[s++] = REPLACEMENT_CHAR;
                            idx--; // Put the input char back
                            continue outer;
                        }
                        // Push new bits in from the right side
                        val <<= 6;
                        val |= b & 0x3f;
                    }

                    // Note: Java allows overlong char
                    // specifications To disallow, check that val
                    // is greater than or equal to the minimum
                    // value for each count:
                    //
                    // count    min value
                    // -----   ----------
                    //   1           0x80
                    //   2          0x800
                    //   3        0x10000
                    //   4       0x200000
                    //   5      0x4000000

                    // Allow surrogate values (0xD800 - 0xDFFF) to
                    // be specified using 3-byte UTF values only
                    if ((utfCount != 2) && (val >= 0xD800) && (val <= 0xDFFF)) {
                        v[s++] = REPLACEMENT_CHAR;
                        continue;
                    }

                    // Reject chars greater than the Unicode maximum of U+10FFFF.
                    if (val > 0x10FFFF) {
                        v[s++] = REPLACEMENT_CHAR;
                        continue;
                    }

                    // Encode chars from U+10000 up as surrogate pairs
                    if (val < 0x10000) {
                        v[s++] = (char) val;
                    } else {
                        int x = val & 0xffff;
                        int u = (val >> 16) & 0x1f;
                        int w = (u - 1) & 0xffff;
                        int hi = 0xd800 | (w << 6) | (x >> 10);
                        int lo = 0xdc00 | (x & 0x3ff);
                        v[s++] = (char) hi;
                        v[s++] = (char) lo;
                    }
                } else {
                    // Illegal values 0x8*, 0x9*, 0xa*, 0xb*, 0xfd-0xff
                    v[s++] = REPLACEMENT_CHAR;
                }
            }

            if (s == byteCount) {
                // We guessed right, so we can use our temporary array as-is.
                value = v;
                length = s;
            } else {
                // Our temporary array was too big, so reallocate and copy.
                value = new char[s];
                length = s;
                System.arraycopy(v, 0, value, 0, s);
            }
        } else if (canonicalCharsetName.equals("ISO-8859-1")) {
            value = new char[byteCount];
            length = byteCount;
            CharsetUtils.isoLatin1BytesToChars(data, offset, byteCount, value);
        } else if (canonicalCharsetName.equals("US-ASCII")) {
            value = new char[byteCount];
            length = byteCount;
            CharsetUtils.asciiBytesToChars(data, offset, byteCount, value);
        } else {
            CharBuffer cb = charset.decode(ByteBuffer.wrap(data, offset, byteCount));
            length = cb.length();
            if (length > 0) {
                // We could use cb.array() directly, but that would mean we'd have to trust
                // the CharsetDecoder doesn't hang on to the CharBuffer and mutate it later,
                // which would break String's immutability guarantee. It would also tend to
                // mean that we'd be wasting memory because CharsetDecoder doesn't trim the
                // array. So we copy.
                value = new char[length];
                System.arraycopy(cb.array(), 0, value, 0, length);
            } else {
                value = EmptyArray.CHAR;
            }
        }
        return newStringFromChars(value, 0, length);
    }

    public static String newStringFromBytes(byte[] data, Charset charset) {
        return newStringFromBytes(data, 0, data.length, charset);
    }

    public static String newStringFromChars(char[] data) {
        return newStringFromChars(data, 0, data.length);
    }

    public static String newStringFromChars(char[] data, int offset, int charCount) {
        if ((offset | charCount) < 0 || charCount > data.length - offset) {
            throw new StringIndexOutOfBoundsException(data.length, offset, charCount);
        }
        return newStringFromChars(offset, charCount, data);
    }

    // The char array passed as {@code java_data} must not be a null reference.
    static native String newStringFromChars(int offset, int charCount, char[] data);

    public static native String newStringFromString(String toCopy);

    public static String newStringFromStringBuffer(StringBuffer stringBuffer) {
        synchronized (stringBuffer) {
            return newStringFromChars(stringBuffer.getValue(), 0, stringBuffer.length());
        }
    }

    // TODO: Implement this method natively.
    public static String newStringFromCodePoints(int[] codePoints, int offset, int count) {
        if (codePoints == null) {
            throw new NullPointerException("codePoints == null");
        }
        if ((offset | count) < 0 || count > codePoints.length - offset) {
            throw new StringIndexOutOfBoundsException(codePoints.length, offset, count);
        }
        char[] value = new char[count * 2];
        int end = offset + count;
        int length = 0;
        for (int i = offset; i < end; i++) {
            length += Character.toChars(codePoints[i], value, length);
        }
        return newStringFromChars(value, 0, length);
    }

    public static String newStringFromStringBuilder(StringBuilder stringBuilder) {
        return newStringFromChars(stringBuilder.getValue(), 0, stringBuilder.length());
    }
}
