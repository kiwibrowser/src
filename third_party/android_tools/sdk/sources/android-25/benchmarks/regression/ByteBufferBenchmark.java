/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks.regression;

import com.google.caliper.Param;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.DoubleBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import java.nio.ShortBuffer;
import java.nio.channels.FileChannel;

public class ByteBufferBenchmark {
    public enum MyByteOrder {
        BIG(ByteOrder.BIG_ENDIAN), LITTLE(ByteOrder.LITTLE_ENDIAN);
        final ByteOrder byteOrder;
        MyByteOrder(ByteOrder byteOrder) {
            this.byteOrder = byteOrder;
        }
    }

    @Param private MyByteOrder byteOrder;

    @Param({"true", "false"}) private boolean aligned;

    enum MyBufferType {
        DIRECT, HEAP, MAPPED;
    }
    @Param private MyBufferType bufferType;

    public static ByteBuffer newBuffer(MyByteOrder byteOrder, boolean aligned, MyBufferType bufferType) throws IOException {
        int size = aligned ? 8192 : 8192 + 8 + 1;
        ByteBuffer result = null;
        switch (bufferType) {
        case DIRECT:
            result = ByteBuffer.allocateDirect(size);
            break;
        case HEAP:
            result = ByteBuffer.allocate(size);
            break;
        case MAPPED:
            File tmpFile = new File("/sdcard/bm.tmp");
            if (new File("/tmp").isDirectory()) {
                // We're running on the desktop.
                tmpFile = File.createTempFile("MappedByteBufferTest", ".tmp");
            }
            tmpFile.createNewFile();
            tmpFile.deleteOnExit();
            RandomAccessFile raf = new RandomAccessFile(tmpFile, "rw");
            raf.setLength(8192*8);
            FileChannel fc = raf.getChannel();
            result = fc.map(FileChannel.MapMode.READ_WRITE, 0, fc.size());
            break;
        }
        result.order(byteOrder.byteOrder);
        result.position(aligned ? 0 : 1);
        return result;
    }

    //
    // peeking
    //

    public void timeByteBuffer_getByte(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.get();
            }
        }
    }

    public void timeByteBuffer_getByteArray(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        byte[] dst = new byte[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(aligned ? 0 : 1);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getByte_indexed(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.get(i);
            }
        }
    }

    public void timeByteBuffer_getChar(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getChar();
            }
        }
    }

    public void timeCharBuffer_getCharArray(int reps) throws Exception {
        CharBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asCharBuffer();
        char[] dst = new char[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getChar_indexed(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getChar(i * 2);
            }
        }
    }

    public void timeByteBuffer_getDouble(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getDouble();
            }
        }
    }

    public void timeDoubleBuffer_getDoubleArray(int reps) throws Exception {
        DoubleBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asDoubleBuffer();
        double[] dst = new double[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getFloat(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getFloat();
            }
        }
    }

    public void timeFloatBuffer_getFloatArray(int reps) throws Exception {
        FloatBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asFloatBuffer();
        float[] dst = new float[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getInt(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getInt();
            }
        }
    }

    public void timeIntBuffer_getIntArray(int reps) throws Exception {
        IntBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asIntBuffer();
        int[] dst = new int[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getLong(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getLong();
            }
        }
    }

    public void timeLongBuffer_getLongArray(int reps) throws Exception {
        LongBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asLongBuffer();
        long[] dst = new long[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    public void timeByteBuffer_getShort(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.getShort();
            }
        }
    }

    public void timeShortBuffer_getShortArray(int reps) throws Exception {
        ShortBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asShortBuffer();
        short[] dst = new short[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                src.position(0);
                src.get(dst);
            }
        }
    }

    //
    // poking
    //

    public void timeByteBuffer_putByte(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(0);
            for (int i = 0; i < 1024; ++i) {
                src.put((byte) 0);
            }
        }
    }

    public void timeByteBuffer_putByteArray(int reps) throws Exception {
        ByteBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        byte[] src = new byte[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(aligned ? 0 : 1);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putChar(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putChar(' ');
            }
        }
    }

    public void timeCharBuffer_putCharArray(int reps) throws Exception {
        CharBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asCharBuffer();
        char[] src = new char[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putDouble(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putDouble(0.0);
            }
        }
    }

    public void timeDoubleBuffer_putDoubleArray(int reps) throws Exception {
        DoubleBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asDoubleBuffer();
        double[] src = new double[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putFloat(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putFloat(0.0f);
            }
        }
    }

    public void timeFloatBuffer_putFloatArray(int reps) throws Exception {
        FloatBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asFloatBuffer();
        float[] src = new float[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putInt(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putInt(0);
            }
        }
    }

    public void timeIntBuffer_putIntArray(int reps) throws Exception {
        IntBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asIntBuffer();
        int[] src = new int[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putLong(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putLong(0L);
            }
        }
    }

    public void timeLongBuffer_putLongArray(int reps) throws Exception {
        LongBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asLongBuffer();
        long[] src = new long[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

    public void timeByteBuffer_putShort(int reps) throws Exception {
        ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            for (int i = 0; i < 1024; ++i) {
                src.putShort((short) 0);
            }
        }
    }

    public void timeShortBuffer_putShortArray(int reps) throws Exception {
        ShortBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType).asShortBuffer();
        short[] src = new short[1024];
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < 1024; ++i) {
                dst.position(0);
                dst.put(src);
            }
        }
    }

/*
    public void time_new_byteArray(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            byte[] bs = new byte[8192];
        }
    }

    public void time_ByteBuffer_allocate(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            ByteBuffer bs = ByteBuffer.allocate(8192);
        }
    }
    */
}
