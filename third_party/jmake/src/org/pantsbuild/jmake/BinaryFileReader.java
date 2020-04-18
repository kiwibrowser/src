/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.DataInputStream;
import java.io.ByteArrayInputStream;
import java.io.IOException;

/**
 * Basic operations for reading a byte array representing a binary file.
 *
 * @author  Misha Dmitriev
 *  10 November 2001
 */
public class BinaryFileReader {

    protected byte[] buf;
    protected int curBufPos;
    protected String fileFullPath;  // Required only for nice error reports

    protected void initBuf(byte[] buf, String fileFullPath) {
        this.buf = buf;
        curBufPos = 0;
        this.fileFullPath = fileFullPath;
    }

    protected char nextChar() {
        return (char) (((buf[curBufPos++] & 255) << 8) + (buf[curBufPos++] & 255));
    }

    protected char getChar(int bufPos) {
        return (char) (((buf[bufPos] & 255) << 8) + (buf[bufPos+1] & 255));
    }

    protected int nextInt() {
        return ((buf[curBufPos++] & 255) << 24) + ((buf[curBufPos++] & 255) << 16) +
                ((buf[curBufPos++] & 255) << 8) + (buf[curBufPos++] & 255);
    }

    protected int getInt(int bufPos) {
        return ((buf[bufPos] & 255) << 24) + ((buf[bufPos+1] & 255) << 16) +
                ((buf[bufPos+2] & 255) << 8) + (buf[bufPos+3] & 255);
    }

    protected long nextLong() {
        long res = getLong(curBufPos);
        curBufPos += 8;
        return res;
    }

    protected long getLong(int bufPos) {
        DataInputStream bufin =
                new DataInputStream(new ByteArrayInputStream(buf, bufPos, 8));
        try {
            return bufin.readLong();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    protected float nextFloat() {
        float res = getFloat(curBufPos);
        curBufPos += 4;
        return res;
    }

    protected float getFloat(int bufPos) {
        DataInputStream bufin =
                new DataInputStream(new ByteArrayInputStream(buf, bufPos, 4));
        try {
            return bufin.readFloat();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    protected double nextDouble() {
        double res = getDouble(curBufPos);
        curBufPos += 8;
        return res;
    }

    protected double getDouble(int bufPos) {
        DataInputStream bufin =
                new DataInputStream(new ByteArrayInputStream(buf, bufPos, 8));
        try {
            return bufin.readDouble();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }
}
