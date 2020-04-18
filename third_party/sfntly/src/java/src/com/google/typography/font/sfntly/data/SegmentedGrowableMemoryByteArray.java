/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly.data;

import com.google.typography.font.sfntly.math.FontMath;

import java.util.ArrayList;
import java.util.List;

/**
 * A growable memory implementation of the ByteArray interface.
 * 
 * @author Stuart Gill
 */
final class SegmentedGrowableMemoryByteArray extends ByteArray<SegmentedGrowableMemoryByteArray> {
  private static final int DEFAULT_BUFFER_LOW_BITS = 8;
  
  private List<byte[]> buffers;
  private final int lowBits;

  /**
   * Constructor.
   * 
   */
  public SegmentedGrowableMemoryByteArray() {
    this(DEFAULT_BUFFER_LOW_BITS);
  }

  /**
   * Constructor.
   *
   *  The low bits parameter is used to set the initial size of the first
   * chained memory buffer used internally. That initial buffer is 2^(low bits)
   * in size and each subsequent buffer is roughly double the preceding one. If
   * this is too small then there will be a number of small buffers and the cost
   * of access will increase. If it's too large then there will a lot of
   * allocated but unused space.
   *
   * @param lowBits the number of bits to use as the initial buffer index
   * @throws IndexOutOfBoundsException if the given bounds don't fit within the
   *         byte array given
   */
  public SegmentedGrowableMemoryByteArray(int lowBits) {
    super(0, Integer.MAX_VALUE, true /*growable*/);
    this.buffers = new ArrayList<byte[]>();
    this.lowBits = lowBits;
  }

  @Override
  protected void internalPut(int index, byte b) {
    int bufferIndex = this.bufferIndex(index);
    int bufferOffset = this.bufferOffset(bufferIndex, index);
    byte[] buffer = this.buffer(bufferIndex);
    buffer[bufferOffset] = b;
  }

  @Override
  protected int internalPut(int index, byte[] b, int offset, int length) {
    int copyCount = 0;
    while (copyCount < length) {
      int bufferIndex = this.bufferIndex(index);
      int bufferOffset = this.bufferOffset(bufferIndex, index);
      byte[] buffer = this.buffer(bufferIndex);
      int copyLength = Math.min(length - copyCount, buffer.length - bufferOffset);
      System.arraycopy(b, offset, buffer, bufferOffset, copyLength);
      index += copyLength;
      offset += copyLength;
      copyCount += copyLength;
    }
    return copyCount;
  }

  @Override
  protected int internalGet(int index) {
    int bufferIndex = this.bufferIndex(index);
    int bufferOffset = this.bufferOffset(bufferIndex, index);
    byte[] buffer = this.buffer(bufferIndex);
    return buffer[bufferOffset];
  }

  @Override
  protected int internalGet(int index, byte[] b, int offset, int length) {
    int copyCount = 0;
    while (copyCount < length) {
      int bufferIndex = this.bufferIndex(index);
      int bufferOffset = this.bufferOffset(bufferIndex, index);
      byte[] buffer = this.buffer(bufferIndex);
      int copyLength = Math.min(length - copyCount, buffer.length - bufferOffset);
      System.arraycopy(buffer, bufferOffset, b, offset, copyLength);
      index += copyLength;
      offset += copyLength;
      copyCount += copyLength;
    }
    return copyCount;
  }

  @Override
  public void close() {
    this.buffers = null;
 }

  /**
   * Calculate the offset within a given buffer from the buffer index and the
   * overall index in the overall "virtual" buffer.
   *
   * @param bufferIndex the index of the buffer where the data is found
   * @param index the index into the overall "virtual" buffer
   * @return the offset in the buffer
   * @see #bufferIndex(int)
   */
  private int bufferOffset(int bufferIndex, int index) {
    return index & ~(0x01 << Math.max(this.lowBits, bufferIndex + this.lowBits - 1));
  }

  /**
   * Calculate the buffer index from the index in the overall "virtual" buffer.
   *
   * @param index the index into the overall "virtual" buffer
   * @return the buffer index
   * @see #bufferOffset(int, int)
   */
  private int bufferIndex(int index) {
    return FontMath.log2(index >> this.lowBits) + 1;
  }

  /**
   * Get the buffer at the buffer index specified. If the buffer has not
   * previously been allocated then it is allocated. Any buffers between this
   * buffer and the last previously allocated buffer are also allocated.
   *
   * @param index the buffer index
   * @return the buffer specified
   */
  private byte[] buffer(int index) {
    byte[] b = null;
    if (index >= this.buffers.size()) {
      // must fill all buffers between the last one created and this one
      for (int i = this.buffers.size(); i < index + 1; i++) {
        int bufferSize = 1 << (Math.max(0, i - 1) + this.lowBits);
        b = new byte[bufferSize];
        this.buffers.add(b);
      }
    }
    b = this.buffers.get(index);
    return b;
  }
}
