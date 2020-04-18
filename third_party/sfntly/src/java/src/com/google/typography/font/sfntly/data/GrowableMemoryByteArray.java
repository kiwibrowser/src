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

import java.io.IOException;
import java.io.OutputStream;

/**
 * A growable memory implementation of the ByteArray interface.
 * 
 * @author Stuart Gill
 */
final class GrowableMemoryByteArray extends ByteArray<GrowableMemoryByteArray> {

  private static final int INITIAL_LENGTH = 256;
  private byte[] b;

  public GrowableMemoryByteArray() {
    super(0, Integer.MAX_VALUE, true /*growable*/);
    b = new byte[INITIAL_LENGTH];
  }

  @Override
  protected void internalPut(int index, byte b) {
    growTo(index + 1);
    this.b[index] = b;
  }

  @Override
  protected int internalPut(int index, byte[] b, int offset, int length) {
    growTo(index + length);
    System.arraycopy(b, offset, this.b, index, length);
    return length;
  }

  @Override
  protected int internalGet(int index) {
    return this.b[index];
  }

  @Override
  protected int internalGet(int index, byte[] b, int offset, int length) {
    System.arraycopy(this.b, index, b, offset, length);
    return length;
  }

  @Override
  public void close() {
   
    this.b = null;
  }
  
  @Override
  public int copyTo(OutputStream os, int offset, int length) throws IOException {
    os.write(b, offset, length);
    return length;
  }

  private void growTo(int newSize) {
    if (newSize <= b.length) {
      return;
    }
    newSize = Math.max(newSize, b.length * 2);
    byte[] newArray = new byte[newSize];
    System.arraycopy(b, 0, newArray, 0, b.length);
    b = newArray;
  }
}
