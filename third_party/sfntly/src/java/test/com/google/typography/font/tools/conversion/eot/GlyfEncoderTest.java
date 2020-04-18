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

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * @author Raph Levien
 */
public class GlyfEncoderTest extends TestCase {
  // Note: if we use Guava, use MoreAsserts.assertEquals instead
  private void assertEqualsByteArray(byte[] expected, byte[] actual) {
    assertEquals(expected.length, actual.length);
    for (int i = 0; i < expected.length; i++) {
      assertEquals(expected[i], actual[i]);
    }
  }
  
  public void test255UShort1() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255UShort(os, 142);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)142};
    assertEqualsByteArray(expected, actual);
  }

  public void test255UShort2() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255UShort(os, 254);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)255, (byte)1};
    assertEqualsByteArray(expected, actual);
  }

  public void test255UShort3() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255UShort(os, 507);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)254, (byte)1};
    assertEqualsByteArray(expected, actual);
  }

  public void test255UShort4() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255UShort(os, 0x1234);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)253, (byte)0x12, (byte)0x34};
    assertEqualsByteArray(expected, actual);
  }

  public void test255Short1() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255Short(os, 142);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)142};
    assertEqualsByteArray(expected, actual);
  }

  public void test255Short2() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255Short(os, -142);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte) 250, (byte)142};
    assertEqualsByteArray(expected, actual);
  }

  public void test255Short3() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255Short(os, 251);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte) 255, (byte)1};
    assertEqualsByteArray(expected, actual);
  }

  public void test255Short4() throws IOException {
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    GlyfEncoder.write255Short(os, 0x1234);
    byte[] actual = os.toByteArray();
    byte[] expected = {(byte)253, (byte)0x12, (byte)0x34};
    assertEqualsByteArray(expected, actual);
  }
  
  private byte[] tripletEncode(boolean onCurve, int x, int y) throws IOException {
    GlyfEncoder e = new GlyfEncoder();
    ByteArrayOutputStream os = new ByteArrayOutputStream();
    e.writeTriplet(os, onCurve, x, y);
    
    byte[] flagBytes = e.getGlyfBytes();
    assertEquals(1, flagBytes.length);
    byte[] valueBytes = os.toByteArray();
    byte[] result = new byte[flagBytes.length + valueBytes.length];
    System.arraycopy(flagBytes, 0, result, 0, flagBytes.length);
    System.arraycopy(valueBytes, 0, result, flagBytes.length, valueBytes.length);
    return result;
  }
  
  public void testTriplet1() throws IOException {
    byte[] expected = {(byte)1, (byte)1};
    assertEqualsByteArray(expected, tripletEncode(true, 0, 1));
  }

  public void testTriplet2() throws IOException {
    byte[] expected = {(byte)2, (byte)1};
    assertEqualsByteArray(expected, tripletEncode(true, 0, -257));
  }

  public void testTriplet11() throws IOException {
    byte[] expected = {(byte)11, (byte)1};
    assertEqualsByteArray(expected, tripletEncode(true, 1, 0));
  }

  public void testTriplet15() throws IOException {
    byte[] expected = {(byte)15, (byte)1};
    assertEqualsByteArray(expected, tripletEncode(true, 513, 0));
  }

  public void testTriplet21() throws IOException {
    byte[] expected = {(byte)21, (byte)0x12};
    assertEqualsByteArray(expected, tripletEncode(true, 2, -3));
  }

  public void testTriplet56() throws IOException {
    byte[] expected = {(byte)56, (byte)0x12};
    assertEqualsByteArray(expected, tripletEncode(true, -34, -19));
  }

  public void testTriplet87() throws IOException {
    byte[] expected = {(byte)87, (byte)128, (byte)130};
    assertEqualsByteArray(expected, tripletEncode(true, 129, 131));
  }

  public void testTriplet105() throws IOException {
    byte[] expected = {(byte)105, (byte)200, (byte)100};
    assertEqualsByteArray(expected, tripletEncode(true, 457, -613));
  }

  public void testTriplet121() throws IOException {
    byte[] expected = {(byte)121, (byte)0x12, (byte)0x34, (byte)0x56};
    assertEqualsByteArray(expected, tripletEncode(true, 0x123, -0x456));
  }

  public void testTriplet126() throws IOException {
    byte[] expected = {(byte)126, (byte)0x12, (byte)0x34, (byte)0x56, (byte)0x78};
    assertEqualsByteArray(expected, tripletEncode(true, -0x1234, 0x5678));
  }

  public void testTriplet129() throws IOException {
    byte[] expected = {(byte)129, (byte)1};
    assertEqualsByteArray(expected, tripletEncode(false, 0, 1));
  }

  public void testTriplet254() throws IOException {
    byte[] expected = {(byte)254, (byte)0x12, (byte)0x34, (byte)0x56, (byte)0x78};
    assertEqualsByteArray(expected, tripletEncode(false, -0x1234, 0x5678));
  }
}
