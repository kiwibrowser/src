// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf.micro;

import java.io.UnsupportedEncodingException;

/**
 * Immutable array of bytes.
 *
 * @author crazybob@google.com Bob Lee
 * @author kenton@google.com Kenton Varda
 */
public final class ByteStringMicro {
  private final byte[] bytes;

  private ByteStringMicro(final byte[] bytes) {
    this.bytes = bytes;
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
   * Returns {@code true} if the size is {@code 0}, {@code false} otherwise.
   */
  public boolean isEmpty() {
    return bytes.length == 0;
  }

  // =================================================================
  // byte[] -> ByteStringMicro

  /**
   * Empty ByteStringMicro.
   */
  public static final ByteStringMicro EMPTY = new ByteStringMicro(new byte[0]);

  /**
   * Copies the given bytes into a {@code ByteStringMicro}.
   */
  public static ByteStringMicro copyFrom(final byte[] bytes, final int offset,
                                    final int size) {
    final byte[] copy = new byte[size];
    System.arraycopy(bytes, offset, copy, 0, size);
    return new ByteStringMicro(copy);
  }

  /**
   * Copies the given bytes into a {@code ByteStringMicro}.
   */
  public static ByteStringMicro copyFrom(final byte[] bytes) {
    return copyFrom(bytes, 0, bytes.length);
  }

  /**
   * Encodes {@code text} into a sequence of bytes using the named charset
   * and returns the result as a {@code ByteStringMicro}.
   */
  public static ByteStringMicro copyFrom(final String text, final String charsetName)
      throws UnsupportedEncodingException {
    return new ByteStringMicro(text.getBytes(charsetName));
  }

  /**
   * Encodes {@code text} into a sequence of UTF-8 bytes and returns the
   * result as a {@code ByteStringMicro}.
   */
  public static ByteStringMicro copyFromUtf8(final String text) {
    try {
      return new ByteStringMicro(text.getBytes("UTF-8"));
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported?");
    }
  }

  // =================================================================
  // ByteStringMicro -> byte[]

  /**
   * Copies bytes into a buffer at the given offset.
   *
   * @param target buffer to copy into
   * @param offset in the target buffer
   */
  public void copyTo(final byte[] target, final int offset) {
    System.arraycopy(bytes, 0, target, offset, bytes.length);
  }

  /**
   * Copies bytes into a buffer.
   *
   * @param target buffer to copy into
   * @param sourceOffset offset within these bytes
   * @param targetOffset offset within the target buffer
   * @param size number of bytes to copy
   */
  public void copyTo(final byte[] target, final int sourceOffset,
                     final int targetOffset,
      final int size) {
    System.arraycopy(bytes, sourceOffset, target, targetOffset, size);
  }

  /**
   * Copies bytes to a {@code byte[]}.
   */
  public byte[] toByteArray() {
    final int size = bytes.length;
    final byte[] copy = new byte[size];
    System.arraycopy(bytes, 0, copy, 0, size);
    return copy;
  }

  /**
   * Constructs a new {@code String} by decoding the bytes using the
   * specified charset.
   */
  public String toString(final String charsetName)
      throws UnsupportedEncodingException {
    return new String(bytes, charsetName);
  }

  /**
   * Constructs a new {@code String} by decoding the bytes as UTF-8.
   */
  public String toStringUtf8() {
    try {
      return new String(bytes, "UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported?");
    }
  }

  // =================================================================
  // equals() and hashCode()

  //@Override for compatibility with Java 1.3 code we can't use annotations
  public boolean equals(final Object o) {
    if (o == this) {
      return true;
    }

    if (!(o instanceof ByteStringMicro)) {
      return false;
    }

    final ByteStringMicro other = (ByteStringMicro) o;
    final int size = bytes.length;
    if (size != other.bytes.length) {
      return false;
    }

    final byte[] thisBytes = bytes;
    final byte[] otherBytes = other.bytes;
    for (int i = 0; i < size; i++) {
      if (thisBytes[i] != otherBytes[i]) {
        return false;
      }
    }

    return true;
  }

  private volatile int hash = 0;

  //@Override for compatibility with Java 1.3 code we can't use annotations
  public int hashCode() {
    int h = hash;

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
}
