package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.opentype.component.GsubLookupType;
import com.google.typography.font.sfntly.table.opentype.component.OffsetRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;

public class LookupTable extends OffsetRecordTable<SubstSubtable> {
  private static final int FIELD_COUNT = 2;

  static final int LOOKUP_TYPE_INDEX = 0;
  private static final int LOOKUP_TYPE_DEFAULT = 0;

  private static final int LOOKUP_FLAG_INDEX = 1;

  private enum LookupFlagBit {
    RIGHT_TO_LEFT(0x0001),
    IGNORE_BASE_GLYPHS(0x0002),
    IGNORE_LIGATURES(0x0004),
    IGNORE_MARKS(0x0008),
    USE_MARK_FILTERING_SET(0x0010),
    RESERVED(0x00E0),
    MARK_ATTACHMENT_TYPE(0xFF00);

    private int bit;

    private LookupFlagBit(int bit) {
      this.bit = bit;
    }

    private int getValue(int value) {
      return bit & value;
    }
  }

  protected LookupTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    int lookupFlag = getField(LOOKUP_FLAG_INDEX);
    if (LookupFlagBit.USE_MARK_FILTERING_SET.getValue(lookupFlag) != 0) {
      throw new IllegalArgumentException(
          "Lookup Flag has Use Mark Filtering Set which is unimplemented.");
    }
    if (LookupFlagBit.RESERVED.getValue(lookupFlag) != 0) {
      throw new IllegalArgumentException("Reserved bits of Lookup Flag are not 0");
    }
  }

  public GsubLookupType lookupType() {
    return GsubLookupType.forTypeNum(getField(LOOKUP_TYPE_INDEX));
  }

  public GsubLookupType lookupFlag() {
    return GsubLookupType.forTypeNum(getField(LOOKUP_FLAG_INDEX));
  }

  @Override
  protected SubstSubtable readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    int lookupType = getField(LOOKUP_TYPE_INDEX);
    GsubLookupType gsubLookupType = GsubLookupType.forTypeNum(lookupType);
    switch (gsubLookupType) {
    case GSUB_LIGATURE:
      return new LigatureSubst(data, base, dataIsCanonical);
    case GSUB_SINGLE:
      return new SingleSubst(data, base, dataIsCanonical);
    case GSUB_MULTIPLE:
      return new MultipleSubst(data, base, dataIsCanonical);
    case GSUB_ALTERNATE:
      return new AlternateSubst(data, base, dataIsCanonical);
    case GSUB_CONTEXTUAL:
      return new ContextSubst(data, base, dataIsCanonical);
    case GSUB_CHAINING_CONTEXTUAL:
      return new ChainContextSubst(data, base, dataIsCanonical);
    case GSUB_EXTENSION:
      return new ExtensionSubst(data, base, dataIsCanonical);
    case GSUB_REVERSE_CHAINING_CONTEXTUAL_SINGLE:
      return new ReverseChainSingleSubst(data, base, dataIsCanonical);
    default:
      System.err.println("Unimplemented LookupType: " + gsubLookupType);
      return new NullTable(data, base, dataIsCanonical);
      // throw new IllegalArgumentException("LookupType is " + lookupType);
    }
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  Builder builder() {
    return new Builder();
  }

  public static class Builder extends OffsetRecordTable.Builder<LookupTable, SubstSubtable> {
    Builder() {
      super();
    }

    Builder(ReadableFontData data, boolean dataIsCanonical) {
      this(data, 0, dataIsCanonical);
    }

    private Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data, base, dataIsCanonical);
    }

    Builder(LookupTable table) {
      super(table);
    }

    @Override
    protected LookupTable readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new LookupTable(data, base, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubstSubtable> createSubTableBuilder() {
      return new LigatureSubst.Builder();
    }

    @Override
    protected VisibleSubTable.Builder<SubstSubtable> createSubTableBuilder(
        ReadableFontData data, boolean dataIsCanonical) {
      return new LigatureSubst.Builder(data, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<SubstSubtable> createSubTableBuilder(SubstSubtable subTable) {
      return new LigatureSubst.Builder(subTable);
    }

    @Override
    public int fieldCount() {
      return FIELD_COUNT;
    }

    @Override
    public void initFields() {
      setField(LOOKUP_TYPE_INDEX, LOOKUP_TYPE_DEFAULT);
      setField(LOOKUP_FLAG_INDEX, LOOKUP_FLAG_INDEX);
    }
  }
}
