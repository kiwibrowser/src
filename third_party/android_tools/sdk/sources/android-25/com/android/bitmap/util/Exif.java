/*
 * Copyright (C) 2013 The Android Open Source Project
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
 * limitations under the License.
 */

package com.android.bitmap.util;

import android.util.Log;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

/**
 * TODO
 * Exif and InputStreamBuffer were pulled in from frameworks/ex/photo, and should be part of a
 * separate library that is used by both this and chips.
 */
public class Exif {
    private static final String TAG = Exif.class.getSimpleName();

    /**
     * Returns the degrees in clockwise. Values are 0, 90, 180, or 270.
     * @param inputStream The input stream will not be closed for you.
     * @param byteSize Recommended parameter declaring the length of the input stream. If you
     *                 pass in -1, we will have to read more from the input stream.
     * @return 0, 90, 180, or 270.
     */
    public static int getOrientation(final InputStream inputStream, final long byteSize) {
        if (inputStream == null) {
            return 0;
        }

        /*
          Looking at this algorithm, we never look ahead more than 8 bytes. As long as we call
          advanceTo() at the end of every loop, we should never have to reallocate a larger buffer.

          Also, the most we ever read backwards is 4 bytes. pack() reads backwards if the encoding
          is in little endian format. These following two lines potentially reads 4 bytes backwards:

          int tag = pack(jpeg, offset, 4, false);
          count = pack(jpeg, offset - 2, 2, littleEndian);

          To be safe, we will always advance to some index-4, so we'll need 4 more for the +8
          look ahead, which makes it a +12 look ahead total. Use 16 just in case my analysis is off.

          This means we only need to allocate a single 16 byte buffer.

          Note: If you do not pass in byteSize parameter, a single large allocation will occur.
          For a 1MB image, I see one 30KB allocation. This is due to the line containing:

          has(jpeg, byteSize, offset + length - 1)

          where length is a variable int (around 30KB above) read from the EXIF headers.

          This is still much better than allocating a 1MB byte[] which we were doing before.
         */

        final int lookAhead = 16;
        final int readBackwards = 4;
        final InputStreamBuffer jpeg = new InputStreamBuffer(inputStream, lookAhead, false);

        int offset = 0;
        int length = 0;

        if (has(jpeg, byteSize, 1)) {
            // JPEG image files begin with FF D8. Only JPEG images have EXIF data.
            final boolean possibleJpegFormat = jpeg.get(0) == (byte) 0xFF
                    && jpeg.get(1) == (byte) 0xD8;
            if (!possibleJpegFormat) {
                return 0;
            }
        }

        // ISO/IEC 10918-1:1993(E)
        while (has(jpeg, byteSize, offset + 3) && (jpeg.get(offset++) & 0xFF) == 0xFF) {
            final int marker = jpeg.get(offset) & 0xFF;

            // Check if the marker is a padding.
            if (marker == 0xFF) {
                continue;
            }
            offset++;

            // Check if the marker is SOI or TEM.
            if (marker == 0xD8 || marker == 0x01) {
                continue;
            }
            // Check if the marker is EOI or SOS.
            if (marker == 0xD9 || marker == 0xDA) {
                // Loop ends.
                jpeg.advanceTo(offset - readBackwards);
                break;
            }

            // Get the length and check if it is reasonable.
            length = pack(jpeg, offset, 2, false);
            if (length < 2 || !has(jpeg, byteSize, offset + length - 1)) {
                Log.e(TAG, "Invalid length");
                return 0;
            }

            // Break if the marker is EXIF in APP1.
            if (marker == 0xE1 && length >= 8 &&
                    pack(jpeg, offset + 2, 4, false) == 0x45786966 &&
                    pack(jpeg, offset + 6, 2, false) == 0) {
                offset += 8;
                length -= 8;
                // Loop ends.
                jpeg.advanceTo(offset - readBackwards);
                break;
            }

            // Skip other markers.
            offset += length;
            length = 0;

            // Loop ends.
            jpeg.advanceTo(offset - readBackwards);
        }

        // JEITA CP-3451 Exif Version 2.2
        if (length > 8) {
            // Identify the byte order.
            int tag = pack(jpeg, offset, 4, false);
            if (tag != 0x49492A00 && tag != 0x4D4D002A) {
                Log.e(TAG, "Invalid byte order");
                return 0;
            }
            final boolean littleEndian = (tag == 0x49492A00);

            // Get the offset and check if it is reasonable.
            int count = pack(jpeg, offset + 4, 4, littleEndian) + 2;
            if (count < 10 || count > length) {
                Log.e(TAG, "Invalid offset");
                return 0;
            }
            offset += count;
            length -= count;

            // Offset has changed significantly.
            jpeg.advanceTo(offset - readBackwards);

            // Get the count and go through all the elements.
            count = pack(jpeg, offset - 2, 2, littleEndian);

            while (count-- > 0 && length >= 12) {
                // Get the tag and check if it is orientation.
                tag = pack(jpeg, offset, 2, littleEndian);
                if (tag == 0x0112) {
                    // We do not really care about type and count, do we?
                    final int orientation = pack(jpeg, offset + 8, 2, littleEndian);
                    switch (orientation) {
                        case 1:
                            return 0;
                        case 3:
                            return 180;
                        case 6:
                            return 90;
                        case 8:
                            return 270;
                    }
                    Log.i(TAG, "Unsupported orientation");
                    return 0;
                }
                offset += 12;
                length -= 12;

                // Loop ends.
                jpeg.advanceTo(offset - readBackwards);
            }
        }

        return 0;
    }

    private static int pack(final InputStreamBuffer bytes, int offset, int length,
            final boolean littleEndian) {
        int step = 1;
        if (littleEndian) {
            offset += length - 1;
            step = -1;
        }

        int value = 0;
        while (length-- > 0) {
            value = (value << 8) | (bytes.get(offset) & 0xFF);
            offset += step;
        }
        return value;
    }

    private static boolean has(final InputStreamBuffer jpeg, final long byteSize, final int index) {
        if (byteSize >= 0) {
            return index < byteSize;
        } else {
            // For large values of index, this will cause the internal buffer to resize.
            return jpeg.has(index);
        }
    }

    @Deprecated
    public static int getOrientation(final byte[] jpeg) {
        return getOrientation(new ByteArrayInputStream(jpeg), jpeg.length);
    }
}