package com.google.typography.font.sfntly.table.opentype.contextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class SubRuleSet extends SubGenericRuleSet<SubRule> {
  SubRuleSet(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  @Override
  protected SubRule readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new SubRule(data, base, dataIsCanonical);
  }

  static class Builder extends SubGenericRuleSet.Builder<SubRuleSet, SubRule> {
    Builder() {
      super();
    }

    Builder(SubRuleSet table) {
      super(table);
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    @Override
    protected SubRuleSet readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new SubRuleSet(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubRule> createSubTableBuilder() {
      return new SubRule.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<SubRule> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new SubRule.Builder(data, 0, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubRule> createSubTableBuilder(SubRule subTable) {
      return new SubRule.Builder(subTable);
    }
  }
}
