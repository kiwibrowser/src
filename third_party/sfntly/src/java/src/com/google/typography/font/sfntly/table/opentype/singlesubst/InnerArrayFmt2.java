package com.google.typography.font.sfntly.table.opentype.singlesubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.component.NumRecord;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordList;
import com.google.typography.font.sfntly.table.opentype.component.RecordsTable;

public class InnerArrayFmt2 extends RecordsTable<NumRecord> {
  private static final int FIELD_COUNT = 1;

  private static final int COVERAGE_INDEX = 0;
  private static final int COVERAGE_DEFAULT = 0;
  public final CoverageTable coverage;

  public InnerArrayFmt2(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    int coverageOffset = getField(COVERAGE_INDEX);
    coverage = new CoverageTable(data.slice(coverageOffset), 0, dataIsCanonical);
  }

  @Override
  protected RecordList<NumRecord> createRecordList(ReadableFontData data) {
    return new NumRecordList(data);
  }

  public static class Builder extends RecordsTable.Builder<InnerArrayFmt2, NumRecord> {
    public Builder() {
      super();
    }

    public Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    public Builder(InnerArrayFmt2 table) {
      super(table);
    }

    @Override
    protected InnerArrayFmt2 readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new InnerArrayFmt2(data, base, dataIsCanonical);
    }

    @Override
    protected void initFields() {
      setField(COVERAGE_INDEX, COVERAGE_DEFAULT);
    }

    @Override
    protected int fieldCount() {
      return FIELD_COUNT;
    }

    @Override
    protected RecordList<NumRecord> readRecordList(ReadableFontData data, int base) {
      if (base != 0) {
        throw new UnsupportedOperationException();
      }
      return new NumRecordList(data);
    }
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }
}
