package com.google.typography.font.sfntly.table.opentype.multiplesubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.OffsetRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class GlyphIds extends OffsetRecordTable<NumRecordTable> {
  private static final int FIELD_COUNT = 1;

  private static final int COVERAGE_INDEX = 0;
  private static final int COVERAGE_DEFAULT = 0;
  public final CoverageTable coverage;

  public GlyphIds(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    int coverageOffset = getField(COVERAGE_INDEX);
    coverage = new CoverageTable(data.slice(coverageOffset), 0, dataIsCanonical);
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  @Override
  public NumRecordTable readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new NumRecordTable(data, 0, dataIsCanonical);
  }

  public static class Builder extends OffsetRecordTable.Builder<GlyphIds, NumRecordTable> {
    public Builder() {
      super();
    }

    public Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    public Builder(GlyphIds table) {
      super(table);
    }

    @Override
    protected GlyphIds readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new GlyphIds(data, base, dataIsCanonical);
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
    protected VisibleSubTable.Builder<NumRecordTable> createSubTableBuilder() {
      return new NumRecordTable.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<NumRecordTable> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new NumRecordTable.Builder(data, 0, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<NumRecordTable> createSubTableBuilder(NumRecordTable subTable) {
      return new NumRecordTable.Builder(subTable);
    }
  }
}
