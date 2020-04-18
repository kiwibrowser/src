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
import java.io.InputStream;

/**
 * Writable font data wrapper. Supports writing of data primitives in the
 * TrueType / OpenType spec.
 *
 * @author Stuart Gill
 */
public final class WritableFontData extends ReadableFontData {

  /**
   * Constructs a writable font data object. If the length is specified as
   * positive then a fixed size font data object will be created. If the length
   * is zero or less then a growable font data object will be created and the
   * size will be used as an estimate to help in allocating the original space.
   *
   * @param length if length > 0 create a fixed length font data; otherwise
   *        create a growable font data
   * @return a new writable font data
   */
  public static final WritableFontData createWritableFontData(int length) {
    ByteArray<?> ba = null;
    if (length > 0) {
      ba = new MemoryByteArray(length);
      ba.setFilledLength(length);
    } else {
      ba = new GrowableMemoryByteArray();
    }
    WritableFontData wfd = new WritableFontData(ba);
    return wfd;
  }

  /**
   * Constructs a writable font data object. The new font data object will wrap
   * the bytes passed in to the factory and it will take ownership of those
   * bytes. They should not be used again by the caller.
   *
   * @param b the byte array to wrap
   * @return a new writable font data
   */
  public static final WritableFontData createWritableFontData(byte[] b) {
    ByteArray<?> ba = new MemoryByteArray(b);
    WritableFontData wfd = new WritableFontData(ba);
    return wfd;
  }
  
  /**
   * Constructs a writable font data object. The new font data object will wrap
   * a copy of the the data used by the original writable font data object passed in.
   *
   * @param original the source font data
   * @return a new writable font data
   */
  public static final WritableFontData createWritableFontData(ReadableFontData original) {
    ByteArray<?> ba = null;
    // TODO(stuartg): push this down into the BAs - maybe remove the difference between growable and fixed
    if (original.array.growable()) {
      ba = new GrowableMemoryByteArray();
    } else {
      ba = new MemoryByteArray(original.array.length());
    }
    original.array.copyTo(ba);
    
    WritableFontData wfd = new WritableFontData(ba);
    wfd.setCheckSumRanges(original.checkSumRange());
    return wfd;
  }
  
  /**
   * Constructor.
   *
   * @param array byte array to wrap
   */
  private WritableFontData(ByteArray<? extends ByteArray<?>> array) {
    super(array);
  }

  /**
   * Constructor with a lower bound.
   *
   * @param data other WritableFontData object to share data with
   * @param offset offset from the other WritableFontData's data
   */
  private WritableFontData(WritableFontData data, int offset) {
    super(data, offset);
  }

  /**
   * Constructor with lower bound and a length bound.
   *
   * @param data other WritableFontData object to share data with
   * @param offset offset from the other WritableFontData's data
   * @param length length of other WritableFontData's data to use
   */
  private WritableFontData(WritableFontData data, int offset, int length) {
    super(data, offset, length);
  }

  /**
   * Makes a slice of this FontData. The returned slice will share the data with
   * the original <code>FontData</code>.
   *
   * @param offset the start of the slice
   * @param length the number of bytes in the slice
   * @return a slice of the original FontData
   */
  @Override
  public WritableFontData slice(int offset, int length) {
    if (offset < 0 || length < 0 || offset > Integer.MAX_VALUE - length ||
        (offset + length) > this.size()) {
      throw new IndexOutOfBoundsException("Attempt to bind data outside of its limits.");
    }
    WritableFontData slice = new WritableFontData(this, offset, length);
    return slice;
  }

  /**
   * Makes a bottom bound only slice of this array. The returned slice will
   * share the data with the original <code>FontData</code>.
   *
   * @param offset the start of the slice
   * @return a slice of the original FontData
   */
  @Override
  public WritableFontData slice(int offset) {
    if (offset < 0 || offset > this.size()) {
      throw new IndexOutOfBoundsException("Attempt to bind data outside of its limits.");
    }
    WritableFontData slice = new WritableFontData(this, offset);
    return slice;
  }

  /**
   * Writes a byte at the given index.
   *
   * @param index index into the font data
   * @param b the byte to write
   * @return the number of bytes written
   */
  public int writeByte(int index, byte b) {
    this.array.put(this.boundOffset(index), b);
    return 1;
  }

  /**
   * Writes the bytes from the array.
   *
   * @param index index into the font data
   * @param b the source for the bytes to be written
   * @param offset offset in the byte array
   * @param length the length of the bytes to be written
   * @return the number of bytes actually written; -1 if the index is outside
   *         the FontData's range
   */
  public int writeBytes(int index, byte[] b, int offset, int length) {
    return this.array.put(this.boundOffset(index), b, offset, this.boundLength(index, length));
  }

  /**
   * Writes the bytes from the array and pad if necessary.
   *
   *  Writes to the length given using the byte array provided and if there are
   * not enough bytes in the array then pad to the requested length using the
   * pad byte specified.
   *
   * @param index index into the font data
   * @param b the source for the bytes to be written
   * @param offset offset in the byte array
   * @param length the length of the bytes to be written
   * @param pad the padding byte to be used if necessary
   * @return the number of bytes actually written
   */
  public int writeBytesPad(int index, byte[] b, int offset, int length, byte pad) {
    int written = this.array.put(this.boundOffset(index), b, offset,
        this.boundLength(index, Math.min(length, b.length - offset)));
    written += this.writePadding(written + index, length - written, pad);
    return written;
  }

  /**
   * Writes padding to the FontData. The padding byte written is 0x00.
   *
   * @param index index into the font data
   * @param count the number of pad bytes to write
   * @return the number of pad bytes written
   */
  public int writePadding(int index, int count) {
    return this.writePadding(index, count, (byte) 0x00);
  }

  /**
   * Writes padding to the FontData.
   *
   * @param index index into the font data
   * @param count the number of pad bytes to write
   * @param pad the byte value to use as padding
   * @return the number of pad bytes written
   */
  public int writePadding(int index, int count, byte pad) {
    for (int i = 0; i < count; i++) {
      this.array.put(index + i, pad);
    }
    return count;
  }

  /**
   * Writes the bytes from the array.
   *
   * @param index index into the font data
   * @param b the source for the bytes to be written
   * @return the number of bytes actually written; -1 if the index is outside
   *         the FontData's range
   */
  public int writeBytes(int index, byte[] b) {
    return this.writeBytes(index, b, 0, b.length);
  }

  /**
   * Writes the CHAR at the given index.
   *
   * @param index index into the font data
   * @param c the CHAR
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeChar(int index, byte c) {
    return this.writeByte(index, c);
  }

  /**
   * Writes the USHORT at the given index.
   *
   * @param index index into the font data
   * @param us the USHORT
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeUShort(int index, int us) {
    this.writeByte(index, (byte) ((us >> 8) & 0xff));
    this.writeByte(index + 1, (byte) (us & 0xff));
    return 2;
  }

  /**
   * Writes the USHORT at the given index in little endian format.
   *
   * @param index index into the font data
   * @param us the USHORT
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeUShortLE(int index, int us) {
    this.array.put(index, (byte) (us & 0xff));
    this.array.put(index + 1, (byte) ((us >> 8) & 0xff));
    return 2;
  }

  /**
   * Writes the SHORT at the given index.
   *
   * @param index index into the font data
   * @param s the SHORT
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeShort(int index, int s) {
    return this.writeUShort(index, s);
  }

  /**
   * Writes the UINT24 at the given index.
   *
   * @param index index into the font data
   * @param ui the UINT24
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeUInt24(int index, int ui) {
    this.writeByte(index, (byte) ((ui >> 16) & 0xff));
    this.writeByte(index + 1, (byte) ((ui >> 8) & 0xff));
    this.writeByte(index + 2, (byte) (ui & 0xff));
    return 3;
  }

  /**
   * Writes the ULONG at the given index.
   *
   * @param index index into the font data
   * @param ul the ULONG
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeULong(int index, long ul) {
    this.writeByte(index, (byte) ((ul >> 24) & 0xff));
    this.writeByte(index + 1, (byte) ((ul >> 16) & 0xff));
    this.writeByte(index + 2, (byte) ((ul >> 8) & 0xff));
    this.writeByte(index + 3, (byte) (ul & 0xff));
    return 4;
  }

  /**
   * Writes the ULONG at the given index in little endian format.
   *
   * @param index index into the font data
   * @param ul the ULONG
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeULongLE(int index, long ul) {
    this.array.put(index, (byte) (ul & 0xff));
    this.array.put(index + 1, (byte) ((ul >> 8) & 0xff));
    this.array.put(index + 2, (byte) ((ul >> 16) & 0xff));
    this.array.put(index + 3, (byte) ((ul >> 24) & 0xff));
    return 4;
  }

  /**
   * Writes the LONG at the given index.
   *
   * @param index index into the font data
   * @param l the LONG
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeLong(int index, long l) {
    return this.writeULong(index, l);
  }

  /**
   * Writes the Fixed at the given index.
   *
   * @param index index into the font data
   * @param f the Fixed
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeFixed(int index, int f) {
    return this.writeLong(index, f);
  }

  /**
   * Writes the LONGDATETIME at the given index.
   *
   * @param index index into the font data
   * @param date the LONGDATETIME
   * @return the number of bytes actually written
   * @throws IndexOutOfBoundsException if index is outside the FontData's range
   */
  public int writeDateTime(int index, long date) {
    this.writeULong(index, (date >> 32) & 0xffffffff);
    this.writeULong(index + 4, date & 0xffffffff);
    return 8;
  }

  /**
   * Copy from the InputStream into this FontData.
   *
   * @param is the source
   * @param length the number of bytes to copy
   * @throws IOException
   */
  public void copyFrom(InputStream is, int length) throws IOException {
    this.array.copyFrom(is, length);
  }

  /**
   * Copy everything from the InputStream into this FontData.
   *
   * @param is the source
   * @throws IOException
   */
  public void copyFrom(InputStream is) throws IOException {
    this.array.copyFrom(is);
  }
}
