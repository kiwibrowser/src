// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;

import java.util.Iterator;
import java.util.Map.Entry;
import java.util.NoSuchElementException;
import java.util.TreeMap;

public abstract class TagOffsetsTable<S extends SubTable> extends HeaderTable
    implements Iterable<S> {
  private final TagOffsetRecordList recordList;

  // ///////////////
  // constructors

  protected TagOffsetsTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    recordList = new TagOffsetRecordList(data.slice(headerSize() + base));
  }

  protected TagOffsetsTable(ReadableFontData data, boolean dataIsCanonical) {
    this(data, 0, dataIsCanonical);
  }

  // ////////////////
  // private methods

  public int count() {
    return recordList.count();
  }

  protected int tagAt(int index) {
    return recordList.get(index).tag;
  }

  public S subTableAt(int index) {
    TagOffsetRecord record = recordList.get(index);
    return subTableForRecord(record);
  }

  @Override
  public Iterator<S> iterator() {
    return new Iterator<S>() {
      private Iterator<TagOffsetRecord> recordIterator = recordList.iterator();

      @Override
      public boolean hasNext() {
        return recordIterator.hasNext();
      }

      @Override
      public S next() {
        if (!hasNext()) {
          throw new NoSuchElementException();
        }
        TagOffsetRecord record = recordIterator.next();
        return subTableForRecord(record);
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

  // ////////////////////////////////////
  // implementations pushed to subclasses

  protected abstract S readSubTable(ReadableFontData data, boolean dataIsCanonical);

  // ////////////////////////////////////
  // private methods

  private S subTableForRecord(TagOffsetRecord record) {
    ReadableFontData newBase = data.slice(record.offset);
    return readSubTable(newBase, dataIsCanonical);
  }

  public abstract static class Builder<T extends HeaderTable, S extends SubTable>
      extends HeaderTable.Builder<T> {

    private TreeMap<Integer, VisibleSubTable.Builder<S>> builders;
    private int serializedLength;
    private int serializedCount;
    private final int base;

    protected Builder() {
      super();
      base = 0;
    }

    protected Builder(TagOffsetsTable.Builder<T, S> other) {
      super();
      builders = other.builders;
      dataIsCanonical = other.dataIsCanonical;
      base = other.base;
    }

    protected Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data);
      this.base = base;
      this.dataIsCanonical = dataIsCanonical;
      if (!dataIsCanonical) {
        prepareToEdit();
      }
    }

    @Override
    public int subDataSizeToSerialize() {
      if (builders != null) {
        computeSizeFromBuilders();
      } else {
        computeSizeFromData(internalReadData().slice(headerSize() + base));
      }
      serializedLength += super.subDataSizeToSerialize();
      return serializedLength;
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      if (serializedLength == 0) {
        return 0;
      }

      int writtenBytes = super.subSerialize(newData);
      if (builders != null) {
        return serializeFromBuilders(newData.slice(writtenBytes));
      }
      return serializeFromData(newData.slice(writtenBytes));
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      builders = null;
    }

    @Override
    public T subBuildTable(ReadableFontData data) {
      return readTable(data, 0, true);
    }

    // ////////////////////////////////////
    // implementations pushed to subclasses

    protected abstract T readTable(ReadableFontData data, int base, boolean dataIsCanonical);

    protected abstract VisibleSubTable.Builder<S> createSubTableBuilder();

    protected abstract VisibleSubTable.Builder<S> createSubTableBuilder(
        ReadableFontData data, int tag, boolean dataIsCanonical);

    // ////////////////////////////////////
    // private methods

    private void prepareToEdit() {
      if (builders == null) {
        initFromData(internalReadData(), headerSize() + base);
        setModelChanged();
      }
    }

    private void initFromData(ReadableFontData data, int base) {
      builders = new TreeMap<Integer, VisibleSubTable.Builder<S>>();
      if (data == null) {
        return;
      }

      data = data.slice(base);
      // Start of the first subtable in the data, if we're canonical.
      TagOffsetRecordList recordList = new TagOffsetRecordList(data);
      if (recordList.count() == 0) {
        return;
      }

      int subTableLimit = recordList.limit();
      Iterator<TagOffsetRecord> recordIterator = recordList.iterator();
      if (dataIsCanonical) {
        do {
          // Each table starts where the previous one ended.
          int offset = subTableLimit;
          TagOffsetRecord record = recordIterator.next();
          int tag = record.tag;
          // Each table ends at the next start, or at the end of the data.
          subTableLimit = record.offset;
          // TODO(cibu): length computation does not seems to be correct.
          int length = subTableLimit - offset;
          VisibleSubTable.Builder<S> builder = createSubTableBuilder(data, offset, length, tag);
          builders.put(tag, builder);
        } while (recordIterator.hasNext());
      } else {
        do {
          TagOffsetRecord record = recordIterator.next();
          int offset = record.offset;
          int tag = record.tag;
          VisibleSubTable.Builder<S> builder = createSubTableBuilder(data, offset, -1, tag);
          builders.put(tag, builder);
        } while (recordIterator.hasNext());
      }
    }

    private void computeSizeFromBuilders() {
      // This does not merge LangSysTables that reference the same
      // features.

      // If there is no data in the default LangSysTable or any
      // of the other LangSysTables, the size is zero, and this table
      // will not be written.

      int len = 0;
      int count = 0;
      for (VisibleSubTable.Builder<? extends SubTable> builder : builders.values()) {
        int sublen = builder.subDataSizeToSerialize();
        if (sublen > 0) {
          ++count;
          len += sublen;
        }
      }
      if (len > 0) {
        len += TagOffsetRecordList.sizeOfListOfCount(count);
      }
      serializedLength = len;
      serializedCount = count;
    }

    private void computeSizeFromData(ReadableFontData data) {
      // This assumes canonical data.
      int len = 0;
      int count = 0;
      if (data != null) {
        len = data.length();
        count = new TagOffsetRecordList(data).count();
      }
      serializedLength = len;
      serializedCount = count;
    }

    private int serializeFromBuilders(WritableFontData newData) {
      // The canonical form of the data consists of the header,
      // the index, then the
      // scriptTables from the index in index order. All
      // scriptTables are distinct; there's no sharing of tables.

      // Find size for table
      int tableSize = TagOffsetRecordList.sizeOfListOfCount(serializedCount);

      // Fill header in table and serialize its builder.
      int subTableFillPos = tableSize;

      TagOffsetRecordList recordList = new TagOffsetRecordList(newData);
      for (Entry<Integer, VisibleSubTable.Builder<S>> entry : builders.entrySet()) {
        int tag = entry.getKey();
        VisibleSubTable.Builder<? extends SubTable> builder = entry.getValue();
        if (builder.serializedLength > 0) {
          TagOffsetRecord record = new TagOffsetRecord(tag, subTableFillPos);
          recordList.add(record);
          subTableFillPos += builder.subSerialize(newData.slice(subTableFillPos));
        }
      }
      recordList.writeTo(newData);
      return subTableFillPos;
    }

    private int serializeFromData(WritableFontData newData) {
      // The source data must be canonical.
      ReadableFontData data = internalReadData().slice(base);
      data.copyTo(newData);
      return data.length();
    }

    private VisibleSubTable.Builder<S> createSubTableBuilder(
        ReadableFontData data, int offset, int length, int tag) {
      boolean dataIsCanonical = (length >= 0);
      ReadableFontData newData = dataIsCanonical ? data.slice(offset, length) : data.slice(offset);
      return createSubTableBuilder(newData, tag, dataIsCanonical);
    }
  }
}
