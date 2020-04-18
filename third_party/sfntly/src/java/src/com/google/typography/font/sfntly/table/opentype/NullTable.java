package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public final class NullTable extends SubstSubtable {
  private static final int RECORD_SIZE = 0;

  NullTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  private NullTable(ReadableFontData data) {
    super(data, 0, false);
  }

  private NullTable() {
    super(null, 0, false);
  }

  public final static class Builder extends VisibleSubTable.Builder<NullTable> {
    private Builder() {
    }

    private Builder(ReadableFontData data, boolean dataIsCanonical) {
    }

    private Builder(NullTable table) {
    }

    @Override
    public int subDataSizeToSerialize() {
      return NullTable.RECORD_SIZE;
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      return NullTable.RECORD_SIZE;
    }

    @Override
    public NullTable subBuildTable(ReadableFontData data) {
      return new NullTable(data);
    }

    @Override
    public void subDataSet() {
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }
  }
}
