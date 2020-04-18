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

/**
 * A chunk of DDM data.  This is really just meant to hold a few pieces
 * of data together.
 *
 * The "offset" and "length" fields are present so handlers can over-allocate
 * or share byte buffers.
 */
public class Chunk {

    /*
     * Public members.  Do not rename without updating the VM.
     */
    public int type;                // chunk type
    public byte[] data;             // chunk data
    public int offset, length;      // position within "data"

    /**
     * Blank constructor.  Fill in your own fields.
     */
    public Chunk() {}

    /**
     * Constructor with all fields.
     */
    public Chunk(int type, byte[] data, int offset, int length) {
        this.type = type;
        this.data = data;
        this.offset = offset;
        this.length = length;
    }

    /**
     * Construct from a ByteBuffer.  The chunk is assumed to start at
     * offset 0 and continue to the current position.
     */
    public Chunk(int type, ByteBuffer buf) {
        this.type = type;

        this.data = buf.array();
        this.offset = buf.arrayOffset();
        this.length = buf.position();
    }
}

