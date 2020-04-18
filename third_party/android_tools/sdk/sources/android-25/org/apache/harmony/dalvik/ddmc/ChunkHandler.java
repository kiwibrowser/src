/*
 * Copyright (C) 2007 The Android Open Source Project
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

package org.apache.harmony.dalvik.ddmc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Handle a chunk of data sent from a DDM server.
 *
 * To handle a chunk type, sub-class ChunkHandler and register your class
 * with DdmServer.
 */
public abstract class ChunkHandler {

    public static final ByteOrder CHUNK_ORDER = ByteOrder.BIG_ENDIAN;

    public static final int CHUNK_FAIL = type("FAIL");


    public ChunkHandler() {}

    /**
     * Called when the DDM server connects.  The handler is allowed to
     * send messages to the server.
     */
    public abstract void connected();

    /**
     * Called when the DDM server disconnects.  Can be used to disable
     * periodic transmissions or clean up saved state.
     */
    public abstract void disconnected();

    /**
     * Handle a single chunk of data.  "request" includes the type and
     * the chunk payload.
     *
     * Returns a response in a Chunk.
     */
    public abstract Chunk handleChunk(Chunk request);

    /**
     * Create a FAIL chunk.  The "handleChunk" methods can use this to
     * return an error message when they are not able to process a chunk.
     */
    public static Chunk createFailChunk(int errorCode, String msg) {
        if (msg == null)
            msg = "";

        ByteBuffer out = ByteBuffer.allocate(8 + msg.length() * 2);
        out.order(ChunkHandler.CHUNK_ORDER);
        out.putInt(errorCode);
        out.putInt(msg.length());
        putString(out, msg);

        return new Chunk(CHUNK_FAIL, out);
    }

    /**
     * Utility function to wrap a ByteBuffer around a Chunk.
     */
    public static ByteBuffer wrapChunk(Chunk request) {
        ByteBuffer in;

        in = ByteBuffer.wrap(request.data, request.offset, request.length);
        in.order(CHUNK_ORDER);
        return in;
    }


    /**
     * Utility function to copy a String out of a ByteBuffer.
     *
     * This is here because multiple chunk handlers can make use of it,
     * and there's nowhere better to put it.
     */
    public static String getString(ByteBuffer buf, int len) {
        char[] data = new char[len];
        for (int i = 0; i < len; i++)
            data[i] = buf.getChar();
        return new String(data);
    }

    /**
     * Utility function to copy a String into a ByteBuffer.
     */
    public static void putString(ByteBuffer buf, String str) {
        int len = str.length();
        for (int i = 0; i < len; i++)
            buf.putChar(str.charAt(i));
    }

    /**
     * Convert a 4-character string to a 32-bit type.
     */
    public static int type(String typeName) {
        if (typeName.length() != 4) {
            throw new IllegalArgumentException("Bad type name: " + typeName);
        }
        int result = 0;
        for (int i = 0; i < 4; ++i) {
            result = ((result << 8) | (typeName.charAt(i) & 0xff));
        }
        return result;
    }

    /**
     * Convert an integer type to a 4-character string.
     */
    public static String name(int type)
    {
        char[] ascii = new char[4];

        ascii[0] = (char) ((type >> 24) & 0xff);
        ascii[1] = (char) ((type >> 16) & 0xff);
        ascii[2] = (char) ((type >> 8) & 0xff);
        ascii[3] = (char) (type & 0xff);

        return new String(ascii);
    }

}
