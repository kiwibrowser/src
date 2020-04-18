package com.google.typography.font.sfntly.table.opentype.contextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class SubClassSet extends SubGenericRuleSet<SubClassRule> {
  SubClassSet(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  @Override
  protected SubClassRule readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new SubClassRule(data, base, dataIsCanonical);
  }

  static class Builder extends SubGenericRuleSet.Builder<SubClassSet, SubClassRule> {
    Builder() {
      super();
    }

    Builder(SubClassSet table) {
      super(table);
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    @Override
    protected SubClassSet readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new SubClassSet(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubClassRule> createSubTableBuilder() {
      return new SubClassRule.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<SubClassRule> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new SubClassRule.Builder(data, 0, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubClassRule> createSubTableBuilder(SubClassRule subTable) {
      return new SubClassRule.Builder(subTable);
    }
  }
}
