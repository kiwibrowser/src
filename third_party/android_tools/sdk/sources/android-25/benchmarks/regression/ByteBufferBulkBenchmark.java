/*
 * Copyright (C) 2016 Google Inc.
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

public class ByteBufferBulkBenchmark {
    @Param({"true", "false"}) private boolean aligned;


    enum MyBufferType {
        DIRECT, HEAP, MAPPED
    }
    @Param private MyBufferType srcBufferType;
    @Param private MyBufferType dataBufferType;

    @Param({"4096", "1232896"}) private int bufferSize;

    public static ByteBuffer newBuffer(boolean aligned, MyBufferType bufferType, int bsize) throws IOException {
        int size = aligned ?  bsize : bsize + 8 + 1;
        ByteBuffer result = null;
        switch (bufferType) {
        case DIRECT:
            result = ByteBuffer.allocateDirect(size);
            break;
        case HEAP:
            result = ByteBuffer.allocate(size);
            break;
        case MAPPED:
            File tmpFile = File.createTempFile("MappedByteBufferTest", ".tmp");
            tmpFile.createNewFile();
            tmpFile.deleteOnExit();
            RandomAccessFile raf = new RandomAccessFile(tmpFile, "rw");
            raf.setLength(size);
            FileChannel fc = raf.getChannel();
            result = fc.map(FileChannel.MapMode.READ_WRITE, 0, fc.size());
            break;
        }
        result.position(aligned ? 0 : 1);
        return result;
    }

    public void timeByteBuffer_putByteBuffer(int reps) throws Exception {
        ByteBuffer src = ByteBufferBulkBenchmark.newBuffer(aligned, srcBufferType, bufferSize);
        ByteBuffer data = ByteBufferBulkBenchmark.newBuffer(aligned, dataBufferType, bufferSize);
        for (int rep = 0; rep < reps; ++rep) {
            src.position(aligned ? 0 : 1);
            data.position(aligned ? 0 : 1 );
            src.put(data);
        }
    }

}
