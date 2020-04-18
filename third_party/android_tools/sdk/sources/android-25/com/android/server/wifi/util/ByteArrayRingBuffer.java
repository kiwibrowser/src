/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.server.wifi.util;

import java.util.ArrayList;

/**
 * A ring buffer where each element of the ring is itself a byte array.
 */
public class ByteArrayRingBuffer {
    private ArrayList<byte[]> mArrayList;
    private int mMaxBytes;
    private int mBytesUsed;

    /**
     * Creates a ring buffer that holds at most |maxBytes| of data. The overhead for each element
     * is not included in this limit.
     * @param maxBytes upper bound on the amount of data to hold
     */
    public ByteArrayRingBuffer(int maxBytes) {
        if (maxBytes < 1) {
            throw new IllegalArgumentException();
        }
        mArrayList = new ArrayList<byte[]>();
        mMaxBytes = maxBytes;
        mBytesUsed = 0;
    }

    /**
     * Adds |newData| to the ring buffer. Removes existing entries to make room, if necessary.
     * Existing entries are removed in FIFO order.
     * <p><b>Note:</b> will fail if |newData| itself exceeds the size limit for this buffer.
     * Will first remove all existing entries in this case. (This guarantees that the ring buffer
     * always represents a contiguous sequence of data.)
     * @param newData data to be added to the ring
     * @return true if the data was added
     */
    public boolean appendBuffer(byte[] newData) {
        pruneToSize(mMaxBytes - newData.length);
        if (mBytesUsed + newData.length > mMaxBytes) {
            return false;
        }

        mArrayList.add(newData);
        mBytesUsed += newData.length;
        return true;
    }

    /**
     * Returns the |i|-th element of the ring. The element retains its position in the ring.
     * @param i
     * @return the requested element
     */
    public byte[] getBuffer(int i) {
        return mArrayList.get(i);
    }

    /**
     * Returns the number of elements present in the ring.
     * @return the number of elements present
     */
    public int getNumBuffers() {
        return mArrayList.size();
    }

    /**
     * Resize the buffer, removing existing data if necessary.
     * @param maxBytes upper bound on the amount of data to hold
     */
    public void resize(int maxBytes) {
        pruneToSize(maxBytes);
        mMaxBytes = maxBytes;
    }

    private void pruneToSize(int sizeBytes) {
        int newBytesUsed = mBytesUsed;
        int i = 0;
        while (i < mArrayList.size() && newBytesUsed > sizeBytes) {
            newBytesUsed -= mArrayList.get(i).length;
            i++;
        }
        mArrayList.subList(0, i).clear();
        mBytesUsed = newBytesUsed;
    }
}
