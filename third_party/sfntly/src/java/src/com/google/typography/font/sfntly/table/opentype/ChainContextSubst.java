package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassSetArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubGenericRuleSet;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRuleSetArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.InnerArraysFmt3;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordList;

public class ChainContextSubst extends SubstSubtable {
  private final ChainSubRuleSetArray ruleSets;
  private final ChainSubClassSetArray classSets;
  public final InnerArraysFmt3 fmt3Array;

  // //////////////
  // Constructors

  ChainContextSubst(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
    switch (format) {
    case 1:
      ruleSets = new ChainSubRuleSetArray(data, headerSize(), dataIsCanonical);
      classSets = null;
      fmt3Array = null;
      break;
    case 2:
      ruleSets = null;
      classSets = new ChainSubClassSetArray(data, headerSize(), dataIsCanonical);
      fmt3Array = null;
      break;
    case 3:
      ruleSets = null;
      classSets = null;
      fmt3Array = new InnerArraysFmt3(data, headerSize(), dataIsCanonical);
      break;
    default:
      throw new IllegalStateException("Subt format value is " + format + " (should be 1 or 2).");
    }
  }

  // //////////////////////////////////
  // Methods redirected to the array

  public ChainSubRuleSetArray fmt1Table() {
    switch (format) {
    case 1:
      return ruleSets;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public ChainSubClassSetArray fmt2Table() {
    switch (format) {
    case 2:
      return classSets;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public InnerArraysFmt3 fmt3Table() {
    switch (format) {
    case 3:
      return fmt3Array;
    default:
      throw new IllegalArgumentException("unexpected format table requested: " + format);
    }
  }

  public NumRecordList recordList() {
    switch (format) {
    case 1:
      return ruleSets.recordList;
    case 2:
      return classSets.recordList;
    default:
      return null;
    }
  }

  public ChainSubGenericRuleSet<?> subTableAt(int index) {
    switch (format) {
    case 1:
      return ruleSets.subTableAt(index);
    case 2:
      return classSets.subTableAt(index);
    default:
      return null;
    }
  }



  // //////////////////////////////////
  // Methods specific to this class

  public CoverageTable coverage() {
    switch (format) {
    case 1:
      return ruleSets.coverage;
    case 2:
      return classSets.coverage;
    default:
      return null;
    }
  }

  public ClassDefTable backtrackClassDef() {
    return (format == 2) ? classSets.backtrackClassDef : null;
  }

  public ClassDefTable inputClassDef() {
    return (format == 2) ? classSets.inputClassDef : null;
  }

  public ClassDefTable lookAheadClassDef() {
    return (format == 2) ? classSets.lookAheadClassDef : null;
  }

  protected static class Builder extends SubstSubtable.Builder<SubstSubtable> {
    private final ChainSubRuleSetArray.Builder arrayBuilder;

    protected Builder() {
      super();
      arrayBuilder = new ChainSubRuleSetArray.Builder();
    }

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
      arrayBuilder = new ChainSubRuleSetArray.Builder(data, dataIsCanonical);
    }

    protected Builder(SubstSubtable subTable) {
      ChainContextSubst ligSubst = (ChainContextSubst) subTable;
      arrayBuilder = new ChainSubRuleSetArray.Builder(ligSubst.ruleSets);
    }

    // ///////////////////////////////
    // Public methods to serialize

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
    public ChainContextSubst subBuildTable(ReadableFontData data) {
      return new ChainContextSubst(data, 0, true);
    }
  }
}
