/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.dex;

import com.android.dex.util.ByteArrayByteInput;
import junit.framework.TestCase;

public final class EncodedValueReaderTest extends TestCase {

    public void testReadByte() {
        assertEquals((byte) 0x80, readerOf(0, 0x80).readByte());
        assertEquals((byte) 0xff, readerOf(0, 0xff).readByte());
        assertEquals((byte) 0x00, readerOf(0, 0x00).readByte());
        assertEquals((byte) 0x01, readerOf(0, 0x01).readByte());
        assertEquals((byte) 0x7f, readerOf(0, 0x7f).readByte());
    }

    public void testReadShort() {
        assertEquals((short) 0x8000, readerOf(34, 0x00, 0x80).readShort());
        assertEquals((short)      0, readerOf( 2, 0x00).readShort());
        assertEquals((short)   0xab, readerOf(34, 0xab, 0x00).readShort());
        assertEquals((short) 0xabcd, readerOf(34, 0xcd, 0xab).readShort());
        assertEquals((short) 0x7FFF, readerOf(34, 0xff, 0x7f).readShort());
    }

    public void testReadInt() {
        assertEquals(0x80000000, readerOf(100, 0x00, 0x00, 0x00, 0x80).readInt());
        assertEquals(      0x00, readerOf(  4, 0x00).readInt());
        assertEquals(      0xab, readerOf( 36, 0xab, 0x00).readInt());
        assertEquals(    0xabcd, readerOf( 68, 0xcd, 0xab, 0x00).readInt());
        assertEquals(  0xabcdef, readerOf(100, 0xef, 0xcd, 0xab, 0x00).readInt());
        assertEquals(0xabcdef01, readerOf(100, 0x01, 0xef, 0xcd, 0xab).readInt());
        assertEquals(0x7fffffff, readerOf(100, 0xff, 0xff, 0xff, 127).readInt());
    }

    public void testReadLong() {
        assertEquals(0x8000000000000000L, readerOf( -26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80).readLong());
        assertEquals(              0x00L, readerOf(   6, 0x00).readLong());
        assertEquals(              0xabL, readerOf(  38, 0xab, 0x00).readLong());
        assertEquals(            0xabcdL, readerOf(  70, 0xcd, 0xab, 0x00).readLong());
        assertEquals(          0xabcdefL, readerOf( 102, 0xef, 0xcd, 0xab, 0x00).readLong());
        assertEquals(        0xabcdef01L, readerOf(-122, 0x01, 0xef, 0xcd, 0xab, 0x00).readLong());
        assertEquals(      0xabcdef0123L, readerOf( -90, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x00).readLong());
        assertEquals(    0xabcdef012345L, readerOf( -58, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x00).readLong());
        assertEquals(  0xabcdef01234567L, readerOf( -26, 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x00).readLong());
        assertEquals(0xabcdef0123456789L, readerOf( -26, 0x89, 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab).readLong());
        assertEquals(0x7fffffffffffffffL, readerOf( -26, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f).readLong());
    }

    public void testReadFloat() {
        assertEquals(Float.NEGATIVE_INFINITY, readerOf(48, -128, -1).readFloat());
        assertEquals(Float.POSITIVE_INFINITY, readerOf(48, -128, 127).readFloat());
        assertEquals(Float.NaN, readerOf(48, -64, 127).readFloat());
        assertEquals(-0.0f, readerOf(16, -128).readFloat());
        assertEquals(0.0f, readerOf(16, 0).readFloat());
        assertEquals(0.5f, readerOf(16, 63).readFloat());
        assertEquals(1f, readerOf(48, -128, 63).readFloat());
        assertEquals(1.0E06f, readerOf(80, 36, 116, 73).readFloat());
        assertEquals(1.0E12f, readerOf(112, -91, -44, 104, 83).readFloat());
    }

    public void testReadDouble() {
        assertEquals(Double.NEGATIVE_INFINITY, readerOf(49, -16, -1).readDouble());
        assertEquals(Double.POSITIVE_INFINITY, readerOf(49, -16, 127).readDouble());
        assertEquals(Double.NaN, readerOf(49, -8, 127).readDouble());
        assertEquals(-0.0, readerOf(17, -128).readDouble());
        assertEquals(0.0, readerOf(17, 0).readDouble());
        assertEquals(0.5, readerOf(49, -32, 63).readDouble());
        assertEquals(1.0, readerOf(49, -16, 63).readDouble());
        assertEquals(1.0E06, readerOf(113, -128, -124, 46, 65).readDouble());
        assertEquals(1.0E12, readerOf(-111, -94, -108, 26, 109, 66).readDouble());
        assertEquals(1.0E24, readerOf(-15, -76, -99, -39, 121, 67, 120, -22, 68).readDouble());
    }

    public void testReadChar() {
        assertEquals('\u0000', readerOf( 3, 0x00).readChar());
        assertEquals('\u00ab', readerOf( 3, 0xab).readChar());
        assertEquals('\uabcd', readerOf(35, 0xcd, 0xab).readChar());
        assertEquals('\uffff', readerOf(35, 0xff, 0xff).readChar());
    }

    public void testReadBoolean() {
        assertEquals(true, readerOf(63).readBoolean());
        assertEquals(false, readerOf(31).readBoolean());
    }

    public void testReadNull() {
        readerOf(30).readNull();
    }

    public void testReadReference() {
        assertEquals(      0xab, readerOf(0x17, 0xab).readString());
        assertEquals(    0xabcd, readerOf(0x37, 0xcd, 0xab).readString());
        assertEquals(  0xabcdef, readerOf(0x57, 0xef, 0xcd, 0xab).readString());
        assertEquals(0xabcdef01, readerOf(0x77, 0x01, 0xef, 0xcd, 0xab).readString());
    }

    public void testReadWrongType() {
        try {
            readerOf(0x17, 0xab).readField();
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    private EncodedValueReader readerOf(int... bytes) {
        byte[] data = new byte[bytes.length];
        for (int i = 0; i < bytes.length; i++) {
            data[i] = (byte) bytes[i];
        }
        return new EncodedValueReader(new ByteArrayByteInput(data));
    }
}
