package com.google.typography.font.sfntly.table.opentype.chaincontextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.component.OffsetRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class ChainSubRuleSetArray extends OffsetRecordTable<ChainSubRuleSet> {
  private static final int FIELD_COUNT = 1;

  private static final int COVERAGE_INDEX = 0;
  private static final int COVERAGE_DEFAULT = 0;

  public final CoverageTable coverage;

  public ChainSubRuleSetArray(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    int coverageOffset = getField(COVERAGE_INDEX);
    coverage = new CoverageTable(data.slice(coverageOffset), 0, dataIsCanonical);
  }

  @Override
  public ChainSubRuleSet readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new ChainSubRuleSet(data, 0, dataIsCanonical);
  }

  public static class Builder
      extends OffsetRecordTable.Builder<ChainSubRuleSetArray, ChainSubRuleSet> {

    public Builder() {
      super();
    }

    public Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    public Builder(ChainSubRuleSetArray table) {
      super(table);
    }

    @Override
    protected ChainSubRuleSetArray readTable(
        ReadableFontData data, int base, boolean dataIsCanonical) {
      return new ChainSubRuleSetArray(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRuleSet> createSubTableBuilder() {
      return new ChainSubRuleSet.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRuleSet> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new ChainSubRuleSet.Builder(data, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRuleSet> createSubTableBuilder(
        ChainSubRuleSet subTable) {
      return new ChainSubRuleSet.Builder(subTable);
    }

    @Override
    protected void initFields() {
      setField(COVERAGE_INDEX, COVERAGE_DEFAULT);
    }

    @Override
    public int fieldCount() {
      return FIELD_COUNT;
    }
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }
}
