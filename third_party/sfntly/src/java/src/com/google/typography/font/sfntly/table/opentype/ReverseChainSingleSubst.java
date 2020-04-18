package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.CoverageArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.InnerArraysFmt3;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class ReverseChainSingleSubst extends SubstSubtable {
  private static final int FIELD_COUNT = 1;
  private static final int COVERAGE_INDEX = SubstSubtable.FIELD_SIZE;
  public final CoverageTable coverage;
  public final CoverageArray backtrackGlyphs;
  public final CoverageArray lookAheadGlyphs;
  public final NumRecordTable substitutes;

  // //////////////
  // Constructors

  ReverseChainSingleSubst(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    if (format != 1) {
      throw new IllegalStateException("Subt format value is " + format + " (should be 1).");
    }
    int coverageOffset = getField(COVERAGE_INDEX);
    coverage = new CoverageTable(data.slice(coverageOffset), 0, dataIsCanonical);

    NumRecordList records = new NumRecordList(data, 0, headerSize());
    backtrackGlyphs = new CoverageArray(records);

    records = new NumRecordList(data, 0, records.limit());
    lookAheadGlyphs = new CoverageArray(records);

    records = new NumRecordList(data, 0, records.limit());
    substitutes = new NumRecordTable(records);
  }

  @Override
  public int fieldCount() {
    return super.fieldCount() + FIELD_COUNT;
  }

  public static class Builder extends VisibleSubTable.Builder<ReverseChainSingleSubst> {
    private CoverageTable.Builder coverageBuilder;
    private CoverageArray.Builder backtrackGlyphsBuilder;
    private CoverageArray.Builder lookAheadGlyphsBuilder;

    protected Builder() {
      super();
    }

    protected Builder(InnerArraysFmt3 table) {
      this(table.readFontData(), 0, false);
    }

    protected Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data);
      if (!dataIsCanonical) {
        prepareToEdit();
      }
    }

    protected Builder(Builder other) {
      super();
      coverageBuilder = other.coverageBuilder;
      backtrackGlyphsBuilder = other.backtrackGlyphsBuilder;
      lookAheadGlyphsBuilder = other.lookAheadGlyphsBuilder;
    }

    @Override
    public int subDataSizeToSerialize() {
      if (lookAheadGlyphsBuilder != null) {
        serializedLength = lookAheadGlyphsBuilder.limit();
      } else {
        computeSizeFromData(internalReadData());
      }
      return serializedLength;
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      if (serializedLength == 0) {
        return 0;
      }

      if (coverageBuilder == null
          || backtrackGlyphsBuilder == null || lookAheadGlyphsBuilder == null) {
        return serializeFromData(newData);
      }

      int tableOnlySize = 0;
      tableOnlySize += coverageBuilder.headerSize();
      tableOnlySize += backtrackGlyphsBuilder.tableSizeToSerialize();
      tableOnlySize += lookAheadGlyphsBuilder.tableSizeToSerialize();
      int subTableWriteOffset = tableOnlySize;

      coverageBuilder.subSerialize(newData);

      backtrackGlyphsBuilder.subSerialize(newData, subTableWriteOffset);
      subTableWriteOffset += backtrackGlyphsBuilder.subTableSizeToSerialize();
      int tableWriteOffset = backtrackGlyphsBuilder.tableSizeToSerialize();

      lookAheadGlyphsBuilder.subSerialize(newData.slice(tableWriteOffset), subTableWriteOffset);
      subTableWriteOffset += lookAheadGlyphsBuilder.subTableSizeToSerialize();

      return subTableWriteOffset;
    }

    @Override
    public ReverseChainSingleSubst subBuildTable(ReadableFontData data) {
      return new ReverseChainSingleSubst(data, 0, true);
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      backtrackGlyphsBuilder = null;
      lookAheadGlyphsBuilder = null;
    }

    // ////////////////////////////////////
    // private methods

    private void prepareToEdit() {
      initFromData(internalReadData());
      setModelChanged();
    }

    private void initFromData(ReadableFontData data) {
      if (backtrackGlyphsBuilder == null
          || lookAheadGlyphsBuilder == null) {
        NumRecordList records = new NumRecordList(data);
        backtrackGlyphsBuilder = new CoverageArray.Builder(records);

        records = new NumRecordList(data, 0, records.limit());
        lookAheadGlyphsBuilder = new CoverageArray.Builder(records);
      }
    }

    private void computeSizeFromData(ReadableFontData data) {
      // This assumes canonical data.
      int len = 0;
      if (data != null) {
        len = data.length();
      }
      serializedLength = len;
    }

    private int serializeFromData(WritableFontData newData) {
      // The source data must be canonical.
      ReadableFontData data = internalReadData();
      data.copyTo(newData);
      return data.length();
    }
  }
}
