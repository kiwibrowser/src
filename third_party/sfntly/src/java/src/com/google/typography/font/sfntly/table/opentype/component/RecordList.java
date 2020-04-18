package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

public abstract class RecordList<T extends Record> implements Iterable<T> {
  private static final int COUNT_OFFSET = 0;
  static final int DATA_OFFSET = 2;
  final int base;
  private final int recordBase;

  final ReadableFontData readData;
  private final WritableFontData writeData;
  private int count;
  private List<T> recordsToWrite;

  /*
   *private RecordList(WritableFontData data) { this.readData = null;
   * this.writeData = data; this.count = 0; this.base = 0; this.recordBase =
   * RECORD_BASE_DEFAULT; if (writeData != null) {
   * writeData.writeUShort(COUNT_OFFSET, 0); } }
   */
  protected RecordList(ReadableFontData data, int countDecrement, int countOffset,
      int valuesOffset) {
    this.readData = data;
    this.writeData = null;
    this.base = countOffset;
    this.recordBase = valuesOffset; // base + RECORD_BASE_DEFAULT +
                                    // recordBaseOffset;
    if (readData != null) {
      this.count = data.readUShort(countOffset + COUNT_OFFSET) - countDecrement;
    }
  }

  protected RecordList(RecordList<T> other) {
    this.readData = other.readData;
    this.writeData = other.writeData;
    this.base = other.base;
    this.recordBase = other.recordBase;
    this.count = other.count;
    this.recordsToWrite = other.recordsToWrite;
  }

  protected RecordList(ReadableFontData data) {
    this(data, 0);
  }

  protected RecordList(ReadableFontData data, int countDecrement) {
    this(data, countDecrement, 0, DATA_OFFSET);
  }

  protected RecordList(ReadableFontData data, int countDecrement, int countOffset) {
    this(data, countDecrement, countOffset, countOffset + DATA_OFFSET);
  }

  public int count() {
    if (recordsToWrite != null) {
      return recordsToWrite.size();
    }
    return count;
  }

  public int limit() {
    return sizeOfList(count());
  }

  private int sizeOfList(int count) {
    return baseAt(recordBase, count);
  }

  private int baseAt(int base, int index) {
    return base + index * recordSize();
  }

  T get(int index) {
    if (recordsToWrite != null) {
      return recordsToWrite.get(index);
    }
    return getRecordAt(readData, sizeOfList(index));
  }

  public boolean contains(T record) {
    if (recordsToWrite != null) {
      return recordsToWrite.contains(record);
    }

    Iterator<T> iterator = iterator();
    while (iterator.hasNext()) {
      if (record.equals(iterator.next())) {
        return true;
      }
    }
    return false;
  }

  @Override
  public Iterator<T> iterator() {
    if (recordsToWrite != null) {
      return recordsToWrite.iterator();
    }

    return new Iterator<T>() {
      private int current = 0;

      @Override
      public boolean hasNext() {
        return current < count;
      }

      @Override
      public T next() {
        if (!hasNext()) {
          throw new NoSuchElementException();
        }
        return getRecordAt(readData, sizeOfList(current++));
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

  public RecordList<T> add(T record) {
    copyFromRead();
    recordsToWrite.add(record);
    return this;
  }

  public int writeTo(WritableFontData writeData) {
    copyFromRead();

    writeData.writeUShort(base + COUNT_OFFSET, count);
    int nextWritePos = recordBase;
    for (T record : recordsToWrite) {
      nextWritePos += record.writeTo(writeData, nextWritePos);
    }
    return nextWritePos - recordBase + DATA_OFFSET; // bytes wrote
  }

  private void copyFromRead() {
    if (recordsToWrite == null) {
      recordsToWrite = new ArrayList<T>(count);
      Iterator<T> iterator = iterator();
      while (iterator.hasNext()) {
        recordsToWrite.add(iterator.next());
      }
    }
  }

  protected abstract T getRecordAt(ReadableFontData data, int pos);

  protected abstract int recordSize();
}
