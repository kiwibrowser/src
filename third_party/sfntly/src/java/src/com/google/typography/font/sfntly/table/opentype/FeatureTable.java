package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.NumRecord;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordsTable;

public class FeatureTable extends RecordsTable<NumRecord> {
  private static final int FIELD_COUNT = 1;
  private static final int FEATURE_PARAMS_INDEX = 0;
  private static final int FEATURE_PARAMS_DEFAULT = 0;

  FeatureTable(ReadableFontData data, boolean dataIsCanonical) {
    super(data, dataIsCanonical);
  }

  @Override
  protected RecordList<NumRecord> createRecordList(ReadableFontData data) {
    return new NumRecordList(data);
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  static class Builder extends
  RecordsTable.Builder<FeatureTable, NumRecord> {

    Builder() {
      super();
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    @Override
    protected FeatureTable readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new FeatureTable(data, dataIsCanonical);
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

    @Override
    protected void initFields() {
      setField(FEATURE_PARAMS_INDEX, FEATURE_PARAMS_DEFAULT);
    }
  }
}

