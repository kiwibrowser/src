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

package com.google.typography.font.sfntly.testutils;

import com.google.typography.font.sfntly.data.ReadableFontData;

import com.ibm.icu.charset.CharsetICU;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CoderResult;

/**
 * @author Stuart Gill
 */
public class TestUtils {
  private TestUtils() {}

  /**
   * Compare sections of two byte arrays for equality.
   * @param b1 byte array 1
   * @param offset1 offset for comparison in byte array 1
   * @param b2 byte array 2
   * @param offset2 offset for comparison in byte array 2
   * @param length the length of the byte arrays to compare
   * @return true if the array segments are equal; false otherwise
   */
  public static boolean equals(byte[] b1, int offset1, byte[] b2, int offset2, int length) {
    for (int i = 0; i < length; i++) {
      if (b1[i + offset1] != b2[i + offset2]) {
        return false;
      }
    }
    return true;
  }

  public static FileChannel createFileChannelForWriting(File file) throws IOException {
    createNewFile(file);

    RandomAccessFile raf = new RandomAccessFile(file, "rw");
    return raf.getChannel();
  }

  /**
   * Creates a new file including deleting an already existing file with the same path 
   * and name and creating any needed directories.
   * 
   * @param file the file to create
   * @throws IOException
   */
  public static void createNewFile(File file) throws IOException {
    if (file.exists()) {
      file.delete();
    }
    file.getParentFile().mkdirs();
    file.createNewFile();
  }

  public static OutputStream createOutputStream(File file) throws IOException {
    createNewFile(file);
    return new FileOutputStream(file);
  }

  /**
   * Converts an integer into a 4 character string using the ASCII encoding.
   * @param i the value to convert
   * @return the String based on the number
   */
  public static String dumpLongAsString(int i) {
    byte[] b = new byte[] {
        (byte) (i >> 24 & 0xff), 
        (byte) (i >> 16 & 0xff), 
        (byte) (i >> 8 & 0xff), 
        (byte) (i & 0xff)};

    String s;
    try {
      s = new String(b, "US-ASCII");
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("Guaranteed encoding US-ASCII missing.");
    }
    return s;
  }

  /**
   * Calculate an OpenType checksum from the array.
   * @param b the array to calculate checksum on
   * @param offset the starting index in the array
   * @param length the number of bytes to check; <b>must</b> be a multiple of 4
   * @return checksum
   */
  public static long checkSum(byte[] b, int offset, int length) {
    long checkSum = 0;

    for (int i = offset; i < length; i+=4) {
      for (int j = 0; j < 4; j++) {
        if (j + i < length) {
          checkSum += (b[j+i] & 0xff) << (24 - 8*j);
        }
      }
    }
    return checkSum & 0xffffffffL;
  }

  /**
   * Encode a single character in UTF-16.
   * @param encoder the encoder to use for the encoding
   * @param uchar the Unicode character to encode
   * @return the encoded character
   */
  public static int encodeOneChar(CharsetEncoder encoder, int uchar) {
    ByteBuffer bb = ByteBuffer.allocate(10);
    CharBuffer cb = CharBuffer.wrap(new char[] {(char) uchar});
    CoderResult result = encoder.encode(cb, bb, true);
    if (result.isError()) {
      return 0;
    }
    if (bb.position() > 4) {
      return 0;
    }
    int encChar = 0;
    for (int position = 0; position < bb.position(); position++) {
      encChar <<= 8;
      encChar |= (bb.get(position) & 0xff);
    }
    return encChar;
  }

  /**
   * Get an encoder for the charset name.
   * If the name is null or the empty string then just return null.
   * @param charsetName the charset to get an encoder for
   * @return an encoder or null if no encoder available for charset name
   */
  public static CharsetEncoder getEncoder(String charsetName) {
    if (charsetName == null || charsetName.equals("")) {
      return null;
    }
    Charset cs = CharsetICU.forNameICU(charsetName);
    return cs.newEncoder();
  }

  private static final char EXTENSION_SEPARATOR = '.';

  public static String extension(File file) {
    String ext = file.getName();
    int extPosition = ext.lastIndexOf(EXTENSION_SEPARATOR);
    if (extPosition == -1) {
      return "";
    }
    return ext.substring(extPosition);
  }

  /**
   * Read a file fully into a new byte array.
   * @param file the file to read
   * @return the byte array
   * @throws IOException
   */
  public static byte[] readFile(File file) throws IOException {
    int length = (int) file.length();
    byte[] b = new byte[length];

    FileInputStream fis = null;
    try {
      fis = new FileInputStream(file);
      while (length > 0) {
        length -= fis.read(b, b.length - length, length);
      }
      return b;
    } finally {
      fis.close();
    }
  }

  /**
   * @param offset1 offset to start comparing the first ReadableFontData from
   * @param rfd1 the first ReadableFontData
   * @param offset2 offset to start comparing the second ReadableFontData from
   * @param rfd2 the second ReadableFontData
   * @param length the number of bytes to compare
   * @return true if all bytes in the ranges given are equal; false otherwise
   */
  public static boolean equals(
      int offset1, ReadableFontData rfd1, int offset2, ReadableFontData rfd2, int length) {
    for (int i = 0; i < length; i++) {
      int b1 = rfd1.readByte(i + offset1);
      int b2 = rfd2.readByte(i + offset2);
      if (b1 != b2) {
        return false;
      }
    }
    return true;
  }

  /**
   * Checks that both objects are equal as defined by the object itself. If one
   * is null then they are not equal. If both are null they are considered
   * equal.
   * 
   * @param o1 first object
   * @param o2 second object
   * @return true if equal
   */
  public static boolean equalsNullOk(Object o1, Object o2) {
    if (o1 == o2) {
      return true;
    }
    if (o1 == null || o2 == null) {
      // both can't be null - caught above
      return false;
    }
    return o1.equals(o2);
  }
}
