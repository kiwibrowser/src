/*
 * Copyright 2011 Google Inc.
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
package com.google.ipc.invalidation.util;

import com.google.ipc.invalidation.util.LazyString.LazyStringReceiver;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Locale;


/**
 * A class that encapsulates a (fixed size) sequence of bytes and provides a
 * equality (along with hashcode) method that considers two sequences to be
 * equal if they have the same contents. Borrowed from protobuf's ByteString
 *
 */
public class Bytes extends InternalBase implements Comparable<Bytes> {

  public static final Bytes EMPTY_BYTES = new Bytes(new byte[0]);
  private static final Charset UTF_8 = Charset.forName("UTF-8");

  /**
   * Interface accessing byte elements from {@code T}, which may be (for instance)
   * {@link com.google.protobuf.ByteString ByteString} or {@code byte[]}.
   */
  interface BytesAccessor<T> {
    int size(T bytes);
    byte get(T bytes, int index);
  }

  private static final BytesAccessor<byte[]> BYTE_ARRAY_ACCESSOR = new BytesAccessor<byte[]>() {
    @Override public int size(byte[] bytes) {
      return bytes == null ? 0 : bytes.length;
    }

    @Override public byte get(byte[] bytes, int index) {
      return bytes[index];
    }
  };

  private static final LazyStringReceiver<byte[]> BYTE_ARRAY_RECEIVER =
      new LazyStringReceiver<byte[]>() {
    @Override public void appendToBuilder(TextBuilder builder, byte[] element) {
      toCompactString(builder, element);
    }
  };

  /**
   * Three arrays that store the representation of each character from 0 to 255.
   * The ith number's octal representation is: CHAR_OCTAL_STRINGS1[i],
   * CHAR_OCTAL_STRINGS2[i], CHAR_OCTAL_STRINGS3[i]
   * <p>
   * E.g., if the number 128, these arrays contain 2, 0, 0 at index 128. We use
   * 3 char arrays instead of an array of strings since the code path for a
   * character append operation is quite a bit shorter than the append operation
   * for strings.
   */
  private static final char[] CHAR_OCTAL_STRINGS1 = new char[256];
  private static final char[] CHAR_OCTAL_STRINGS2 = new char[256];
  private static final char[] CHAR_OCTAL_STRINGS3 = new char[256];

  /** The actual sequence. */
  private final byte[] bytes;

  /** Cached hash */
  private volatile int hash = 0;

  static {
    // Initialize the array with the Octal string values so that we do not have
    // to do String.format for every byte during runtime.
    for (int i = 0; i < CHAR_OCTAL_STRINGS1.length; i++) {
      String value = String.format(Locale.ROOT, "\\%03o", i);
      CHAR_OCTAL_STRINGS1[i] = value.charAt(1);
      CHAR_OCTAL_STRINGS2[i] = value.charAt(2);
      CHAR_OCTAL_STRINGS3[i] = value.charAt(3);
    }
  }

  public Bytes(byte[] bytes) {
    this.bytes = bytes;
  }

  /**
   * Creates a Bytes object with the contents of {@code array1} followed by the
   * contents of {@code array2}.
   */
  public Bytes(byte[] array1, byte[] array2) {
    Preconditions.checkNotNull(array1);
    Preconditions.checkNotNull(array2);
    ByteBuffer buffer = ByteBuffer.allocate(array1.length + array2.length);
    buffer.put(array1);
    buffer.put(array2);
    this.bytes = buffer.array();
  }

  /**
   * Creates a Bytes object with the contents of {@code b1} followed by the
   * contents of {@code b2}.
   */
  public Bytes(Bytes b1, Bytes b2) {
    this(b1.bytes, b2.bytes);
  }

  public Bytes(byte b) {
    this.bytes = new byte[1];
    bytes[0] = b;
  }

  /** Creates a Bytes object from the given string encoded as a UTF-8 byte array. */
  public static Bytes fromUtf8Encoding(String s) {
    return new Bytes(s.getBytes(UTF_8));
  }

  /**
   * Gets the byte at the given index.
   *
   * @throws ArrayIndexOutOfBoundsException {@code index} is < 0 or >= size
   */
  public byte byteAt(final int index) {
    return bytes[index];
  }

  /**
   * Gets the number of bytes.
   */
  public int size() {
    return bytes.length;
  }

  /**
   * Returns the internal byte array.
   */
  public byte[] getByteArray() {
    return bytes;
  }

  /**
   * Returns a new {@code Bytes} containing the given subrange of bytes [{@code from}, {@code to}).
   */
  public Bytes subsequence(int from, int to) {
    // Identical semantics to Arrays.copyOfRange() but implemented manually so runs on
    // Froyo (JDK 1.5).
    int newLength = to - from;
    if (newLength < 0) {
      throw new IllegalArgumentException(from + " > " + to);
    }
    byte[] copy = new byte[newLength];
    System.arraycopy(bytes, from, copy, 0, Math.min(bytes.length - from, newLength));
    return new Bytes(copy);
  }

  @Override public boolean equals(final Object o) {
    if (o == this) {
      return true;
    }

    if (!(o instanceof Bytes)) {
      return false;
    }

    final Bytes other = (Bytes) o;
    return Arrays.equals(bytes, other.bytes);
  }

  @Override public int hashCode() {
    int h = hash;

    // If the hash has been not computed, go through each byte and compute it.
    if (h == 0) {
      final byte[] thisBytes = bytes;
      final int size = bytes.length;

      h = size;
      for (int i = 0; i < size; i++) {
        h = h * 31 + thisBytes[i];
      }
      if (h == 0) {
        h = 1;
      }

      hash = h;
    }

    return h;
  }

  /**
   * Returns whether these bytes are a prefix (either proper or improper) of
   * {@code other}.
   */
  public boolean isPrefixOf(Bytes other) {
    Preconditions.checkNotNull(other);
    if (size() > other.size()) {
      return false;
    }
    for (int i = 0; i < size(); ++i) {
      if (bytes[i] != other.bytes[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Returns whether these bytes are a suffix (either proper or improper) of
   * {@code other}.
   */
  public boolean isSuffixOf(Bytes other) {
    Preconditions.checkNotNull(other);
    int diff = other.size() - size();
    if (diff < 0) {
      return false;
    }
    for (int i = 0; i < size(); ++i) {
      if (bytes[i] != other.bytes[i + diff]) {
        return false;
      }
    }
    return true;
  }

  @Override public int compareTo(Bytes other) {
    return compare(bytes, other.bytes);
  }

  public static Bytes fromByteArray(byte[] bytes) {
    return (bytes == null) ? null : new Bytes(bytes);
  }

  /**
   * Same specs as {@link #compareTo} except for the byte[] type. Null arrays are ordered before
   * non-null arrays.
   */
  public static int compare(byte[] first, byte[] second) {
    return compare(BYTE_ARRAY_ACCESSOR, first, second);
  }

  /**
   * Performs lexicographic comparison of two byte sequences. Null sequences are ordered before
   * non-null sequences.
   */
  static <T> int compare(BytesAccessor<T> accessor, T first, T second) {
    // Order null arrays before non-null arrays.
    if (first == null) {
      return (second == null) ? 0 : -1;
    }
    if (second == null) {
      return 1;
    }

    int minLength = Math.min(accessor.size(first), accessor.size(second));
    for (int i = 0; i < minLength; i++) {

      if (accessor.get(first, i) != accessor.get(second, i)) {
        int firstByte = accessor.get(first, i) & 0xff;
        int secondByte = accessor.get(second, i) & 0xff;
        return firstByte - secondByte;
      }
    }
    // At this point, either both arrays are equal length or one of the arrays has ended.
    // * If the arrays are of equal length, they must be identical (else we would have
    //   returned the correct value above
    // * If they are not of equal length, the one with the longer length is greater.
    return accessor.size(first) - accessor.size(second);
  }

  /**
   * Renders the bytes as a string in standard bigtable ascii / octal mix compatible with bt and
   * returns it.
   */
  public static String toString(byte[] bytes) {
    return toCompactString(new TextBuilder(), bytes).toString();
  }

  /**
   * Renders the bytes as a string in standard bigtable ascii / octal mix compatible with bt and
   * adds it to builder.
   */
  @Override public void toCompactString(TextBuilder builder) {
    toCompactString(builder, bytes);
  }

  /**
   * Renders the bytes as a string in standard bigtable ascii / octal mix compatible with bt and
   * adds it to builder. Returns {@code builder}.
   */
  public static TextBuilder toCompactString(TextBuilder builder, byte[] bytes) {
    return toCompactString(BYTE_ARRAY_ACCESSOR, builder, bytes);
  }

  /**
   * Returns an object that lazily formats {@code bytes} when {@link Object#toString()} is called.
   */
  public static Object toLazyCompactString(byte[] bytes) {
    if (bytes == null || bytes.length == 0) {
      return "";
    }
    return LazyString.toLazyCompactString(bytes, BYTE_ARRAY_RECEIVER);
  }

  /**
   * Renders the bytes as a string in standard bigtable ascii / octal mix compatible with bt and
   * adds it to builder. Borrowed from Bigtable's {@code Util$keyToString()}.
   * Returns {@code builder}.
   */
  static <T> TextBuilder toCompactString(BytesAccessor<T> accessor, TextBuilder builder,
      T bytes) {
    for (int i = 0; i < accessor.size(bytes); i++) {
      byte c = accessor.get(bytes, i);
      switch(c) {
        case '\n': builder.append('\\'); builder.append('n'); break;
        case '\r': builder.append('\\'); builder.append('r'); break;
        case '\t': builder.append('\\'); builder.append('t'); break;
        case '\"': builder.append('\\'); builder.append('"'); break;
        case '\\': builder.append('\\'); builder.append('\\'); break;
        default:
          if ((c >= 32) && (c < 127) && c != '\'') {
            builder.append((char) c);
          } else {
            int byteValue = c;
            if (c < 0) {
              byteValue = c + 256;
            }
            builder.append('\\');
            builder.append(CHAR_OCTAL_STRINGS1[byteValue]);
            builder.append(CHAR_OCTAL_STRINGS2[byteValue]);
            builder.append(CHAR_OCTAL_STRINGS3[byteValue]);
          }
      }
    }
    return builder;
  }
}
