package com.google.typography.font.sfntly.table.opentype.chaincontextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.OffsetRecordTable;

public abstract class ChainSubGenericRuleSet<T extends ChainSubGenericRule>
    extends OffsetRecordTable<T> {
  protected ChainSubGenericRuleSet(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  @Override
  public int fieldCount() {
    return 0;
  }

  static abstract class Builder<
      T extends ChainSubGenericRuleSet<S>, S extends ChainSubGenericRule>
      extends OffsetRecordTable.Builder<T, S> {

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    protected Builder() {
      super();
    }

    protected Builder(T table) {
      super(table);
    }

    @Override
    protected void initFields() {
    }

    @Override
    public int fieldCount() {
      return 0;
    }
  }
}
