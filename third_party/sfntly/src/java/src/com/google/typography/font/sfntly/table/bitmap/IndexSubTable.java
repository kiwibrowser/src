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

package com.google.typography.font.sfntly.table.bitmap;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;
import com.google.typography.font.sfntly.table.bitmap.EblcTable.Offset;

import java.util.Iterator;

public abstract class IndexSubTable extends SubTable {
  private static final boolean DEBUG = false;

  public static final class Format {
    public static final int FORMAT_1 = 1;
    public static final int FORMAT_2 = 2;
    public static final int FORMAT_3 = 3;
    public static final int FORMAT_4 = 4;
    public static final int FORMAT_5 = 5;
  }

  private final int firstGlyphIndex;
  private final int lastGlyphIndex;
  private final int indexFormat;
  private final int imageFormat;
  private final int imageDataOffset;

  protected static IndexSubTable createIndexSubTable(
      ReadableFontData data, int offsetToIndexSubTableArray, int arrayIndex) {

    Builder<? extends IndexSubTable> builder =
        Builder.createBuilder(data, offsetToIndexSubTableArray, arrayIndex);
    if (builder == null) {
      return null;
    }
    return builder.build();
  }

  protected IndexSubTable(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
    super(data);
    this.firstGlyphIndex = firstGlyphIndex;
    this.lastGlyphIndex = lastGlyphIndex;
    this.indexFormat = this.data.readUShort(Offset.indexSubHeader_indexFormat.offset);
    this.imageFormat = this.data.readUShort(Offset.indexSubHeader_imageFormat.offset);
    this.imageDataOffset = this.data.readULongAsInt(Offset.indexSubHeader_imageDataOffset.offset);
  }

  public int indexFormat() {
    return this.indexFormat;
  }

  public int firstGlyphIndex() {
    return this.firstGlyphIndex;
  }

  public int lastGlyphIndex() {
    return this.lastGlyphIndex;
  }

  public int imageFormat() {
    return this.imageFormat;
  }

  public int imageDataOffset() {
    return this.imageDataOffset;
  }

  public BitmapGlyphInfo glyphInfo(int glyphId) {
    int loca = this.checkGlyphRange(glyphId);
    if (loca == -1) {
      return null;
    }
    if (this.glyphStartOffset(glyphId) == -1) {
      return null;
    }

    return new BitmapGlyphInfo(glyphId, this.imageDataOffset(), this.glyphStartOffset(glyphId),
        this.glyphLength(glyphId), this.imageFormat());
  }

  public final int glyphOffset(int glyphId) {
    int glyphStartOffset = this.glyphStartOffset(glyphId);
    if (glyphStartOffset == -1) {
      return -1;
    }
    return this.imageDataOffset() + glyphStartOffset;
  }

  /**
   * Gets the offset of the glyph relative to the block for this index subtable.
   *
   * @param glyphId the glyph id
   * @return the glyph offset
   */
  public abstract int glyphStartOffset(int glyphId);

  public abstract int glyphLength(int glyphId);

  public abstract int numGlyphs();

  protected static int checkGlyphRange(int glyphId, int firstGlyphId, int lastGlyphId) {
    if (glyphId < firstGlyphId || glyphId > lastGlyphId) {
      throw new IndexOutOfBoundsException("Glyph ID is outside of the allowed range.");
    }
    return glyphId - firstGlyphId;
  }

  protected int checkGlyphRange(int glyphId) {
    return IndexSubTable.checkGlyphRange(glyphId, this.firstGlyphIndex(), this.lastGlyphIndex());
  }

  @Override
  public String toString() {
    String s = "IndexSubTable: " + "[0x" + Integer.toHexString(this.firstGlyphIndex()) + " : Ox"
        + Integer.toHexString(this.lastGlyphIndex()) + "]" + ", format = " + this.indexFormat
        + ", image format = " + this.imageFormat() + ", imageOff = "
        + Integer.toHexString(this.imageDataOffset()) + "\n";
    if (DEBUG) {
      for (int g = this.firstGlyphIndex(); g < this.lastGlyphIndex(); g++) {
        s += "\tgid = " + g + ", offset = " + this.glyphStartOffset(g) + "\n";
      }
    }
    return s;
  }

  public abstract static class Builder<T extends IndexSubTable> extends SubTable.Builder<T> {
    private int firstGlyphIndex;
    private int lastGlyphIndex;
    private int indexFormat;
    private int imageFormat;
    private int imageDataOffset;

    public static Builder<? extends IndexSubTable> createBuilder(int indexFormat) {
      switch (indexFormat) {
        case 1:
          return IndexSubTableFormat1.Builder.createBuilder();
        case 2:
          return IndexSubTableFormat2.Builder.createBuilder();
        case 3:
          return IndexSubTableFormat3.Builder.createBuilder();
        case 4:
          return IndexSubTableFormat4.Builder.createBuilder();
        case 5:
          return IndexSubTableFormat5.Builder.createBuilder();
        default:
          // unknown format and unable to process
          throw new IllegalArgumentException(String.format("Invalid Index SubTable Format %i%n",
              indexFormat));
      }
    }

    static Builder<? extends IndexSubTable> createBuilder(
        ReadableFontData data, int offsetToIndexSubTableArray, int arrayIndex) {

      int indexSubTableEntryOffset =
          offsetToIndexSubTableArray + arrayIndex * Offset.indexSubTableEntryLength.offset;

      int firstGlyphIndex = data.readUShort(
          indexSubTableEntryOffset + Offset.indexSubTableEntry_firstGlyphIndex.offset);
      int lastGlyphIndex = data.readUShort(
          indexSubTableEntryOffset + Offset.indexSubTableEntry_lastGlyphIndex.offset);
      int additionOffsetToIndexSubtable = data.readULongAsInt(indexSubTableEntryOffset
          + Offset.indexSubTableEntry_additionalOffsetToIndexSubtable.offset);

      int indexSubTableOffset = offsetToIndexSubTableArray + additionOffsetToIndexSubtable;

      int indexFormat = data.readUShort(indexSubTableOffset);
      switch (indexFormat) {
        case 1:
          return IndexSubTableFormat1.Builder.createBuilder(
              data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
        case 2:
          return IndexSubTableFormat2.Builder.createBuilder(
              data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
        case 3:
          return IndexSubTableFormat3.Builder.createBuilder(
              data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
        case 4:
          return IndexSubTableFormat4.Builder.createBuilder(
              data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
        case 5:
          return IndexSubTableFormat5.Builder.createBuilder(
              data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
        default:
          // unknown format and unable to process
          throw new IllegalArgumentException(
              String.format("Invalid Index SubTable Foramt %i%n", indexFormat));
      }
    }

    protected Builder(int dataSize, int indexFormat) {
      super(dataSize);
      this.indexFormat = indexFormat;
    }

    protected Builder(int indexFormat, int imageFormat, int imageDataOffset, int dataSize) {
      this(dataSize, indexFormat);
      this.imageFormat = imageFormat;
      this.imageDataOffset = imageDataOffset;
    }

    protected Builder(WritableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data);
      this.firstGlyphIndex = firstGlyphIndex;
      this.lastGlyphIndex = lastGlyphIndex;
      this.initialize(data);
    }

    protected Builder(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data);
      this.firstGlyphIndex = firstGlyphIndex;
      this.lastGlyphIndex = lastGlyphIndex;
      this.initialize(data);
    }

    /**
     * @param data
     */
    private void initialize(ReadableFontData data) {
      this.indexFormat = data.readUShort(Offset.indexSubHeader_indexFormat.offset);
      this.imageFormat = data.readUShort(Offset.indexSubHeader_imageFormat.offset);
      this.imageDataOffset = data.readULongAsInt(Offset.indexSubHeader_imageDataOffset.offset);
    }


    /**
     * Unable to fully revert unless some changes happen to hold the original
     * data. Until then keep as protected.
     */
    protected void revert() {
      this.setModelChanged(false);
      this.initialize(this.internalReadData());
    }

    public int indexFormat() {
      return this.indexFormat;
    }

    public int firstGlyphIndex() {
      return this.firstGlyphIndex;
    }

    public void setFirstGlyphIndex(int firstGlyphIndex) {
      this.firstGlyphIndex = firstGlyphIndex;
    }

    public int lastGlyphIndex() {
      return this.lastGlyphIndex;
    }

    public void setLastGlyphIndex(int lastGlyphIndex) {
      this.lastGlyphIndex = lastGlyphIndex;
    }

    public int imageFormat() {
      return this.imageFormat;
    }

    public void setImageFormat(int imageFormat) {
      this.imageFormat = imageFormat;
    }

    public int imageDataOffset() {
      return this.imageDataOffset;
    }

    public void setImageDataOffset(int offset) {
      this.imageDataOffset = offset;
    }

    public abstract int numGlyphs();

    /**
     * Gets the glyph info for the specified glyph id.
     *
     * @param glyphId the glyph id to look up
     * @return the glyph info
     */
    // TODO(stuartg): could be optimized by pushing down into subclasses
    public BitmapGlyphInfo glyphInfo(int glyphId) {
      return new BitmapGlyphInfo(glyphId, this.imageDataOffset(), this.glyphStartOffset(glyphId),
          this.glyphLength(glyphId), this.imageFormat());
    }

    /**
     * Gets the full offset of the glyph within the EBDT table.
     *
     * @param glyphId the glyph id
     * @return the glyph offset
     */
    public final int glyphOffset(int glyphId) {
      return this.imageDataOffset() + this.glyphStartOffset(glyphId);
    }

    /**
     * Gets the offset of the glyph relative to the block for this index
     * subtable.
     *
     * @param glyphId the glyph id
     * @return the glyph offset
     */
    public abstract int glyphStartOffset(int glyphId);

    /**
     * Gets the length of the glyph within the EBDT table.
     *
     * @param glyphId the glyph id
     * @return the glyph offset
     */
    public abstract int glyphLength(int glyphId);

    /**
     * Checks that the glyph id is within the correct range. If it returns the
     * offset of the glyph id from the start of the range.
     *
     * @param glyphId
     * @return the offset of the glyphId from the start of the glyph range
     * @throws IndexOutOfBoundsException if the glyph id is not within the
     *         correct range
     */
    protected int checkGlyphRange(int glyphId) {
      return IndexSubTable.checkGlyphRange(glyphId, this.firstGlyphIndex(), this.lastGlyphIndex());
    }

    protected int serializeIndexSubHeader(WritableFontData data) {
      int size = data.writeUShort(Offset.indexSubHeader_indexFormat.offset, this.indexFormat);
      size += data.writeUShort(Offset.indexSubHeader_imageFormat.offset, this.imageFormat);
      size += data.writeULong(Offset.indexSubHeader_imageDataOffset.offset, this.imageDataOffset);
      return size;
    }

    abstract Iterator<BitmapGlyphInfo> iterator();

    /*
     * The following methods will never be called but they need to be here to
     * allow the BitmapSizeTable to see these methods through an abstract
     * reference.
     */
    @Override
    protected T subBuildTable(ReadableFontData data) {
      return null;
    }

    @Override
    protected void subDataSet() {
      // NOP
    }


    @Override
    protected int subDataSizeToSerialize() {
      return 0;
    }

    @Override
    protected boolean subReadyToSerialize() {
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      return 0;
    }

    @Override
    public String toString() {
      String s =
          "IndexSubTable: " + "[0x" + Integer.toHexString(this.firstGlyphIndex()) + " : Ox"
              + Integer.toHexString(this.lastGlyphIndex()) + "]" + ", format = " + this.indexFormat
              + ", image format = " + this.imageFormat() + ", imageOff = 0x"
              + Integer.toHexString(this.imageDataOffset()) + "\n";
      if (DEBUG) {
        for (int g = this.firstGlyphIndex(); g < this.lastGlyphIndex(); g++) {
          s += "\tgid = " + g + ", offset = " + this.glyphStartOffset(g) + "\n";
        }
      }
      return s;
    }
  }
}
