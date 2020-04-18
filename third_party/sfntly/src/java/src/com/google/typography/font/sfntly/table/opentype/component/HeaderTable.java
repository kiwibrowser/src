package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

public abstract class HeaderTable extends SubTable {
  protected static final int FIELD_SIZE = 2;
  protected boolean dataIsCanonical = false;
  protected int base = 0;

  protected HeaderTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data);
    this.base = base;
    this.dataIsCanonical = dataIsCanonical;
  }

  public int getField(int index) {
    return data.readUShort(base + index * FIELD_SIZE);
  }

  public int headerSize() {
    return FIELD_SIZE * fieldCount();
  }

  public abstract int fieldCount();

  public abstract static class Builder<T extends HeaderTable> extends VisibleSubTable.Builder<T> {
    private Map<Integer, Integer> map = new HashMap<Integer, Integer>();
    protected boolean dataIsCanonical = false;

    protected Builder() {
      super();
      initFields();
    }

    protected Builder(ReadableFontData data) {
      super(data);
      initFields();
    }

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data);
      this.dataIsCanonical = dataIsCanonical;
      initFields();
    }

    protected Builder(T table) {
      super();
      initFields();
      for (int i = 0; i < table.fieldCount(); i++) {
        map.put(i, table.getField(i));
      }
    }

    protected int setField(int index, int value) {
      return map.put(index, value);
    }

    protected int getField(int index) {
      return map.get(index);
    }

    protected abstract void initFields();

    protected abstract int fieldCount();

    public int headerSize() {
      return FIELD_SIZE * fieldCount();
    }

    /**
     * Even though public, not to be used by the end users. Made public only
     * make it available to packages under
     * {@code com.google.typography.font.sfntly.table.opentype}.
     */
    @Override
    public int subDataSizeToSerialize() {
      return headerSize();
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      for (Entry<Integer, Integer> entry : map.entrySet()) {
        newData.writeUShort(entry.getKey() * FIELD_SIZE, entry.getValue());
      }
      return headerSize();
    }

    @Override
    public void subDataSet() {
      map = new HashMap<Integer, Integer>();
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }
  }
}
