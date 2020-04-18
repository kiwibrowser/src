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

import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.bitmap.EblcTable.Offset;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Format 5 Index Subtable Entry.
 * 
 * @author Stuart Gill
 * 
 */
public final class IndexSubTableFormat5 extends IndexSubTable {
  private final int imageSize;

  private IndexSubTableFormat5(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
    super(data, firstGlyphIndex, lastGlyphIndex);
    this.imageSize = this.data.readULongAsInt(Offset.indexSubTable5_imageSize.offset);
  }

  private static int numGlyphs(ReadableFontData data, int tableOffset) {
    int numGlyphs = data.readULongAsInt(tableOffset + Offset.indexSubTable5_numGlyphs.offset);
    return numGlyphs;
  }

  public int imageSize() {
    return this.data.readULongAsInt(Offset.indexSubTable5_imageSize.offset);
  }

  public BigGlyphMetrics bigMetrics() {
    return new BigGlyphMetrics(this.data.slice(
        Offset.indexSubTable5_bigGlyphMetrics.offset, BigGlyphMetrics.Offset.metricsLength.offset));
  }

  @Override
  public int numGlyphs() {
    return IndexSubTableFormat5.numGlyphs(this.data, 0);
  }

  @Override
  public int glyphStartOffset(int glyphId) {
    this.checkGlyphRange(glyphId);
    int loca =
        this.readFontData().searchUShort(Offset.indexSubTable5_glyphArray.offset,
            FontData.DataSize.USHORT.size(), this.numGlyphs(), glyphId);
    if (loca == -1) {
      return loca;
    }
    return loca * this.imageSize;
  }

  @Override
  public int glyphLength(int glyphId) {
    this.checkGlyphRange(glyphId);
    return this.imageSize;
  }

  public static final class Builder extends IndexSubTable.Builder<IndexSubTableFormat5> {
    private List<Integer> glyphArray;
    private BigGlyphMetrics.Builder metrics;

    public static Builder createBuilder() {
      return new Builder();
    }

    static Builder createBuilder(
        ReadableFontData data, int indexSubTableOffset, int firstGlyphIndex, int lastGlyphIndex) {
      int length = Builder.dataLength(data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
      return new Builder(data.slice(indexSubTableOffset, length), firstGlyphIndex, lastGlyphIndex);
    }

    static Builder createBuilder(
        WritableFontData data, int indexSubTableOffset, int firstGlyphIndex, int lastGlyphIndex) {
      int length = Builder.dataLength(data, indexSubTableOffset, firstGlyphIndex, lastGlyphIndex);
      return new Builder(data.slice(indexSubTableOffset, length), firstGlyphIndex, lastGlyphIndex);
    }

    private static int dataLength(
        ReadableFontData data, int indexSubTableOffset, int firstGlyphIndex, int lastGlyphIndex) {
      int numGlyphs = IndexSubTableFormat5.numGlyphs(data, indexSubTableOffset);
      return Offset.indexSubTable5_glyphArray.offset + numGlyphs * FontData.DataSize.USHORT.size();
    }

    private Builder() {
      super(Offset.indexSubTable5_builderDataSize.offset, Format.FORMAT_5);
      this.metrics = BigGlyphMetrics.Builder.createBuilder();
    }

    private Builder(WritableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    private Builder(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    public int imageSize() {
      return this.internalReadData().readULongAsInt(Offset.indexSubTable5_imageSize.offset);
    }

    public void setImageSize(int imageSize) {
      this.internalWriteData().writeULong(Offset.indexSubTable5_imageSize.offset, imageSize);
    }

    public BigGlyphMetrics.Builder bigMetrics() {
      if (this.metrics == null) {
        WritableFontData data =
            this.internalWriteData().slice(Offset.indexSubTable5_bigGlyphMetrics.offset,
                BigGlyphMetrics.Offset.metricsLength.offset);
        this.metrics = new BigGlyphMetrics.Builder(data);
        this.setModelChanged();
      }
      return this.metrics;
    }
    
    @Override
    public int numGlyphs() {
      return this.getGlyphArray().size();
    }

    @Override
    public int glyphLength(int glyphId) {
      return this.imageSize();
    }

    @Override
    public int glyphStartOffset(int glyphId) {
      this.checkGlyphRange(glyphId);
      List<Integer> glyphArray = this.getGlyphArray();
      int loca = Collections.binarySearch(glyphArray, glyphId);
      if (loca == -1) {
        return -1;
      }
      return loca * this.imageSize();
    }

    public List<Integer> glyphArray() {
      return this.getGlyphArray();
    }

    private List<Integer> getGlyphArray() {
      if (this.glyphArray == null) {
        this.initialize(super.internalReadData());
        super.setModelChanged();
      }
      return this.glyphArray;
    }

    private void initialize(ReadableFontData data) {
      if (this.glyphArray == null) {
        this.glyphArray = new ArrayList<Integer>();
      } else {
        this.glyphArray.clear();
      }

      if (data != null) {
        int numGlyphs = IndexSubTableFormat5.numGlyphs(data, 0);
        for (int i = 0; i < numGlyphs; i++) {
          this.glyphArray.add(data.readUShort(
              Offset.indexSubTable5_glyphArray.offset + i * FontData.DataSize.USHORT.size()));
        }
      }
    }

    public void setGlyphArray(List<Integer> array) {
      this.glyphArray = array;
      this.setModelChanged();
    }

    private class BitmapGlyphInfoIterator implements Iterator<BitmapGlyphInfo> {
      private int offsetIndex;

      public BitmapGlyphInfoIterator() {
      }

      @Override
      public boolean hasNext() {
        if (this.offsetIndex < IndexSubTableFormat5.Builder.this.getGlyphArray().size()) {
          return true;
        }
        return false;
      }

      @Override
      public BitmapGlyphInfo next() {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
        BitmapGlyphInfo info = new BitmapGlyphInfo(
            IndexSubTableFormat5.Builder.this.getGlyphArray().get(this.offsetIndex),
            IndexSubTableFormat5.Builder.this.imageDataOffset(),
                this.offsetIndex * IndexSubTableFormat5.Builder.this.imageSize(),
                IndexSubTableFormat5.Builder.this.imageSize(),
            IndexSubTableFormat5.Builder.this.imageFormat());
        this.offsetIndex++;
        return info;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Unable to remove a glyph info.");
      }
    }

    @Override
    Iterator<BitmapGlyphInfo> iterator() {
      return new BitmapGlyphInfoIterator();
    }

    @Override
    protected void revert() {
      super.revert();
      this.glyphArray = null;
    }

    @Override
    protected IndexSubTableFormat5 subBuildTable(ReadableFontData data) {
      return new IndexSubTableFormat5(data, this.firstGlyphIndex(), this.lastGlyphIndex());
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.glyphArray == null) {
        return this.internalReadData().length();
      }
      return Offset.indexSubTable5_builderDataSize.offset + this.glyphArray.size()
          * FontData.DataSize.USHORT.size();
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.glyphArray != null) {
        return true;
      }
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = super.serializeIndexSubHeader(newData);
      if (!this.modelChanged()) {
        size +=
            this.internalReadData().slice(Offset.indexSubTable5_imageSize.offset)
                .copyTo(newData.slice(Offset.indexSubTable5_imageSize.offset));
      } else {
        size += newData.writeULong(Offset.indexSubTable5_imageSize.offset, this.imageSize());
        size += this.bigMetrics().subSerialize(newData.slice(size));
        size += newData.writeULong(size, this.glyphArray.size());
        for (Integer glyphId : this.glyphArray) {
          size += newData.writeUShort(size, glyphId);
        }
      }
      return size;
    }
  }
}
