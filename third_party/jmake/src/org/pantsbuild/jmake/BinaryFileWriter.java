/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

/**
 * Basic operations for writing to a byte array representing a binary file.
 *
 * @author  Misha Dmitriev
 *  30 January 2002
 */
public class BinaryFileWriter {

    protected byte[] buf;
    protected int curBufSize,  bufInc,  curBufPos,  threshold;
    private boolean bufferIncreaseAllowed = true;

    protected void initBuf(int initSize) {
        buf = new byte[initSize];
        curBufSize = initSize;
        bufInc = initSize / 5;
        curBufPos = 0;
        threshold = curBufSize - bufInc;
    }

    protected void increaseBuf() {
        if (!bufferIncreaseAllowed) {
            return;
        }
        byte newBuf[] = new byte[curBufSize + bufInc];
        System.arraycopy(buf, 0, newBuf, 0, curBufPos);
        buf = newBuf;
        curBufSize = buf.length;
        threshold = curBufSize - bufInc;
    }

    // This should be called with false only when we are sure that we set the exact size of the buffer
    // and there is no need to increase it.
    protected void setBufferIncreaseMode(boolean increaseMode) {
        bufferIncreaseAllowed = increaseMode;
    }

    public byte[] getBuffer() {
        return buf;
    }


    protected void writeByte(byte b) {
        if (curBufPos > threshold) {
            increaseBuf();
        }
        buf[curBufPos++] = b;
    }

    protected void writeChar(int ch) {
        buf[curBufPos++] = (byte) ((ch >> 8) & 255);
        buf[curBufPos++] = (byte) (ch & 255);
        if (curBufPos > threshold) {
            increaseBuf();
        }
    }

    protected void writeInt(int i) {
        buf[curBufPos++] = (byte) ((i >> 24) & 255);
        buf[curBufPos++] = (byte) ((i >> 16) & 255);
        buf[curBufPos++] = (byte) ((i >> 8) & 255);
        buf[curBufPos++] = (byte) (i & 255);
        if (curBufPos > threshold) {
            increaseBuf();
        }
    }

    protected void writeLong(long l) {
        buf[curBufPos++] = (byte) ((l >> 56) & 255);
        buf[curBufPos++] = (byte) ((l >> 48) & 255);
        buf[curBufPos++] = (byte) ((l >> 40) & 255);
        buf[curBufPos++] = (byte) ((l >> 32) & 255);
        buf[curBufPos++] = (byte) ((l >> 24) & 255);
        buf[curBufPos++] = (byte) ((l >> 16) & 255);
        buf[curBufPos++] = (byte) ((l >> 8) & 255);
        buf[curBufPos++] = (byte) (l & 255);
        if (curBufPos > threshold) {
            increaseBuf();
        }
    }

    protected void writeFloat(float f) {
        int i = Float.floatToIntBits(f);
        writeInt(i);
    }

    protected void writeDouble(double d) {
        long l = Double.doubleToLongBits(d);
        writeLong(l);
    }
}
