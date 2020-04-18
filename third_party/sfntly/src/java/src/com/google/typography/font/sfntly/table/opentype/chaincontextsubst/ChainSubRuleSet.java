package com.google.typography.font.sfntly.table.opentype.chaincontextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class ChainSubRuleSet extends ChainSubGenericRuleSet<ChainSubRule> {
  ChainSubRuleSet(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  @Override
  protected ChainSubRule readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new ChainSubRule(data, base, dataIsCanonical);
  }

  static class Builder
      extends ChainSubGenericRuleSet.Builder<ChainSubRuleSet, ChainSubRule> {

    Builder() {
      super();
    }

    Builder(ChainSubRuleSet table) {
      super(table);
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    @Override
    protected ChainSubRuleSet readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new ChainSubRuleSet(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRule> createSubTableBuilder() {
      return new ChainSubRule.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRule> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new ChainSubRule.Builder(data, 0, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<ChainSubRule> createSubTableBuilder(ChainSubRule subTable) {
      return new ChainSubRule.Builder(subTable);
    }
  }
}
