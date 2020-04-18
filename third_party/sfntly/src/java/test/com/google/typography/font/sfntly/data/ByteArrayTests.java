/*
 * Copyright 2011 Google Inc. All Rights Reserved.
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

import com.google.typography.font.sfntly.testutils.TestUtils;

import junit.framework.TestCase;


/**
 * @author Stuart Gill
 *
 */
public class ByteArrayTests extends TestCase {

  private static final int[] BYTE_ARRAY_SIZES =
      new int[] {1, 7, 127, 128, 129, 255, 256, 257, 666, 1023, 10000, 0xffff, 0x10000};


  public ByteArrayTests(String name) {
    super(name);
  }

  public void testMemoryByteArray() throws Exception {
    for (int size : BYTE_ARRAY_SIZES) {
      byteArrayTester(fillTestByteArray(new MemoryByteArray(size), size));
    }
  }

  public void testGrowableMemoryByteArray() throws Exception {
    for (int size : BYTE_ARRAY_SIZES) {
      byteArrayTester(fillTestByteArray(new GrowableMemoryByteArray(), 100));
    }
  }

  private void byteArrayTester(ByteArray<? extends ByteArray<?>> ba) throws Exception {
    copyTest(ba);
    // slicingCopyTest(ba);
  }

  private void copyTest(ByteArray<? extends ByteArray<?>> ba) throws Exception {
    MemoryByteArray fixedCopy = new MemoryByteArray(ba.length());
    ba.copyTo(fixedCopy);
    assertEquals(ba.length(), fixedCopy.length());
    readComparison(ba, fixedCopy);

    GrowableMemoryByteArray growableCopy = new GrowableMemoryByteArray();
    ba.copyTo(growableCopy);
    assertEquals(ba.length(), growableCopy.length());
    readComparison(ba, growableCopy);
  }

  private void readComparison(
      ByteArray<? extends ByteArray<?>> ba1, ByteArray<? extends ByteArray<?>> ba2)
      throws Exception {
    // single byte reads
    for (int i = 0; i < ba1.length(); i++) {
      int b = ba1.get(i);
      if (b != ba2.get(i)) {
        int fred = 12;
      }
      assertEquals(b, ba2.get(i));
    }

    byte[] b1;
    byte[] b2;

    // buffer reads
    int increments = Math.max(ba1.length() / 11, 1);
    for (int bufferSize = 1; bufferSize < ba1.length(); bufferSize += increments) {
      byte[] buffer = new byte[bufferSize];
      b1 = readByteArrayWithBuffer(ba1, buffer);
      b2 = readByteArrayWithBuffer(ba2, buffer);

      assertTrue(TestUtils.equals(b1, 0, b2, 0, ba1.length()));
    }

    // sliding window reads
    for (int windowSize = 1; windowSize < ba1.length(); windowSize += increments) {
      b1 = readByteArrayWithSlidingWindow(ba1, windowSize);
      b2 = readByteArrayWithSlidingWindow(ba2, windowSize);

      assertTrue(TestUtils.equals(b1, 0, b2, 0, ba1.length()));
    }
  }

  private static byte[] readByteArrayWithBuffer(
      ByteArray<? extends ByteArray<?>> ba, byte[] buffer) {
    byte[] b = new byte[ba.length()];

    int index = 0;
    while (index < ba.length()) {
      int bytesRead = ba.get(index, buffer);
      System.arraycopy(buffer, 0, b, index, bytesRead);
      index += bytesRead;
    }
    return b;
  }

  private static byte[] readByteArrayWithSlidingWindow(
      ByteArray<? extends ByteArray<?>> ba, int windowSize) {
    byte[] b = new byte[ba.length()];

    int index = 0;
    while (index < ba.length()) {
      windowSize = Math.min(windowSize, b.length - index);
      int bytesRead = ba.get(index, b, index, windowSize);
      index += bytesRead;
    }
    return b;
  }

  private static ByteArray<? extends ByteArray<?>> fillTestByteArray(
      ByteArray<? extends ByteArray<?>> ba, int size) {
    for (int i = 0; i < size; i++) {
      ba.put(i, (byte) (i % 256));
    }
    return ba;
  }
}
