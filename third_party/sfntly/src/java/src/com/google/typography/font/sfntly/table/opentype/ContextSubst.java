package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;
import com.google.typography.font.sfntly.table.opentype.contextsubst.DoubleRecordTable;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassSetArray;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubGenericRuleSet;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRuleSetArray;

public class ContextSubst extends SubstSubtable {
  private final SubRuleSetArray ruleSets;
  private SubClassSetArray classSets;

  // //////////////
  // Constructors

  ContextSubst(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    switch (format) {
    case 1:
      ruleSets = new SubRuleSetArray(data, headerSize(), dataIsCanonical);
      classSets = null;
      break;
    case 2:
      ruleSets = null;
      classSets = new SubClassSetArray(data, headerSize(), dataIsCanonical);
      break;
    default:
      throw new IllegalStateException("Subt format value is " + format + " (should be 1 or 2).");
    }
  }

  // //////////////////////////////////
  // Methods redirected to the array

  public SubRuleSetArray fmt1Table() {
    switch (format) {
    case 1:
      return ruleSets;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public SubClassSetArray fmt2Table() {
    switch (format) {
    case 2:
      return classSets;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public NumRecordList recordList() {
    return (format == 1) ? ruleSets.recordList : classSets.recordList;
  }

  public SubGenericRuleSet<? extends DoubleRecordTable> subTableAt(int index) {
    return (format == 1) ? ruleSets.subTableAt(index) : classSets.subTableAt(index);
  }

  // //////////////////////////////////
  // Methods specific to this class

  public CoverageTable coverage() {
    return (format == 1) ? ruleSets.coverage : classSets.coverage;
  }

  public ClassDefTable classDef() {
    return (format == 2) ? classSets.classDef : null;
  }

  public static class Builder extends SubstSubtable.Builder<SubstSubtable> {
    private final SubRuleSetArray.Builder arrayBuilder;

    protected Builder() {
      super();
      arrayBuilder = new SubRuleSetArray.Builder();
    }

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
      arrayBuilder = new SubRuleSetArray.Builder(data, dataIsCanonical);
    }

    protected Builder(SubstSubtable subTable) {
      ContextSubst ligSubst = (ContextSubst) subTable;
      arrayBuilder = new SubRuleSetArray.Builder(ligSubst.ruleSets);
    }

    /**
     * Even though public, not to be used by the end users. Made public only
     * make it available to packages under
     * {@code com.google.typography.font.sfntly.table.opentype}.
     */
    @Override
    public int subDataSizeToSerialize() {
      return arrayBuilder.subDataSizeToSerialize();
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      return arrayBuilder.subSerialize(newData);
    }

    // /////////////////////////////////
    // must implement abstract methods

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    public void subDataSet() {
      arrayBuilder.subDataSet();
    }

    @Override
    public ContextSubst subBuildTable(ReadableFontData data) {
      return new ContextSubst(data, 0, true);
    }
  }
}
