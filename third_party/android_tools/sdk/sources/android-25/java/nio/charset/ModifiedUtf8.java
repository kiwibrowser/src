/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package java.nio.charset;

import java.io.UTFDataFormatException;

/**
 * Encoding and decoding methods for Modified UTF-8
 *
 * <p>Modified UTF-8 is a simple variation of UTF-8 in which {@code \u0000} is encoded as
 * 0xc0 0x80 . This avoids the presence of bytes 0 in the output.
 *
 * @hide
 */
public class ModifiedUtf8 {

    /**
     * Count the number of bytes in the modified UTF-8 representation of {@code s}.
     *
     * <p>Additionally, if {@code shortLength} is true, throw a {@code UTFDataFormatException} if
     * the size cannot be presented in an (unsigned) java short.
     */
    public static long countBytes(String s, boolean shortLength) throws UTFDataFormatException {
        long counter = 0;
        int strLen = s.length();
        for (int i = 0; i < strLen; i++) {
            char c = s.charAt(i);
            if (c < '\u0080') {
                counter++;
                if (c == '\u0000') {
                    counter++;
                }
            } else if (c < '\u0800') {
                counter += 2;
            } else {
                counter += 3;
            }
        }
        // Allow up to the maximum value of an unsigned short (as the value is known to be
        // unsigned.
        if (shortLength && counter > 0xffff) {
            throw new UTFDataFormatException(
                    "Size of the encoded string doesn't fit in two bytes");
        }
        return counter;
    }

    /**
     * Encode {@code s} into {@code dst} starting at offset {@code offset}.
     *
     * <p>The output buffer is guaranteed to have enough space.
     */
    public static void encode(byte[] dst, int offset, String s) {
        int strLen = s.length();
        for (int i = 0; i < strLen; i++) {
            char c = s.charAt(i);
            if (c < '\u0080') {
                if (c == 0) {
                    dst[offset++] = (byte) 0xc0;
                    dst[offset++] = (byte) 0x80;
                } else {
                    dst[offset++] = (byte) c;
                }
            } else if (c < '\u0800') {
                dst[offset++] = (byte) ((c >>> 6) | 0xc0);
                dst[offset++] = (byte) ((c & 0x3f) | 0x80);
            } else {
                dst[offset++] = (byte) ((c >>> 12) | 0xe0);
                dst[offset++] = (byte) (((c >>> 6) & 0x3f) | 0x80);
                dst[offset++] = (byte) ((c & 0x3f) | 0x80);
            }
        }
    }

    /**
     * Encodes {@code s} into a buffer with the following format:
     *
     * <p>- the first two bytes of the buffer are the length of the modified-utf8 output
     * (as a big endian short. A UTFDataFormatException is thrown if the encoded size cannot be
     * represented as a short.
     *
     * <p>- the remainder of the buffer contains the modified-utf8 output (equivalent to
     * {@code encode(buf, 2, s)}).
     */
    public static byte[] encode(String s) throws UTFDataFormatException {
        long size = countBytes(s, true);
        byte[] output = new byte[(int) size + 2];
        encode(output, 2, s);
        output[0] = (byte) (size >>> 8);
        output[1] = (byte) size;
        return output;
    }

    /**
     * Decodes {@code length} utf-8 bytes from {@code in} starting at offset {@code offset} to
     * {@code out},
     *
     * <p>A maximum of {@code length} chars are written to the output starting at offset 0.
     * {@code out} is assumed to have enough space for the output (a standard
     * {@code ArrayIndexOutOfBoundsException} is thrown otherwise).
     *
     * <p>If a ‘0’ byte is encountered, it is converted to U+0000.
     */
    public static String decode(byte[] in, char[] out, int offset, int length)
            throws UTFDataFormatException {
        if (offset < 0 || length < 0) {
            throw new IllegalArgumentException("Illegal arguments: offset " + offset
                    + ". Length: " + length);
        }
        int outputIndex = 0;
        int limitIndex = offset + length;
        while (offset < limitIndex) {
            int i = in[offset] & 0xff;
            offset++;
            if (i < 0x80) {
                out[outputIndex] = (char) i;
                outputIndex++;
                continue;
            }
            if (0xc0 <= i && i < 0xe0) {
                // This branch covers the case 0 = 0xc080.

                // The result is: 5 least-significant bits of i + 6 l-s bits of next input byte.
                i = (i & 0x1f) << 6;
                if(offset == limitIndex) {
                    throw new UTFDataFormatException("unexpected end of input");
                }
                // Include 6 least-significant bits of the input byte.
                if ((in[offset] & 0xc0) != 0x80) {
                    throw new UTFDataFormatException("bad second byte at " + offset);
                }
                out[outputIndex] = (char) (i | (in[offset] & 0x3f));
                offset++;
                outputIndex++;
            } else if(i < 0xf0) {
                // The result is: 5 least-significant bits of i + 6 l-s bits of next input byte
                // + 6 l-s of next to next input byte.
                i = (i & 0x1f) << 12;
                // Make sure there are are at least two bytes left.
                if (offset + 1 >= limitIndex) {
                    throw new UTFDataFormatException("unexpected end of input");
                }
                // Include 6 least-significant bits of the input byte, with 6 bits of room
                // for the next byte.
                if ((in[offset] & 0xc0) != 0x80) {
                    throw new UTFDataFormatException("bad second byte at " + offset);
                }
                i = i | (in[offset] & 0x3f) << 6;
                offset++;
                // Include 6 least-significant bits of the input byte.
                if ((in[offset] & 0xc0) != 0x80) {
                    throw new UTFDataFormatException("bad third byte at " + offset);
                }
                out[outputIndex] = (char) (i | (in[offset] & 0x3f));
                offset++;
                outputIndex++;
            } else {
                throw new UTFDataFormatException("Invalid UTF8 byte "
                        + (int) i + " at position " + (offset - 1));
            }
        }
        return String.valueOf(out, 0, outputIndex);
    }
}
