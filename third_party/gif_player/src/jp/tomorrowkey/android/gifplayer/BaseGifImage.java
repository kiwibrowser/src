/*
 * Copyright (C) 2015 The Gifplayer Authors. All Rights Reserved.
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

package jp.tomorrowkey.android.gifplayer;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 * A base wrapper for GIF image data.
 */
public class BaseGifImage {
    private final byte[] mData;
    private final int mOffset;
    private int mWidth;
    private int mHeight;

    private static final byte[] sColorTableBuffer = new byte[256 * 3];

    int mHeaderSize;
    boolean mGlobalColorTableUsed;
    boolean mError;
    int[] mGlobalColorTable = new int[256];
    int mGlobalColorTableSize;
    int mBackgroundColor;
    int mBackgroundIndex;

    public BaseGifImage(byte[] data) {
        this(data, 0);
    }

    /**
     * Unlike the desktop JVM, ByteBuffers created with allocateDirect() can (and since froyo, do)
     * provide a backing array, enabling zero-copy interop with native code. However, they are
     * aligned on a byte boundary, meaning that they often have an arrayOffset as well - in those
     * cases, we can avoid allocating large byte arrays and a copy.
     */
    public BaseGifImage(ByteBuffer data) {
        this(bufferToArray(data), bufferToOffset(data));
    }

    private static int bufferToOffset(ByteBuffer buffer) {
        return buffer.hasArray() ? buffer.arrayOffset() : 0;
    }

    private static byte[] bufferToArray(ByteBuffer buffer) {
        if (buffer.hasArray()) {
            return buffer.array();
        } else {
            int position = buffer.position();
            try {
                byte[] newData = new byte[buffer.capacity()];
                buffer.get(newData);
                return newData;
            } finally {
                buffer.position(position);
            }
        }
    }

    public BaseGifImage(byte[] data, int offset) {
        mData = data;
        mOffset = offset;

        GifHeaderStream stream = new GifHeaderStream(data);
        stream.skip(offset);
        try {
            readHeader(stream);
            mHeaderSize = stream.getPosition();
        } catch (IOException e) {
            mError = true;
        }

        try {
            stream.close();
        } catch (IOException e) {
            // Ignore
        }
    }

    public byte[] getData() {
        return mData;
    }

    public int getDataOffset() {
        return mOffset;
    }

    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    /**
     * Returns an estimate of the size of the object in bytes.
     */
    public int getSizeEstimate() {
        return mData.length + mGlobalColorTable.length * 4;
    }

    /**
     * Reads GIF file header information.
     */
    private void readHeader(InputStream stream) throws IOException {
        boolean valid = stream.read() == 'G';
        valid = valid && stream.read() == 'I';
        valid = valid && stream.read() == 'F';
        if (!valid) {
            mError = true;
            return;
        }

        // Skip the next three letter, which represent the variation of the GIF standard.
        stream.skip(3);

        readLogicalScreenDescriptor(stream);

        if (mGlobalColorTableUsed && !mError) {
            readColorTable(stream, mGlobalColorTable, mGlobalColorTableSize);
            mBackgroundColor = mGlobalColorTable[mBackgroundIndex];
        }
    }

    /**
     * Reads Logical Screen Descriptor
     */
    private void readLogicalScreenDescriptor(InputStream stream) throws IOException {
        // logical screen size
        mWidth = readShort(stream);
        mHeight = readShort(stream);
        // packed fields
        int packed = stream.read();
        mGlobalColorTableUsed = (packed & 0x80) != 0; // 1 : global color table flag
        // 2-4 : color resolution - ignore
        // 5 : gct sort flag - ignore
        mGlobalColorTableSize = 2 << (packed & 7); // 6-8 : gct size
        mBackgroundIndex = stream.read();
        stream.skip(1); // pixel aspect ratio - ignore
    }

    /**
     * Reads color table as 256 RGB integer values
     *
     * @param ncolors int number of colors to read
     */
    static boolean readColorTable(InputStream stream, int[] colorTable, int ncolors)
            throws IOException {
        synchronized (sColorTableBuffer) {
            int nbytes = 3 * ncolors;
            int n = stream.read(sColorTableBuffer, 0, nbytes);
            if (n < nbytes) {
                return false;
            } else {
                int i = 0;
                int j = 0;
                while (i < ncolors) {
                    int r = sColorTableBuffer[j++] & 0xff;
                    int g = sColorTableBuffer[j++] & 0xff;
                    int b = sColorTableBuffer[j++] & 0xff;
                    colorTable[i++] = 0xff000000 | (r << 16) | (g << 8) | b;
                }
            }
        }

        return true;
    }

    /**
     * Reads next 16-bit value, LSB first
     */
    private int readShort(InputStream stream) throws IOException {
        // read 16-bit value, LSB first
        return stream.read() | (stream.read() << 8);
    }

    private final class GifHeaderStream extends ByteArrayInputStream {

        private GifHeaderStream(byte[] buf) {
            super(buf);
        }

        public int getPosition() {
            return pos;
        }
    }
}
