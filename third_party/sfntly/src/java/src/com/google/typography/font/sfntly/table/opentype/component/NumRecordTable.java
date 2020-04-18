package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;

public class NumRecordTable extends RecordsTable<NumRecord> {

  public NumRecordTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  public NumRecordTable(NumRecordList records) {
    super(records);
  }

  @Override
  protected RecordList<NumRecord> createRecordList(ReadableFontData data) {
    return new NumRecordList(data);
  }

  @Override
  public int fieldCount() {
    return 0;
  }

  public static class Builder extends RecordsTable.Builder<NumRecordTable, NumRecord> {
    public Builder() {
      super();
    }

    public Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data, base, dataIsCanonical);
    }

    public Builder(NumRecordTable table) {
      super(table);
    }

    @Override
    protected NumRecordTable readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new NumRecordTable(data, base, dataIsCanonical);
    }

    @Override
    protected RecordList<NumRecord> readRecordList(ReadableFontData data, int base) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new NumRecordList(data);
    }

    @Override
    protected int fieldCount() {
      return 0;
    }

    @Override
    protected void initFields() {
    }
  }
}
