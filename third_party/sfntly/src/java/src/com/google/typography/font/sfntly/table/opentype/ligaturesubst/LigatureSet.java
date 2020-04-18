package com.google.typography.font.sfntly.table.opentype.ligaturesubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.OffsetRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class LigatureSet extends OffsetRecordTable<Ligature> {
  LigatureSet(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  static class Builder extends OffsetRecordTable.Builder<LigatureSet, Ligature> {
    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    Builder() {
      super();
    }

    Builder(LigatureSet table) {
      super(table);
    }

    @Override
    protected LigatureSet readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new LigatureSet(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<Ligature> createSubTableBuilder() {
      return new Ligature.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<Ligature> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new Ligature.Builder(data, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<Ligature> createSubTableBuilder(Ligature subTable) {
      return new Ligature.Builder(subTable);
    }

    @Override
    protected void initFields() {
    }

    @Override
    protected int fieldCount() {
      return 0;
    }
  }

  @Override
  protected Ligature readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new Ligature(data, base, dataIsCanonical);
  }

  @Override
  public int fieldCount() {
    return 0;
  }
}
