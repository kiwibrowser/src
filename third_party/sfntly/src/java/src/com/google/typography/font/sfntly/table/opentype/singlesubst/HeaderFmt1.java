package com.google.typography.font.sfntly.table.opentype.singlesubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.component.HeaderTable;

public class HeaderFmt1 extends HeaderTable {
  private static final int FIELD_COUNT = 2;

  private static final int COVERAGE_INDEX = 0;
  private static final int COVERAGE_DEFAULT = 0;

  private static final int DELTA_GLYPH_ID_INDEX = 1;
  private static final int DELTA_GLYPH_ID_DEFAULT = 0;

  public final CoverageTable coverage;

  public HeaderFmt1(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    int coverageOffset = getField(COVERAGE_INDEX);
    coverage = new CoverageTable(data.slice(coverageOffset), 0, dataIsCanonical);
  }

  public int getDelta() {
    int delta = getField(DELTA_GLYPH_ID_INDEX);
    if (delta > 0x7FFF) {
      // Converting read unsigned int to signed short
      return (short) delta;
    }
    return delta;
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  public static class Builder extends HeaderTable.Builder<HeaderFmt1> {
    public Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    public Builder(HeaderFmt1 table) {
      super(table);
    }

    public Builder() {
      super();
    }

    @Override
    protected void initFields() {
      setField(COVERAGE_INDEX, COVERAGE_DEFAULT);
      setField(DELTA_GLYPH_ID_INDEX, DELTA_GLYPH_ID_DEFAULT);
    }

    @Override
    protected int fieldCount() {
      return FIELD_COUNT;
    }

    @Override
    protected HeaderFmt1 subBuildTable(ReadableFontData data) {
      return new HeaderFmt1(data, 0, false);
    }
  }
}
