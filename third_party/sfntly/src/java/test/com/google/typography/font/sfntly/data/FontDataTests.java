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
 */
public class FontDataTests extends TestCase {

  private static final int[] BYTE_ARRAY_SIZES =
      new int[] {1, 7, 127, 128, 129, 255, 256, 257, 666, 1023, 0x10000};

  // array data for searching
  private static final int[] LOWER_BYTE_ARRAY_FOR_SEARCHING = new int[] {2, 4, 7, 13, 127};
  private static final int[] UPPER_BYTE_ARRAY_FOR_SEARCHING = new int[] {2, 5, 12, 16, 256};

  // search test result pairs - number to search for; index found at
  private static final int[][] SEARCH_TEST_PAIRS = { {0, -1},
      {1, -1},
      {2, 0},
      {3, -1},
      {4, 1},
      {5, 1},
      {6, -1},
      {12, 2},
      {13, 3},
      {17, -1},
      {126, -1},
      {127, 4},
      {256, 4},
      {257, -1},
      {0x1000, -1}};
  
  // offset and start index data for searching data
  // array data size, lowerStartIndex, lowerOffset, upperStartIndex, upperOffset
  private static final int[][] SEARCH_TEST_OFFSETS = {
  // lower[], upper[]
      {
          (LOWER_BYTE_ARRAY_FOR_SEARCHING.length + UPPER_BYTE_ARRAY_FOR_SEARCHING.length)
              * FontData.DataSize.USHORT.size(), 0, FontData.DataSize.USHORT.size(),
          LOWER_BYTE_ARRAY_FOR_SEARCHING.length * FontData.DataSize.USHORT.size(),
          FontData.DataSize.USHORT.size()},
      // {lower, upper} []
      {
          (LOWER_BYTE_ARRAY_FOR_SEARCHING.length + UPPER_BYTE_ARRAY_FOR_SEARCHING.length)
              * FontData.DataSize.USHORT.size(), 0, 2 * FontData.DataSize.USHORT.size(),
          FontData.DataSize.USHORT.size(), 2 * FontData.DataSize.USHORT.size()},
      // upper[], lower[]
      {
          (LOWER_BYTE_ARRAY_FOR_SEARCHING.length + UPPER_BYTE_ARRAY_FOR_SEARCHING.length)
              * FontData.DataSize.USHORT.size(),
          LOWER_BYTE_ARRAY_FOR_SEARCHING.length * FontData.DataSize.USHORT.size(),
          FontData.DataSize.USHORT.size(), 0, FontData.DataSize.USHORT.size()},
      // {upper, lower} []
      {
          (LOWER_BYTE_ARRAY_FOR_SEARCHING.length + UPPER_BYTE_ARRAY_FOR_SEARCHING.length)
              * FontData.DataSize.USHORT.size(), FontData.DataSize.USHORT.size(),
          2 * FontData.DataSize.USHORT.size(), 0, 2 * FontData.DataSize.USHORT.size()},};

  // for sizing the slicing increments on the buffer used in read and write tests 
  private static final int SLICING_READWRITE_TEST_BUFFER_TRIM_FRACTION_DENOMINATOR = 21;
  // for sizing the buffer used in buffered read and write tests as fractions of the original
  private static final int TEST_READWRITE_BUFFER_INCREMENT_FRACTION_OF_ORIGINAL = 11;
    
  public void testReadableFontData() throws Exception {
    for (int size : BYTE_ARRAY_SIZES) {
      ReadableFontData rfd = fillTestWFD(WritableFontData.createWritableFontData(size), size);
      slicingReadTest(rfd);
    }
  }

  public void testReadableFontDataSearching() throws Exception {
    for (int[] arraySetupOffsets : SEARCH_TEST_OFFSETS) {
      WritableFontData wfd = WritableFontData.createWritableFontData(arraySetupOffsets[0]);
      fillTestFontDataWithShortsForSearching(wfd,
          LOWER_BYTE_ARRAY_FOR_SEARCHING,
          arraySetupOffsets[1],
          arraySetupOffsets[2],
          UPPER_BYTE_ARRAY_FOR_SEARCHING,
          arraySetupOffsets[3],
          arraySetupOffsets[4]);
      for (int[] testCase : SEARCH_TEST_PAIRS) {
        int found = wfd.searchUShort(arraySetupOffsets[1],
            arraySetupOffsets[2],
            arraySetupOffsets[3],
            arraySetupOffsets[4],
            LOWER_BYTE_ARRAY_FOR_SEARCHING.length,
            testCase[0]);
        assertEquals(testCase[1], found);
      }
    } 
  }

  public void testWritableFontData() throws Exception {
    // test with fixed byte array
    for (int size : BYTE_ARRAY_SIZES) {
      WritableFontData wfd = WritableFontData.createWritableFontData(size);
      fillTestWFD(wfd, size);
      slicingReadTest(wfd);
      slicingWriteTest(wfd, WritableFontData.createWritableFontData(size));
    }
    
    // test with growable byte array
    for (int size : BYTE_ARRAY_SIZES) {
      WritableFontData wfd = WritableFontData.createWritableFontData(0);
      fillTestWFD(wfd, size);
      slicingReadTest(wfd);
      slicingWriteTest(wfd, WritableFontData.createWritableFontData(0));
    }
  }

  private void slicingReadTest(ReadableFontData rfd) throws Exception {
    for (int trim = 0; trim < (rfd.length() / 2) + 1;
        trim += (rfd.length() / SLICING_READWRITE_TEST_BUFFER_TRIM_FRACTION_DENOMINATOR) + 1) {
      // System.out.println("\tread - trim = " + trim);
      int length = rfd.length() - 2 * trim;
      ReadableFontData slice = rfd.slice(trim, length);
      readComparison(trim, length, rfd, slice);
    }
  }

  private void slicingWriteTest(ReadableFontData rfd, WritableFontData wfd) throws Exception {
    for (int trim = 0; trim < (rfd.length() / 2) + 1;
        trim += (rfd.length() / SLICING_READWRITE_TEST_BUFFER_TRIM_FRACTION_DENOMINATOR) + 1) {
      // System.out.println("\twrite - trim = " + trim);
      int length = rfd.length() - 2 * trim;
      WritableFontData slice = null;

      // single byte writes
      slice = wfd.slice(trim, length);
      writeFontDataWithSingleByte(rfd.slice(trim, length), slice);
      readComparison(trim, length, rfd, slice);

      // buffer writes
      int increments = Math.max(length / TEST_READWRITE_BUFFER_INCREMENT_FRACTION_OF_ORIGINAL, 1);
      for (int bufferSize = 1; bufferSize < length; bufferSize += increments) {
        slice = wfd.slice(trim, length);
        writeFontDataWithBuffer(rfd.slice(trim, length), slice, bufferSize);
        readComparison(trim, length, rfd, slice);
      }

      // sliding window writes
      for (int windowSize = 1; windowSize < length; windowSize += increments) {
        slice = wfd.slice(trim, length);
        writeFontDataWithSlidingWindow(rfd.slice(trim, length), slice, windowSize);
        readComparison(trim, length, rfd, slice);
      }
    }
  }
  
  private void readComparison(int offset, int length, ReadableFontData rfd1, ReadableFontData rfd2)
      throws Exception {
    byte[] b1;
    byte[] b2;

    assertEquals(length, rfd2.length());

    // single byte reads
    b1 = readFontDataWithSingleByte(rfd1);
    b2 = readFontDataWithSingleByte(rfd2);
    assertTrue(TestUtils.equals(b1, offset, b2, 0, length));

    // buffer reads
    int increments = Math.max(length / TEST_READWRITE_BUFFER_INCREMENT_FRACTION_OF_ORIGINAL, 1);
    for (int bufferSize = 1; bufferSize <= length; bufferSize += increments) {
      b1 = readFontDataWithBuffer(rfd1, bufferSize);
      b2 = readFontDataWithBuffer(rfd2, bufferSize);

      assertTrue(TestUtils.equals(b1, offset, b2, 0, length));
    }

    // sliding window reads
    for (int windowSize = 1; windowSize <= length; windowSize += increments) {
      b1 = readFontDataWithSlidingWindow(rfd1, windowSize);
      b2 = readFontDataWithSlidingWindow(rfd2, windowSize);

      assertTrue(TestUtils.equals(b1, offset, b2, 0, length));
    }
  }

  private static byte[] readFontDataWithBuffer(ReadableFontData rfd, int bufferSize) {
    byte[] buffer = new byte[bufferSize];
    byte[] b = new byte[rfd.length()];

    int index = 0;
    while (index < rfd.length()) {
      int bytesRead = rfd.readBytes(index, buffer, 0, buffer.length);
      System.arraycopy(buffer, 0, b, index, bytesRead);
      index += bytesRead;
    }
    return b;
  }

  private static byte[] readFontDataWithSlidingWindow(ReadableFontData rfd, int windowSize) {
    byte[] b = new byte[rfd.length()];

    int index = 0;
    while (index < rfd.length()) {
      windowSize = Math.min(windowSize, b.length - index);
      int bytesRead = rfd.readBytes(index, b, index, windowSize);
      index += bytesRead;
    }
    return b;
  }

  private static byte[] readFontDataWithSingleByte(ReadableFontData rfd) {
    byte[] b = new byte[rfd.length()];

    for (int index = 0; index < rfd.length(); index++) {
      b[index] = (byte) rfd.readByte(index);
    }
    return b;
  }

  private static void writeFontDataWithBuffer(
      ReadableFontData rfd, WritableFontData wfd, int bufferSize) {
    byte[] buffer = new byte[bufferSize];

    int index = 0;
    while (index < rfd.length()) {
      int bytesRead = rfd.readBytes(index, buffer, 0, buffer.length);
      wfd.writeBytes(index, buffer, 0, buffer.length);
      index += bytesRead;
    }
  }

  private static void writeFontDataWithSlidingWindow(
      ReadableFontData rfd, WritableFontData wfd, int windowSize) {
    byte[] b = new byte[rfd.length()];

    int index = 0;
    while (index < rfd.length()) {
      windowSize = Math.min(windowSize, b.length - index);
      int bytesRead = rfd.readBytes(index, b, index, windowSize);
      wfd.writeBytes(index, b, index, bytesRead);
      index += bytesRead;
    }
  }

  private static void writeFontDataWithSingleByte(ReadableFontData rfd, WritableFontData wfd) {
    for (int index = 0; index < rfd.length(); index++) {
      byte b = (byte) rfd.readByte(index);
      wfd.writeByte(index, b);
    }
  }

  private static WritableFontData fillTestWFD(WritableFontData wfd, int size) {
    for (int i = 0; i < size; i++) {
      wfd.writeByte(i, (byte) (i % 256));
    }
    return wfd;
  }
  
  private static ReadableFontData fillTestFontDataWithShortsForSearching(WritableFontData wfd,
      int[] lowerData,
      int lowerStartIndex,
      int lowerOffset,
      int[] upperData,
      int upperStartIndex,
      int upperOffset) {
    
    // lower data
    int offset = lowerStartIndex;
    for (int d : lowerData) {
      wfd.writeUShort(offset, d);
      offset += lowerOffset;
    }

    // upper data
    offset = upperStartIndex;
    for (int d : upperData) {
      wfd.writeUShort(offset, d);
      offset += upperOffset;
    }
    
   return wfd;
  }
}
