package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.NumRecord;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordsTable;

public class LangSysTable extends RecordsTable<NumRecord> {
  private static final int FIELD_COUNT = 2;

  private static final int LOOKUP_ORDER_INDEX = 0;
  private static final int LOOKUP_ORDER_CONST = 0;

  private static final int REQ_FEATURE_INDEX_INDEX = 1;
  private static final int NO_REQ_FEATURE = 0xffff;

  LangSysTable(ReadableFontData data, boolean dataIsCanonical) {
    super(data, dataIsCanonical);
    if (getField(LOOKUP_ORDER_INDEX) != LOOKUP_ORDER_CONST) {
      throw new IllegalArgumentException();
    }
  }

  @Override
  protected RecordList<NumRecord> createRecordList(ReadableFontData data) {
    return new NumRecordList(data);
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  static class Builder extends RecordsTable.Builder<LangSysTable, NumRecord> {
    Builder() {
      super();
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    // //////////////////////////////
    // private methods to update

    @Override
    protected void initFields() {
      setField(LOOKUP_ORDER_INDEX, LOOKUP_ORDER_CONST);
      setField(REQ_FEATURE_INDEX_INDEX, NO_REQ_FEATURE);
    }

    @Override
    protected LangSysTable readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new LangSysTable(data, dataIsCanonical);
    }

    @Override
    protected RecordList<NumRecord> readRecordList(ReadableFontData data, int base) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new NumRecordList(data);
    }

    @Override
    public int fieldCount() {
      return FIELD_COUNT;
    }
  }
}
