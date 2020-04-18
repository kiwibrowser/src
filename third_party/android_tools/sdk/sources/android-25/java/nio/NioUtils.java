/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package java.nio;

import java.io.Closeable;
import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.FileChannel;
import java.util.Set;

import static android.system.OsConstants.*;
import sun.misc.Cleaner;
import sun.nio.ch.DirectBuffer;
import sun.nio.ch.FileChannelImpl;

/**
 * @hide internal use only
 */
public final class NioUtils {
    private NioUtils() {
    }

    public static void freeDirectBuffer(ByteBuffer buffer) {
        if (buffer == null) {
            return;
        }

        DirectByteBuffer dbb = (DirectByteBuffer) buffer;
        // Run the cleaner early, if one is defined.
        if (dbb.cleaner != null) {
            dbb.cleaner.clean();
        }

        dbb.memoryRef.free();
    }

    /**
     * Returns the int file descriptor from within the given FileChannel 'fc'.
     */
    public static FileDescriptor getFD(FileChannel fc) {
        return ((FileChannelImpl) fc).fd;
    }

    /**
     * Helps bridge between io and nio.
     */
    public static FileChannel newFileChannel(Closeable ioObject, FileDescriptor fd, int mode) {
        boolean readable = (mode & (O_RDONLY | O_RDWR | O_SYNC)) != 0;
        boolean writable = (mode & (O_WRONLY | O_RDWR | O_SYNC)) != 0;
        boolean append = (mode & O_APPEND) != 0;
        return FileChannelImpl.open(fd, null, readable, writable, append, ioObject);
    }

    /**
     * Exposes the array backing a non-direct ByteBuffer, even if the ByteBuffer is read-only.
     * Normally, attempting to access the array backing a read-only buffer throws.
     */
    public static byte[] unsafeArray(ByteBuffer b) {
        return b.array();
    }

    /**
     * Exposes the array offset for the array backing a non-direct ByteBuffer,
     * even if the ByteBuffer is read-only.
     */
    public static int unsafeArrayOffset(ByteBuffer b) {
        return b.arrayOffset();
    }
}
