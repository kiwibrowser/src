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

package com.google.typography.font.tools.conversion.eot;

import junit.framework.TestCase;

/**
 * @author Raph Levien
 */
public class BitIOWriterTest extends TestCase {
  // Note: if we use Guava, use MoreAsserts.assertEquals instead
  private void assertEqualsByteArray(byte[] expected, byte[] actual) {
    assertEquals(expected.length, actual.length);
    for (int i = 0; i < expected.length; i++) {
      assertEquals(expected[i], actual[i]);
    }
  }
  
  public void testBitIOBit() {
    BitIOWriter writer = new BitIOWriter();
    writer.writeBit(1);
    writer.writeBit(0);
    writer.writeBit(1);
    writer.writeBit(1);
    writer.writeBit(0);
    writer.writeBit(0);
    writer.writeBit(1);
    writer.writeBit(0);
    writer.writeBit(1);
    writer.flush();
    byte[] result = writer.toByteArray();
    byte[] expected = {(byte)0xb2, (byte)0x80};
    assertEqualsByteArray(expected, result);
  }

  public void testBitIOValues() {
    BitIOWriter writer = new BitIOWriter();
    writer.writeValue(0x55, 7);
    writer.writeValue(0x245, 10);
    writer.flush();
    byte[] result = writer.toByteArray();
    byte[] expected = {(byte)0xab, (byte)0x22, (byte)0x80};
    assertEqualsByteArray(expected, result);
  }

  public void testBitIOBool() {
    BitIOWriter writer = new BitIOWriter();
    writer.writeBit(true);
    writer.writeBit(false);
    writer.writeBit(true);
    writer.writeBit(true);
    writer.writeBit(false);
    writer.writeBit(false);
    writer.writeBit(true);
    writer.writeBit(false);
    writer.writeBit(true);
    writer.flush();
    byte[] result = writer.toByteArray();
    byte[] expected = {(byte)0xb2, (byte)0x80};
    assertEqualsByteArray(expected, result);
  }
}
