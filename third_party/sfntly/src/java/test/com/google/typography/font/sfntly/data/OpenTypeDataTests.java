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

import junit.framework.TestCase;

import java.util.Arrays;

/**
 * @author Stuart Gill
 */
public class OpenTypeDataTests extends TestCase {

  private static final byte[] testBytes = 
    new byte[] {(byte) 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

  public OpenTypeDataTests(String name) {
    super(name);
  }

  public void testRead() {
    MemoryByteArray array = new MemoryByteArray(Arrays.copyOf(testBytes, testBytes.length));
    ReadableFontData data = new ReadableFontData(array);

    assertEquals(-1, data.readByte(0));
    assertEquals(0xff, data.readUByte(0));
    assertEquals(0x01, data.readUByte(1));
    assertEquals(65281, data.readUShort(0));
    assertEquals(-255, data.readShort(0));
    assertEquals(16711937, data.readUInt24(0));
    assertEquals(4278255873L, data.readULong(0));
    assertEquals(-16711423, data.readLong(0));
  }

  public void testCopy() throws Exception {
    byte[] sourceBytes = new byte[1024];

    for (int i = 0; i < sourceBytes.length; i++) {
      sourceBytes[i] = (byte) i;
    }
    MemoryByteArray source = new MemoryByteArray(sourceBytes);

    byte[] destinationBytes = new byte[1024];
    MemoryByteArray destination = new MemoryByteArray(destinationBytes);

    int length = source.copyTo(destination);
    assertEquals(sourceBytes.length, length);
    for (int i = 0; i < sourceBytes.length; i++) {
      assertEquals(sourceBytes[i], destinationBytes[i]);
    }
  }
}
