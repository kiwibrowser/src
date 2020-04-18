package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.opentype.component.TagOffsetsTable;
import com.google.typography.font.sfntly.table.opentype.component.VisibleSubTable;
import java.util.HashMap;
import java.util.Map;

public class ScriptTable extends TagOffsetsTable<LangSysTable> {
  private static final int FIELD_COUNT = 1;

  private static final int DEFAULT_LANG_SYS_INDEX = 0;
  private static final int NO_DEFAULT_LANG_SYS = 0;

  ScriptTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  public LangSysTable defaultLangSysTable() {
    int defaultLangSysOffset = getField(DEFAULT_LANG_SYS_INDEX);
    if (defaultLangSysOffset == NO_DEFAULT_LANG_SYS) {
      return null;
    }

    ReadableFontData newData = data.slice(defaultLangSysOffset);
    LangSysTable langSysTable = new LangSysTable(newData, dataIsCanonical);
    return langSysTable;
  }

  private LanguageTag langSysAt(int index) {
    return LanguageTag.fromTag(this.tagAt(index));
  }

  public Map<LanguageTag, LangSysTable> map() {
    Map<LanguageTag, LangSysTable> map = new HashMap<LanguageTag, LangSysTable>();
    LangSysTable defaultLangSys = defaultLangSysTable();
    if (defaultLangSys != null) {
      map.put(LanguageTag.DFLT, defaultLangSys);
    }
    for (int i = 0; i < count(); i++) {
      LanguageTag lang;
      try {
        lang = langSysAt(i);
      } catch (IllegalArgumentException e) {
        System.err.println("Invalid LangSys tag found: " + e.getMessage());
        continue;
      }
      map.put(lang, subTableAt(i));
    }
    return map;
  }

  @Override
  protected LangSysTable readSubTable(ReadableFontData data, boolean dataIsCanonical) {
    return new LangSysTable(data, dataIsCanonical);
  }

  @Override
  public int fieldCount() {
    return FIELD_COUNT;
  }

  static class Builder extends TagOffsetsTable.Builder<ScriptTable, LangSysTable> {
    private VisibleSubTable.Builder<LangSysTable> defLangSysBuilder;

    Builder() {
      super();
    }

    Builder(ReadableFontData data, int base, boolean dataIsCanonical) {
      super(data, base, dataIsCanonical);
      int defLangSys = getField(DEFAULT_LANG_SYS_INDEX);
      if (defLangSys != NO_DEFAULT_LANG_SYS) {
        defLangSysBuilder = new LangSysTable.Builder(data.slice(defLangSys), dataIsCanonical);
      }
    }

    @Override
    protected VisibleSubTable.Builder<LangSysTable> createSubTableBuilder(
        ReadableFontData data, int tag, boolean dataIsCanonical) {
      return new LangSysTable.Builder(data, dataIsCanonical);
    }

    @Override
    protected VisibleSubTable.Builder<LangSysTable> createSubTableBuilder() {
      return new LangSysTable.Builder();
    }

    @Override
    protected ScriptTable readTable(ReadableFontData data, int base, boolean dataIsCanonical) {
      return new ScriptTable(data, base, dataIsCanonical);
    }

    @Override
    public int subDataSizeToSerialize() {
      int size = super.subDataSizeToSerialize();
      if (defLangSysBuilder != null) {
        size += defLangSysBuilder.subDataSizeToSerialize();
      }
      return size;
    }

    @Override
    public int subSerialize(WritableFontData newData) {
      int byteCount = super.subSerialize(newData);
      if (defLangSysBuilder != null) {
        byteCount += defLangSysBuilder.subSerialize(newData.slice(byteCount));
      }
      return byteCount;
    }

    @Override
    public void subDataSet() {
      super.subDataSet();
      defLangSysBuilder = null;
    }

    @Override
    public int fieldCount() {
      return FIELD_COUNT;
    }

    @Override
    protected void initFields() {
      setField(DEFAULT_LANG_SYS_INDEX, NO_DEFAULT_LANG_SYS);
    }
  }
}
