// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.table.opentype.contextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.component.SubstLookupRecordList;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class DoubleRecordTable extends SubTable {
  public final NumRecordList inputGlyphs;
  public final SubstLookupRecordList lookupRecords;

  // ///////////////
  // constructors

  public DoubleRecordTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data);
    inputGlyphs = new NumRecordList(data, 1, base, base + 4);
    lookupRecords = new SubstLookupRecordList(data, base + 2, inputGlyphs.limit());
  }

  public DoubleRecordTable(ReadableFontData data, boolean dataIsCanonical) {
    this(data, 0, dataIsCanonical);
  }

  public abstract static class Builder<T extends DoubleRecordTable> extends VisibleSubTable.Builder<T> {
    protected NumRecordList inputGlyphIdsBuilder;
    protected SubstLookupRecordList substLookupRecordsBuilder;
    protected int serializedLength;

    public Builder() {
      super();
    }

    public Builder(DoubleRecordTable table) {
      this(table.readFontData(), 0, false);
    }

    public Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data);
      if (!dataIsCanonical) {
        prepareToEdit();
      }
    }

    public Builder(Builder<T> other) {
      super();
      inputGlyphIdsBuilder = other.inputGlyphIdsBuilder;
      substLookupRecordsBuilder = other.substLookupRecordsBuilder;
    }

    @Override
    public int subDataSizeToSerialize() {
      if (substLookupRecordsBuilder != null) {
        serializedLength = substLookupRecordsBuilder.limit();
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

      if (inputGlyphIdsBuilder == null || substLookupRecordsBuilder == null) {
        return serializeFromData(newData);
      }

      return inputGlyphIdsBuilder.writeTo(newData) + substLookupRecordsBuilder.writeTo(newData);
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      inputGlyphIdsBuilder = null;
      substLookupRecordsBuilder = null;
    }

    // ////////////////////////////////////
    // private methods

    private void prepareToEdit() {
      initFromData(internalReadData());
      setModelChanged();
    }

    private void initFromData(ReadableFontData data) {
      if (inputGlyphIdsBuilder == null || substLookupRecordsBuilder == null) {
        inputGlyphIdsBuilder = new NumRecordList(data, 1, 0, 4);
        substLookupRecordsBuilder = new SubstLookupRecordList(
            data, 2, inputGlyphIdsBuilder.limit());
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
