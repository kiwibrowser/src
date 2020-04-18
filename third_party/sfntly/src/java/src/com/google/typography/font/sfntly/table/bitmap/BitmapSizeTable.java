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
import com.google.typography.font.sfntly.math.FontMath;
import com.google.typography.font.sfntly.table.SubTable;
import com.google.typography.font.sfntly.table.bitmap.EblcTable.Offset;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

public final class BitmapSizeTable extends SubTable {
  // binary search would be faster but many fonts have index subtables that
  // aren't sorted
  private static final boolean USE_BINARY_SEARCH = false;

  private final Object indexSubTablesLock = new Object();
  private volatile List<IndexSubTable> indexSubTables = null;

  protected BitmapSizeTable(ReadableFontData data, ReadableFontData masterData) {
    super(data, masterData);
  }

  public int indexSubTableArrayOffset() {
    return this.data.readULongAsInt(Offset.bitmapSizeTable_indexSubTableArrayOffset.offset);
  }

  public int indexTableSize() {
    return this.data.readULongAsInt(Offset.bitmapSizeTable_indexTableSize.offset);
  }

  private static int numberOfIndexSubTables(ReadableFontData data, int tableOffset) {
    return data.readULongAsInt(tableOffset + Offset.bitmapSizeTable_numberOfIndexSubTables.offset);
  }

  public int numberOfIndexSubTables() {
    return BitmapSizeTable.numberOfIndexSubTables(this.data, 0);
  }

  public int colorRef() {
    return this.data.readULongAsInt(Offset.bitmapSizeTable_colorRef.offset);
  }

  // TODO(stuartg): implement later
  public void /* SBitLineMetrics */hori() {
    // NOP
  }

  // TODO(stuartg): implement later
  public void /* SBitLineMetrics */vert() {
    // NOP
  }

  public int startGlyphIndex() {
    return this.data.readUShort(Offset.bitmapSizeTable_startGlyphIndex.offset);
  }

  public int endGlyphIndex() {
    return this.data.readUShort(Offset.bitmapSizeTable_endGlyphIndex.offset);
  }

  public int ppemX() {
    return this.data.readByte(Offset.bitmapSizeTable_ppemX.offset);
  }

  public int ppemY() {
    return this.data.readByte(Offset.bitmapSizeTable_ppemY.offset);
  }

  public int bitDepth() {
    return this.data.readByte(Offset.bitmapSizeTable_bitDepth.offset);
  }

  public int flagsAsInt() {
    return this.data.readChar(Offset.bitmapSizeTable_flags.offset);
  }

  public IndexSubTable indexSubTable(int index) {
    List<IndexSubTable> subTableList = getIndexSubTableList();
    return subTableList.get(index);
  }

  public BitmapGlyphInfo glyphInfo(int glyphId) {
    IndexSubTable subTable = searchIndexSubTables(glyphId);
    if (subTable == null) {
      return null;
    }
    return subTable.glyphInfo(glyphId);
  }

  public int glyphOffset(int glyphId) {
    IndexSubTable subTable = searchIndexSubTables(glyphId);
    if (subTable == null) {
      return -1;
    }
    return subTable.glyphOffset(glyphId);
  }

  public int glyphLength(int glyphId) {
    IndexSubTable subTable = searchIndexSubTables(glyphId);
    if (subTable == null) {
      return -1;
    }
    return subTable.glyphLength(glyphId);
  }

  public int glyphFormat(int glyphId) {
    IndexSubTable subTable = searchIndexSubTables(glyphId);
    if (subTable == null) {
      return -1;
    }
    return subTable.imageFormat();
  }

  private IndexSubTable searchIndexSubTables(int glyphId) {
    // would be faster to binary search but too many size tables don't have
    // sorted subtables
    if (USE_BINARY_SEARCH) {
      return binarySearchIndexSubTables(glyphId);
    }
    return linearSearchIndexSubTables(glyphId);
  }

  private IndexSubTable linearSearchIndexSubTables(int glyphId) {
    for (IndexSubTable subTable : this.getIndexSubTableList()) {
      if (subTable.firstGlyphIndex() <= glyphId && subTable.lastGlyphIndex() >= glyphId) {
        return subTable;
      }
    }
    return null;
  }

  private IndexSubTable binarySearchIndexSubTables(int glyphId) {
    List<IndexSubTable> subTableList = getIndexSubTableList();
    int index = 0;
    int bottom = 0;
    int top = subTableList.size();
    while (top != bottom) {
      index = (top + bottom) / 2;
      IndexSubTable subTable = subTableList.get(index);
      if (glyphId < subTable.firstGlyphIndex()) {
        // location below current location
        top = index;
      } else {
        if (glyphId <= subTable.lastGlyphIndex()) {
          return subTable;
        }
        // location is above the current location
        bottom = index + 1;
      }
    }
    return null;
  }

  private IndexSubTable createIndexSubTable(int index) {
    return IndexSubTable.createIndexSubTable(
        this.masterReadData(), this.indexSubTableArrayOffset(), index);
  }

  private List<IndexSubTable> getIndexSubTableList() {
    if (this.indexSubTables == null) {
      synchronized (this.indexSubTablesLock) {
        if (this.indexSubTables == null) {
          List<IndexSubTable> subTables =
              new ArrayList<IndexSubTable>(this.numberOfIndexSubTables());
          for (int i = 0; i < this.numberOfIndexSubTables(); i++) {
            subTables.add(this.createIndexSubTable(i));
          }
          this.indexSubTables = subTables;
        }
      }
    }
    return this.indexSubTables;
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("BitmapSizeTable: ");
    List<IndexSubTable> indexSubTableList = this.getIndexSubTableList();
    sb.append("[s=0x");
    sb.append(Integer.toHexString(this.startGlyphIndex()));
    sb.append(", e=0x");
    sb.append(Integer.toHexString(this.endGlyphIndex()));
    sb.append(", ppemx=");
    sb.append(this.ppemX());
    sb.append(", index subtables count=");
    sb.append(this.numberOfIndexSubTables());
    sb.append("]");
    for (int index = 0; index < indexSubTableList.size(); index++) {
      sb.append("\n\t");
      sb.append(index);
      sb.append(": ");
      sb.append(indexSubTableList.get(index));
      sb.append(", ");
    }
    sb.append("\n");
    return sb.toString();
  }

  public static final class Builder extends SubTable.Builder<BitmapSizeTable> {
    List<IndexSubTable.Builder<? extends IndexSubTable>> indexSubTables;

    static Builder createBuilder(WritableFontData data, ReadableFontData masterData) {
      return new Builder(data, masterData);
    }

    static Builder createBuilder(ReadableFontData data, ReadableFontData masterData) {
      return new Builder(data, masterData);
    }

    private Builder(WritableFontData data, ReadableFontData masterData) {
      super(data, masterData);
    }

    private Builder(ReadableFontData data, ReadableFontData masterData) {
      super(data, masterData);
    }

    /**
     * Gets the subtable array offset as set in the original table as read from
     * the font file. This value cannot be explicitly set and will be generated
     * during table building.
     *
     * @return the subtable array offset
     */
    public int indexSubTableArrayOffset() {
      return this.internalReadData().readULongAsInt(
          Offset.bitmapSizeTable_indexSubTableArrayOffset.offset);
    }

    /**
     * Sets the subtable array offset. This is used only during the building
     * process when the objects are being serialized.
     *
     * @param offset the offset to the index subtable array
     */
    void setIndexSubTableArrayOffset(int offset) {
      this.internalWriteData().writeULong(
          Offset.bitmapSizeTable_indexSubTableArrayOffset.offset, offset);
    }

    /**
     * Gets the subtable array size as set in the original table as read from
     * the font file. This value cannot be explicitly set and will be generated
     * during table building.
     *
     * @return the subtable array size
     */
    public int indexTableSize() {
      return this.internalReadData().readULongAsInt(Offset.bitmapSizeTable_indexTableSize.offset);
    }

    /**
     * Sets the subtable size. This is used only during the building process
     * when the objects are being serialized.
     *
     * @param size the offset to the index subtable array
     */
    void setIndexTableSize(int size) {
      this.internalWriteData().writeULong(Offset.bitmapSizeTable_indexTableSize.offset, size);
    }

    public int numberOfIndexSubTables() {
      return this.getIndexSubTableBuilders().size();
    }

    private void setNumberOfIndexSubTables(int numberOfIndexSubTables) {
      this.internalWriteData().writeULong(
          Offset.bitmapSizeTable_numberOfIndexSubTables.offset, numberOfIndexSubTables);
    }

    public int colorRef() {
      return this.internalReadData().readULongAsInt(Offset.bitmapSizeTable_colorRef.offset);
    }

    // TODO(stuartg): implement later
    public void /* SBitLineMetrics */hori() {
      // NOP
    }

    // TODO(stuartg): implement later
    public void /* SBitLineMetrics */vert() {
      // NOP
    }

    public int startGlyphIndex() {
      return this.internalReadData().readUShort(Offset.bitmapSizeTable_startGlyphIndex.offset);
    }

    public int endGlyphIndex() {
      return this.internalReadData().readUShort(Offset.bitmapSizeTable_endGlyphIndex.offset);
    }

    public int ppemX() {
      return this.internalReadData().readByte(Offset.bitmapSizeTable_ppemX.offset);
    }

    public int ppemY() {
      return this.internalReadData().readByte(Offset.bitmapSizeTable_ppemY.offset);
    }

    public int bitDepth() {
      return this.internalReadData().readByte(Offset.bitmapSizeTable_bitDepth.offset);
    }

    public int flagsAsInt() {
      return this.internalReadData().readChar(Offset.bitmapSizeTable_flags.offset);
    }

    public IndexSubTable.Builder<? extends IndexSubTable> indexSubTableBuilder(int index) {
      List<IndexSubTable.Builder<? extends IndexSubTable>> subTableList =
          getIndexSubTableBuilders();
      return subTableList.get(index);
    }

    public BitmapGlyphInfo glyphInfo(int glyphId) {
      IndexSubTable.Builder<? extends IndexSubTable> subTable = searchIndexSubTables(glyphId);
      if (subTable == null) {
        return null;
      }
      return subTable.glyphInfo(glyphId);
    }

    public int glyphOffset(int glyphId) {
      IndexSubTable.Builder<? extends IndexSubTable> subTable = searchIndexSubTables(glyphId);
      if (subTable == null) {
        return -1;
      }
      return subTable.glyphOffset(glyphId);
    }

    public int glyphLength(int glyphId) {
      IndexSubTable.Builder<? extends IndexSubTable> subTable = searchIndexSubTables(glyphId);
      if (subTable == null) {
        return -1;
      }
      return subTable.glyphLength(glyphId);
    }

    public int glyphFormat(int glyphId) {
      IndexSubTable.Builder<? extends IndexSubTable> subTable = searchIndexSubTables(glyphId);
      if (subTable == null) {
        return -1;
      }
      return subTable.imageFormat();
    }

    public List<IndexSubTable.Builder<? extends IndexSubTable>> indexSubTableBuilders() {
      return this.getIndexSubTableBuilders();
    }

    private class BitmapGlyphInfoIterator implements Iterator<BitmapGlyphInfo> {
      Iterator<IndexSubTable.Builder<? extends IndexSubTable>> subTableIter;
      Iterator<BitmapGlyphInfo> subTableGlyphInfoIter;

      public BitmapGlyphInfoIterator() {
        this.subTableIter = BitmapSizeTable.Builder.this.getIndexSubTableBuilders().iterator();
      }

      @Override
      public boolean hasNext() {
        if (this.subTableGlyphInfoIter != null && this.subTableGlyphInfoIter.hasNext()) {
          return true;
        }
        while (subTableIter.hasNext()) {
          IndexSubTable.Builder<? extends IndexSubTable> indexSubTable = this.subTableIter.next();
          this.subTableGlyphInfoIter = indexSubTable.iterator();
          if (this.subTableGlyphInfoIter.hasNext()) {
            return true;
          }
        }
        return false;
      }

      @Override
      public BitmapGlyphInfo next() {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
        return this.subTableGlyphInfoIter.next();
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Unable to remove a glyph info.");
      }
    }

    Iterator<BitmapGlyphInfo> iterator() {
      return new BitmapGlyphInfoIterator();
    }

    protected void revert() {
      this.indexSubTables = null;
      this.setModelChanged(false);
    }

    public Map<Integer, BitmapGlyphInfo> generateLocaMap() {
      Map<Integer, BitmapGlyphInfo> locaMap = new HashMap<Integer, BitmapGlyphInfo>();
      Iterator<BitmapGlyphInfo> iter = this.iterator();
      while (iter.hasNext()) {
        BitmapGlyphInfo info = iter.next();
        locaMap.put(info.glyphId(), info);
      }
      return locaMap;
    }

    private IndexSubTable.Builder<? extends IndexSubTable> searchIndexSubTables(int glyphId) {
      // would be faster to binary search but too many size tables don't have
      // sorted subtables
      if (USE_BINARY_SEARCH) {
        return binarySearchIndexSubTables(glyphId);
      }
      return linearSearchIndexSubTables(glyphId);
    }

    private IndexSubTable.Builder<? extends IndexSubTable> linearSearchIndexSubTables(int glyphId) {
      List<IndexSubTable.Builder<? extends IndexSubTable>> subTableList =
          getIndexSubTableBuilders();
      for (IndexSubTable.Builder<? extends IndexSubTable> subTable : subTableList) {
        if (subTable.firstGlyphIndex() <= glyphId && subTable.lastGlyphIndex() >= glyphId) {
          return subTable;
        }
      }
      return null;
    }

    private IndexSubTable.Builder<? extends IndexSubTable> binarySearchIndexSubTables(int glyphId) {
      List<IndexSubTable.Builder<? extends IndexSubTable>> subTableList =
          getIndexSubTableBuilders();
      int index = 0;
      int bottom = 0;
      int top = subTableList.size();
      while (top != bottom) {
        index = (top + bottom) / 2;
        IndexSubTable.Builder<? extends IndexSubTable> subTable = subTableList.get(index);
        if (glyphId < subTable.firstGlyphIndex()) {
          // location below current location
          top = index;
        } else {
          if (glyphId <= subTable.lastGlyphIndex()) {
            return subTable;
          }
          // location is above the current location
          bottom = index + 1;
        }
      }
      return null;
    }

    private List<IndexSubTable.Builder<? extends IndexSubTable>> getIndexSubTableBuilders() {
      if (this.indexSubTables == null) {
        this.initialize(this.internalReadData());
        this.setModelChanged();
      }
      return this.indexSubTables;
    }

    private void initialize(ReadableFontData data) {
      if (this.indexSubTables == null) {
        this.indexSubTables = new ArrayList<IndexSubTable.Builder<? extends IndexSubTable>>();
      } else {
        this.indexSubTables.clear();
      }
      if (data != null) {
        int numberOfIndexSubTables = BitmapSizeTable.numberOfIndexSubTables(data, 0);
        for (int i = 0; i < numberOfIndexSubTables; i++) {
          this.indexSubTables.add(this.createIndexSubTableBuilder(i));
        }
      }
    }

    private IndexSubTable.Builder<? extends IndexSubTable> createIndexSubTableBuilder(int index) {
      return IndexSubTable.Builder.createBuilder(
          this.masterReadData(), this.indexSubTableArrayOffset(), index);
    }

    @Override
    protected BitmapSizeTable subBuildTable(ReadableFontData data) {
      return new BitmapSizeTable(data, this.masterReadData());
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.indexSubTableBuilders() == null) {
        return 0;
      }
      int size = Offset.bitmapSizeTableLength.offset;
      boolean variable = false;
      for (IndexSubTable.Builder<? extends IndexSubTable> subTableBuilder : this.indexSubTables) {
        size += Offset.indexSubTableEntryLength.offset;
        int subTableSize = subTableBuilder.subDataSizeToSerialize();
        int padding =
            FontMath.paddingRequired(Math.abs(subTableSize), FontData.DataSize.ULONG.size());
        variable = subTableSize > 0 ? variable : true;
        size += Math.abs(subTableSize) + padding;
      }
      return variable ? -size : size;
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.indexSubTableBuilders() == null) {
        return false;
      }
      return true;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      this.setNumberOfIndexSubTables(this.indexSubTableBuilders().size());
      int size = this.internalReadData().copyTo(newData);
      return size;
    }
  }
}