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
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Format 3 Index Subtable Entry.
 *
 * @author Stuart Gill
 *
 */
public final class IndexSubTableFormat3 extends IndexSubTable {
  private IndexSubTableFormat3(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
    super(data, firstGlyphIndex, lastGlyphIndex);
  }

  @Override
  public int numGlyphs() {
    return this.lastGlyphIndex() - this.firstGlyphIndex() + 1;
  }

  @Override
  public int glyphStartOffset(int glyphId) {
    int loca = this.checkGlyphRange(glyphId);
    return this.loca(loca);
  }

  @Override
  public int glyphLength(int glyphId) {
    int loca = this.checkGlyphRange(glyphId);
    return this.loca(loca + 1) - this.loca(loca);
  }

  private int loca(int loca) {
    int readLocation =
        Offset.indexSubTable3_offsetArray.offset + loca * FontData.DataSize.USHORT.size();
    int readOffset = this.data.readUShort(
        Offset.indexSubTable3_offsetArray.offset + loca * FontData.DataSize.USHORT.size());
    return readOffset;
  }
  
  public static final class Builder extends IndexSubTable.Builder<IndexSubTableFormat3> {
    private List<Integer> offsetArray;
    
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
      return Offset.indexSubHeaderLength.offset + (lastGlyphIndex - firstGlyphIndex + 1 + 1)
          * FontData.DataSize.USHORT.size();
    }
      
    private Builder() {
      super(Offset.indexSubTable3_builderDataSize.offset, Format.FORMAT_3);
    }

    private Builder(WritableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    private Builder(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
      super(data, firstGlyphIndex, lastGlyphIndex);
    }

    @Override
    public int numGlyphs() {
      return this.getOffsetArray().size() - 1;
    }

    @Override
    public int glyphLength(int glyphId) {
      int loca = this.checkGlyphRange(glyphId);
      List<Integer> offsetArray = this.getOffsetArray();
      return offsetArray.get(loca + 1) - offsetArray.get(loca);
    }

    @Override
    public int glyphStartOffset(int glyphId) {
      int loca = this.checkGlyphRange(glyphId);
      List<Integer> offsetArray = this.getOffsetArray();
      return offsetArray.get(loca);
    }

    public List<Integer> offsetArray() {
      return this.getOffsetArray();
    }

    private List<Integer> getOffsetArray() {
      if (this.offsetArray == null) {
        this.initialize(super.internalReadData());
        super.setModelChanged();
      }
      return this.offsetArray;
    }

    private void initialize(ReadableFontData data) {
      if (this.offsetArray == null) {
        this.offsetArray = new ArrayList<Integer>();
      } else {
        this.offsetArray.clear();
      }

      if (data != null) {
        int numOffsets = (this.lastGlyphIndex() - this.firstGlyphIndex() + 1) + 1;
        for (int i = 0; i < numOffsets; i++) {
          this.offsetArray.add(data.readUShort(
Offset.indexSubTable3_offsetArray.offset + i
              * FontData.DataSize.USHORT.size()));
        }
      }
    }

    public void setOffsetArray(List<Integer> array) {
      this.offsetArray = array;
      this.setModelChanged();
    }

    private class BitmapGlyphInfoIterator implements Iterator<BitmapGlyphInfo> {
      private int glyphId;

      public BitmapGlyphInfoIterator() {
        this.glyphId = IndexSubTableFormat3.Builder.this.firstGlyphIndex();
      }

      @Override
      public boolean hasNext() {
        if (this.glyphId <= IndexSubTableFormat3.Builder.this.lastGlyphIndex()) {
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
            new BitmapGlyphInfo(this.glyphId, IndexSubTableFormat3.Builder.this.imageDataOffset(),
                IndexSubTableFormat3.Builder.this.glyphStartOffset(this.glyphId),
                IndexSubTableFormat3.Builder.this.glyphLength(this.glyphId),
                IndexSubTableFormat3.Builder.this.imageFormat());
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
    protected void revert() {
      super.revert();
      this.offsetArray = null;
    }

    @Override
    protected IndexSubTableFormat3 subBuildTable(ReadableFontData data) {
      return new IndexSubTableFormat3(data, this.firstGlyphIndex(), this.lastGlyphIndex());
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.offsetArray == null) {
        return this.internalReadData().length();
      }
      return Offset.indexSubHeaderLength.offset + this.offsetArray.size()
          * FontData.DataSize.ULONG.size();
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.offsetArray != null) {
        return true;
      }
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = super.serializeIndexSubHeader(newData);
      if (!this.modelChanged()) {
        size += this.internalReadData().slice(Offset.indexSubTable3_offsetArray.offset).copyTo(
            newData.slice(Offset.indexSubTable3_offsetArray.offset));
      } else {

        for (Integer loca : this.offsetArray) {
          size += newData.writeUShort(size, loca);
        }
      }
      return size;
    }
  }
}
