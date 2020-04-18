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
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Format 4 Index Subtable Entry.
 * 
 * @author Stuart Gill
 * 
 */
public final class IndexSubTableFormat4 extends IndexSubTable {
  private IndexSubTableFormat4(ReadableFontData data, int firstGlyphIndex, int lastGlyphIndex) {
    super(data, firstGlyphIndex, lastGlyphIndex);
  }

  private static int numGlyphs(ReadableFontData data, int tableOffset) {
    int numGlyphs = data.readULongAsInt(tableOffset + Offset.indexSubTable4_numGlyphs.offset);
    return numGlyphs;
  }
  
  @Override
  public int numGlyphs() {
    return IndexSubTableFormat4.numGlyphs(this.data, 0);
  }

  @Override
  public int glyphStartOffset(int glyphId) {
    this.checkGlyphRange(glyphId);
    int pairIndex = this.findCodeOffsetPair(glyphId);
    if (pairIndex < 0) {
      return -1;
    }
    return this.data.readUShort(
        Offset.indexSubTable4_glyphArray.offset + pairIndex * Offset.codeOffsetPairLength.offset
            + Offset.codeOffsetPair_offset.offset);
  }

  @Override
  public int glyphLength(int glyphId) {
    this.checkGlyphRange(glyphId);
    int pairIndex = this.findCodeOffsetPair(glyphId);
    if (pairIndex < 0) {
      return -1;
    }
    return (this.data.readUShort(Offset.indexSubTable4_glyphArray.offset + (pairIndex + 1)
        * Offset.codeOffsetPairLength.offset + Offset.codeOffsetPair_offset.offset))
        - this.data.readUShort(Offset.indexSubTable4_glyphArray.offset + (pairIndex)
            * Offset.codeOffsetPairLength.offset + Offset.codeOffsetPair_offset.offset);
  }

  protected int findCodeOffsetPair(int glyphId) {
    return this.data.searchUShort(Offset.indexSubTable4_glyphArray.offset,
        Offset.codeOffsetPairLength.offset, this.numGlyphs(), glyphId);
  }
  
  public static class CodeOffsetPair {
    protected int glyphCode;
    protected int offset;

    private CodeOffsetPair(int glyphCode, int offset) {
      this.glyphCode = glyphCode;
      this.offset = offset;
    }

    public int glyphCode() {
      return this.glyphCode;
    }

    public int offset() {
      return this.offset;
    }
  }
  
  public static final class CodeOffsetPairBuilder extends CodeOffsetPair {
    private CodeOffsetPairBuilder(int glyphCode, int offset) {
      super(glyphCode, offset);
    }

    public void setGlyphCode(int glyphCode) {
      this.glyphCode = glyphCode;
    }

    public void setOffset(int offset) {
      this.offset = offset;
    }
  }
  
  private static final class CodeOffsetPairGlyphCodeComparator implements Comparator<
      CodeOffsetPair> {
    private CodeOffsetPairGlyphCodeComparator() {
      // Prevent construction.
    }

    @Override
    public int compare(CodeOffsetPair p1, CodeOffsetPair p2) {
      return p1.glyphCode - p2.glyphCode;
    }
  }
  public static final Comparator<CodeOffsetPair> CodeOffsetPairComparatorByGlyphCode =
      new CodeOffsetPairGlyphCodeComparator();
  
  public static final class Builder extends IndexSubTable.Builder<IndexSubTableFormat4> {
    private List<CodeOffsetPairBuilder> offsetPairArray;

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
      int numGlyphs = IndexSubTableFormat4.numGlyphs(data, indexSubTableOffset);
      return Offset.indexSubTable4_glyphArray.offset + numGlyphs
          * Offset.indexSubTable4_codeOffsetPairLength.offset;
    }

    private Builder() {
      super(Offset.indexSubTable4_builderDataSize.offset, Format.FORMAT_4);
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
      this.checkGlyphRange(glyphId);
      int pairIndex = this.findCodeOffsetPair(glyphId);
      if (pairIndex == -1) {
        return -1;
      }
      return this.getOffsetArray().get(pairIndex + 1).offset()
          - this.getOffsetArray().get(pairIndex).offset();
    }

    @Override
    public int glyphStartOffset(int glyphId) {
      this.checkGlyphRange(glyphId);
      int pairIndex = this.findCodeOffsetPair(glyphId);
      if (pairIndex == -1) {
        return -1;
      }
      return this.getOffsetArray().get(pairIndex).offset();
    }

    public List<CodeOffsetPairBuilder> offsetArray() {
      return this.getOffsetArray();
    }

    private List<CodeOffsetPairBuilder> getOffsetArray() {
      if (this.offsetPairArray == null) {
        this.initialize(super.internalReadData());
        super.setModelChanged();
      }
      return this.offsetPairArray;
    }

    private void initialize(ReadableFontData data) {
      if (this.offsetPairArray == null) {
        this.offsetPairArray = new ArrayList<CodeOffsetPairBuilder>();
      } else {
        this.offsetPairArray.clear();
      }

      if (data != null) {
        int numPairs = IndexSubTableFormat4.numGlyphs(data, 0) + 1;
        int offset = Offset.indexSubTable4_glyphArray.offset;
        for (int i = 0; i < numPairs; i++) {
          int glyphCode =
              data.readUShort(offset + Offset.indexSubTable4_codeOffsetPair_glyphCode.offset);
          int glyphOffset =
              data.readUShort(offset + Offset.indexSubTable4_codeOffsetPair_offset.offset);
          offset += Offset.indexSubTable4_codeOffsetPairLength.offset;
          CodeOffsetPairBuilder pairBuilder = new CodeOffsetPairBuilder(glyphCode, glyphOffset);
          this.offsetPairArray.add(pairBuilder);
        }
      }
    }

    private int findCodeOffsetPair(int glyphId) {
      List<CodeOffsetPairBuilder> pairList = this.getOffsetArray();
      int location = 0;
      int bottom = 0;
      int top = pairList.size();
      while (top != bottom) {
        location = (top + bottom) / 2;
        CodeOffsetPairBuilder pair = pairList.get(location);
        if (glyphId < pair.glyphCode()) {
          // location is below current location
          top = location;
        } else if (glyphId > pair.glyphCode()) {
          // location is above current location
          bottom = location + 1;
        } else {
          return location;
        }
      }
      return -1;
    }
    
    public void setOffsetArray(List<CodeOffsetPairBuilder> array) {
      this.offsetPairArray = array;
      this.setModelChanged();
    }

    private class BitmapGlyphInfoIterator implements Iterator<BitmapGlyphInfo> {
      private int codeOffsetPairIndex;

      public BitmapGlyphInfoIterator() {
      }

      @Override
      public boolean hasNext() {
        if (this.codeOffsetPairIndex
            < IndexSubTableFormat4.Builder.this.getOffsetArray().size() - 1) {
          return true;
        }
        return false;
      }

      @Override
      public BitmapGlyphInfo next() {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
        List<CodeOffsetPairBuilder> offsetArray =
            IndexSubTableFormat4.Builder.this.getOffsetArray();
        CodeOffsetPair pair =
            offsetArray.get(this.codeOffsetPairIndex);
        BitmapGlyphInfo info = new BitmapGlyphInfo(pair.glyphCode(),
            IndexSubTableFormat4.Builder.this.imageDataOffset(), pair.offset(),
            offsetArray.get(this.codeOffsetPairIndex + 1).offset() - pair.offset(),
            IndexSubTableFormat4.Builder.this.imageFormat());
        this.codeOffsetPairIndex++;
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
      this.offsetPairArray = null;
    }

    @Override
    protected IndexSubTableFormat4 subBuildTable(ReadableFontData data) {
      return new IndexSubTableFormat4(data, this.firstGlyphIndex(), this.lastGlyphIndex());
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.offsetPairArray == null) {
        return this.internalReadData().length();
      }
      return Offset.indexSubHeaderLength.offset + FontData.DataSize.ULONG.size()
          + this.offsetPairArray.size() * Offset.indexSubTable4_codeOffsetPairLength.offset;
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.offsetPairArray != null) {
        return true;
      }
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = super.serializeIndexSubHeader(newData);
      if (!this.modelChanged()) {
        size += this.internalReadData().slice(Offset.indexSubTable4_numGlyphs.offset).copyTo(
            newData.slice(Offset.indexSubTable4_numGlyphs.offset));
      } else {

        size += newData.writeLong(size, this.offsetPairArray.size() - 1);
        for (CodeOffsetPair pair : this.offsetPairArray) {
          size += newData.writeUShort(size, pair.glyphCode());
          size += newData.writeUShort(size, pair.offset());
        }
      }
      return size;
    }
  }
}
