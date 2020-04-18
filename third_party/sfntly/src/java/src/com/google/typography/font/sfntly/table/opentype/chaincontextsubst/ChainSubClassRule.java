package com.google.typography.font.sfntly.table.opentype.chaincontextsubst;

import com.google.typography.font.sfntly.data.ReadableFontData;

public class ChainSubClassRule extends ChainSubGenericRule {
  ChainSubClassRule(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  static class Builder extends ChainSubGenericRule.Builder<ChainSubClassRule> {
    Builder() {
      super();
    }

    Builder(ChainSubClassRule table) {
      super(table);
    }

    Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data, base, dataIsCanonical);
    }

    @Override
    public ChainSubClassRule subBuildTable(ReadableFontData data) {
      return new ChainSubClassRule(data, 0, true);
    }

  }
}
