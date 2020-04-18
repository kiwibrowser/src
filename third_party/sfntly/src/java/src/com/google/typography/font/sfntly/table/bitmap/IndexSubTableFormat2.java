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
import com.google.typography.font.sfntly.table.bitmap.EblcTable.Offset;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * Format 2 Index Subtable Entry.
 * 
 * @author Stuart Gill
 * 
 */
public final class IndexSubTableFormat2 extends IndexSubTable {
  private final int imageSize;
  
  private IndexSubTableFormat2(ReadableFontData data, int first, int last) {
    super(data, first, last);
    this.imageSize = this.data.readULongAsInt(Offset.indexSubTable2_imageSize.offset);
  }

  public int imageSize() {
    return this.data.readULongAsInt(Offset.indexSubTable2_imageSize.offset);
  }

  public BigGlyphMetrics bigMetrics() {
    return new BigGlyphMetrics(this.data.slice(Offset.indexSubTable2_bigGlyphMetrics.offset,
        BigGlyphMetrics.Offset.metricsLength.offset));
  }

  @Override
  public int numGlyphs() {
    return this.lastGlyphIndex() - this.firstGlyphIndex() + 1;
  }

  @Override
  public int glyphStartOffset(int glyphId) {
    int loca = this.checkGlyphRange(glyphId);
    return loca * this.imageSize;
  }

  @Override
  public int glyphLength(int glyphId) {
    this.checkGlyphRange(glyphId);
    return this.imageSize;
  }
  
  public static final class Builder extends IndexSubTable.Builder<IndexSubTableFormat2> {

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
      return Offset.indexSubTable2Length.offset;
    }
    
    private Builder() {
      super(Offset.indexSubTable2_builderDataSize.offset, Format.FORMAT_2);
      this.metrics = BigGlyphMetrics.Builder.createBuilder();
    }

    private Builder(WritableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    private Builder(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    @Override
    public int numGlyphs() {
      return this.lastGlyphIndex() - this.firstGlyphIndex() + 1;
    }

    @Override
    public int glyphStartOffset(int glyphId) {
      int loca = super.checkGlyphRange(glyphId);
      return loca * this.imageSize();
    }

    @Override
    public int glyphLength(int glyphId) {
      super.checkGlyphRange(glyphId);
      return this.imageSize();
    }

    public int imageSize() {
      return this.internalReadData().readULongAsInt(Offset.indexSubTable2_imageSize.offset);
    }

    public void setImageSize(int imageSize) {
      this.internalWriteData().writeULong(Offset.indexSubTable2_imageSize.offset, imageSize);
    }
    
    public BigGlyphMetrics.Builder bigMetrics() {
      if (this.metrics == null) {
        WritableFontData data =
            this.internalWriteData().slice(Offset.indexSubTable2_bigGlyphMetrics.offset,
                BigGlyphMetrics.Offset.metricsLength.offset);
        this.metrics = new BigGlyphMetrics.Builder(data);
      }
      return this.metrics;
    }

    private class BitmapGlyphInfoIterator implements Iterator<BitmapGlyphInfo> {
      private int glyphId;

      public BitmapGlyphInfoIterator() {
        this.glyphId = IndexSubTableFormat2.Builder.this.firstGlyphIndex();
      }

      @Override
      public boolean hasNext() {
        if (this.glyphId <= IndexSubTableFormat2.Builder.this.lastGlyphIndex()) {
          return true;
        }
        return false;
      }

      @Override
      public BitmapGlyphInfo next() {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
        BitmapGlyphInfo info =
            new BitmapGlyphInfo(this.glyphId, IndexSubTableFormat2.Builder.this.imageDataOffset(),
                IndexSubTableFormat2.Builder.this.glyphStartOffset(this.glyphId),
                IndexSubTableFormat2.Builder.this.glyphLength(this.glyphId),
                IndexSubTableFormat2.Builder.this.imageFormat());
        this.glyphId++;
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
    protected IndexSubTableFormat2 subBuildTable(ReadableFontData data) {
      return new IndexSubTableFormat2(data, this.firstGlyphIndex(), this.lastGlyphIndex());
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      return Offset.indexSubTable2Length.offset;
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = super.serializeIndexSubHeader(newData);
      if (this.metrics == null) {
        size += this.internalReadData().slice(size).copyTo(newData.slice(size));
      } else {
        size += newData.writeLong(Offset.indexSubTable2_imageSize.offset, this.imageSize());
        size += this.metrics.subSerialize(newData.slice(size));
      }
      return size;
    }
  }
}
