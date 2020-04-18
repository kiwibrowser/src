package com.google.typography.font.sfntly.table.opentype.chaincontextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.SubstLookupRecordList;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class ChainSubGenericRule extends SubTable {
  public final NumRecordList backtrackGlyphs;
  public final NumRecordList inputClasses;
  public final NumRecordList lookAheadGlyphs;
  public final SubstLookupRecordList lookupRecords;

  // //////////////
  // Constructors

  protected ChainSubGenericRule(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data);
    backtrackGlyphs = new NumRecordList(data);
    inputClasses = new NumRecordList(data, 1, backtrackGlyphs.limit());
    lookAheadGlyphs = new NumRecordList(data, 0, inputClasses.limit());
    lookupRecords = new SubstLookupRecordList(
        data, lookAheadGlyphs.limit(), lookAheadGlyphs.limit() + 2);
  }

  abstract static class Builder<T extends ChainSubGenericRule> extends
      VisibleSubTable.Builder<T> {
    private NumRecordList backtrackGlyphsBuilder;
    private NumRecordList inputGlyphsBuilder;
    private NumRecordList lookAheadGlyphsBuilder;
    private SubstLookupRecordList lookupRecordsBuilder;

    protected Builder() {
      super();
    }

    protected Builder(ChainSubGenericRule table) {
      this(table.readFontData(), 0, false);
    }

    protected Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data);
      if (!dataIsCanonical) {
        prepareToEdit();
      }
    }

    @Override
    public int subDataSizeToSerialize() {
      if (lookupRecordsBuilder != null) {
        serializedLength = lookupRecordsBuilder.limit();
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

      if (backtrackGlyphsBuilder == null || inputGlyphsBuilder == null
          || lookAheadGlyphsBuilder == null || lookupRecordsBuilder == null) {
        return serializeFromData(newData);
      }

      return backtrackGlyphsBuilder.writeTo(newData) + inputGlyphsBuilder.writeTo(newData)
          + lookAheadGlyphsBuilder.writeTo(newData) + lookupRecordsBuilder.writeTo(newData);
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      backtrackGlyphsBuilder = null;
      inputGlyphsBuilder = null;
      lookupRecordsBuilder = null;
      lookAheadGlyphsBuilder = null;
    }

    // ////////////////////////////////////
    // private methods

    private void prepareToEdit() {
      initFromData(internalReadData());
      setModelChanged();
    }

    private void initFromData(ReadableFontData data) {
      if (backtrackGlyphsBuilder == null || inputGlyphsBuilder == null
          || lookAheadGlyphsBuilder == null || lookupRecordsBuilder == null) {
        backtrackGlyphsBuilder = new NumRecordList(data);
        inputGlyphsBuilder = new NumRecordList(data, 0, backtrackGlyphsBuilder.limit());
        lookAheadGlyphsBuilder = new NumRecordList(data, 0, inputGlyphsBuilder.limit());
        lookupRecordsBuilder = new SubstLookupRecordList(data, lookAheadGlyphsBuilder.limit());
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
