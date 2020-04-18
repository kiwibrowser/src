package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.singlesubst.HeaderFmt1;
import com.google.typography.font.sfntly.table.opentype.singlesubst.InnerArrayFmt2;

public class SingleSubst extends SubstSubtable {
  private final HeaderFmt1 fmt1;
  private final InnerArrayFmt2 fmt2;

  // //////////////
  // Constructors

  SingleSubst(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    switch (format) {
    case 1:
      fmt1 = new HeaderFmt1(data, headerSize(), dataIsCanonical);
      fmt2 = null;
      break;
    case 2:
      fmt1 = null;
      fmt2 = new InnerArrayFmt2(data, headerSize(), dataIsCanonical);
      break;
    default:
      throw new IllegalStateException("Subt format value is " + format + " (should be 1 or 2).");
    }
  }

  // //////////////////////////////////
  // Methods specific to this class

  public CoverageTable coverage() {
    switch (format) {
    case 1:
      return fmt1.coverage;
    case 2:
      return fmt2.coverage;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public HeaderFmt1 fmt1Table() {
    if (format == 1) {
      return fmt1;
    }
    throw new IllegalArgumentException("unexpected format table requested: " + format);
  }

  public InnerArrayFmt2 fmt2Table() {
    if (format == 2) {
      return fmt2;
    }
    throw new IllegalArgumentException("unexpected format table requested: " + format);
  }

  public static class Builder extends SubstSubtable.Builder<SubstSubtable> {

    private final HeaderFmt1.Builder fmt1Builder;
    private final InnerArrayFmt2.Builder fmt2Builder;

    protected Builder() {
      super();
      fmt1Builder = new HeaderFmt1.Builder();
      fmt2Builder = new InnerArrayFmt2.Builder();
    }

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
      fmt1Builder = new HeaderFmt1.Builder(data, dataIsCanonical);
      fmt2Builder = new InnerArrayFmt2.Builder(data, dataIsCanonical);
    }

    protected Builder(SubstSubtable subTable) {
      SingleSubst ligSubst = (SingleSubst) subTable;
      fmt1Builder = new HeaderFmt1.Builder(ligSubst.fmt1);
      fmt2Builder = new InnerArrayFmt2.Builder(ligSubst.fmt2);
    }

    @Override
    public int subDataSizeToSerialize() {
      return fmt1Builder.subDataSizeToSerialize() + fmt2Builder.subDataSizeToSerialize();
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      int byteCount = fmt1Builder.subSerialize(newData);
      byteCount += fmt2Builder.subSerialize(newData.slice(byteCount));
      return byteCount;
    }

    // /////////////////////////////////
    // must implement abstract methods

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      fmt1Builder.subDataSet();
      fmt2Builder.subDataSet();
    }

    @Override
    public SingleSubst subBuildTable(ReadableFontData data) {
      return new SingleSubst(data, 0, true);
    }
  }
}
