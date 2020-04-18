// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.sample.sfview;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.sample.sfview.TaggedData.FieldType;
import com.google.typography.font.sfntly.table.FontDataTable;
import com.google.typography.font.sfntly.table.opentype.AlternateSubst;
import com.google.typography.font.sfntly.table.opentype.ChainContextSubst;
import com.google.typography.font.sfntly.table.opentype.ClassDefTable;
import com.google.typography.font.sfntly.table.opentype.ContextSubst;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.ExtensionSubst;
import com.google.typography.font.sfntly.table.opentype.FeatureListTable;
import com.google.typography.font.sfntly.table.opentype.FeatureTable;
import com.google.typography.font.sfntly.table.opentype.GSubTable;
import com.google.typography.font.sfntly.table.opentype.LangSysTable;
import com.google.typography.font.sfntly.table.opentype.LigatureSubst;
import com.google.typography.font.sfntly.table.opentype.LookupListTable;
import com.google.typography.font.sfntly.table.opentype.LookupTable;
import com.google.typography.font.sfntly.table.opentype.MultipleSubst;
import com.google.typography.font.sfntly.table.opentype.NullTable;
import com.google.typography.font.sfntly.table.opentype.ReverseChainSingleSubst;
import com.google.typography.font.sfntly.table.opentype.ScriptListTable;
import com.google.typography.font.sfntly.table.opentype.ScriptTable;
import com.google.typography.font.sfntly.table.opentype.SingleSubst;
import com.google.typography.font.sfntly.table.opentype.SubstSubtable;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassRule;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassSet;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubGenericRuleSet;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRule;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRuleSet;
import com.google.typography.font.sfntly.table.opentype.classdef.InnerArrayFmt1;
import com.google.typography.font.sfntly.table.opentype.component.NumRecordTable;
import com.google.typography.font.sfntly.table.opentype.component.OneToManySubst;
import com.google.typography.font.sfntly.table.opentype.component.RangeRecordTable;
import com.google.typography.font.sfntly.table.opentype.contextsubst.DoubleRecordTable;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassRule;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassSet;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubGenericRuleSet;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRule;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRuleSet;
import com.google.typography.font.sfntly.table.opentype.ligaturesubst.Ligature;
import com.google.typography.font.sfntly.table.opentype.ligaturesubst.LigatureSet;
import com.google.typography.font.sfntly.table.opentype.singlesubst.HeaderFmt1;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
class OtTableTagger {
  private final TaggedData td;
  private final Map<Class<? extends FontDataTable>, TagMethod> tagMethodRegistry;

  OtTableTagger(TaggedData tdata) {
    this.td = tdata;
    this.tagMethodRegistry = new HashMap<Class<? extends FontDataTable>, TagMethod>();

    registerTagMethods();
  }

  void tag(GSubTable gsub) {
    if (gsub == null) {
      return;
    }

    tagTable(gsub.scriptList());
    tagTable(gsub.featureList());
    tagTable(gsub.lookupList());
  }

  private final List<String> tableCache = new ArrayList<String>();

  private void tagTable(FontDataTable table) {
    if (table == null) {
      return;
    }
    ReadableFontData data = table.readFontData();
    if (data == null) {
      return;
    }

    if (tableCache.contains(table.toString())) {
      return;
    }
    tableCache.add(table.toString());

    TagMethod tm = getTagMethod(table);
    if (tm == null) {
      td.pushRange(table.getClass().getSimpleName(), data);
    } else {
      td.pushRange(tm.tableLabel(table), data);
      tm.tag(table);
    }
    td.popRange();
  }

  abstract class TagMethod {
    private final Class<? extends FontDataTable> clzz;

    private TagMethod(Class<? extends FontDataTable> clzz) {
      this.clzz = clzz;
    }

    private String tableLabel(FontDataTable table) {
      Class<?> clzz = table.getClass();
      Class<?> encl = clzz.getEnclosingClass();
      if (encl == null) {
        return clzz.getSimpleName();
      }
      return encl.getSimpleName() + "." + clzz.getSimpleName();
    }

    protected abstract void tag(FontDataTable table);
  }

  private void register(TagMethod m) {
    tagMethodRegistry.put(m.clzz, m);
  }

  @SafeVarargs
  private final void register(TagMethod m, Class<? extends FontDataTable>... clzzes) {
    tagMethodRegistry.put(m.clzz, m);
    for (Class<? extends FontDataTable> clzz : clzzes) {
      tagMethodRegistry.put(clzz, m);
    }
  }

  void registerTagMethods() {
    register(new TagMethod(ScriptListTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ScriptListTable table = (ScriptListTable) fdt;
        int scriptCount = td.tagRangeField(FieldType.SHORT, "script count");
        for (int i = 0; i < scriptCount; ++i) {
          td.tagRangeField(FieldType.TAG, null);
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (ScriptTable st : table) {
          tagTable(st);
        }
      }
    });

    register(new TagMethod(ScriptTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ScriptTable table = (ScriptTable) fdt;
        td.tagRangeField(FieldType.OFFSET_NONZERO, "default lang sys");
        int langCount = td.tagRangeField(FieldType.SHORT, "language count");
        for (int i = 0; i < langCount; ++i) {
          td.tagRangeField(FieldType.TAG, null);
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (LangSysTable lst : table) {
          tagTable(lst);
        }
        tagTable(table.defaultLangSysTable());
      }
    });

    register(new TagMethod(LangSysTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        LangSysTable table = (LangSysTable) fdt;
        td.tagRangeField(FieldType.SHORT_IGNORED, "lookup order");
        td.tagRangeField(FieldType.SHORT_IGNORED_FFFF, "required feature");
        td.tagRangeField(FieldType.SHORT, "feature count");
        for (int i = 0; i < table.recordList.count(); ++i) {
          td.tagRangeField(FieldType.SHORT, null);
        }
      }
    });

    register(new TagMethod(FeatureListTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        FeatureListTable table = (FeatureListTable) fdt;
        int featureCount = td.tagRangeField(FieldType.SHORT, "feature count");
        for (int i = 0; i < featureCount; ++i) {
          td.tagRangeField(FieldType.TAG, "index: " + i);
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (FeatureTable ft : table) {
          tagTable(ft);
        }
      }
    });

    register(new TagMethod(FeatureTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        FeatureTable table = (FeatureTable) fdt;
        td.tagRangeField(FieldType.OFFSET_NONZERO, "feature params");
        td.tagRangeField(FieldType.SHORT, "lookup count");
        for (int i = 0; i < table.recordList.count(); ++i) {
          td.tagRangeField(FieldType.SHORT, null);
        }
      }
    });

    register(new TagMethod(LookupListTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        LookupListTable table = (LookupListTable) fdt;
        int lookupCount = td.tagRangeField(FieldType.SHORT, "lookup count");
        for (int i = 0; i < lookupCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, "index: " + i);
        }
        for (int i = 0; i < lookupCount; ++i) {
          LookupTable lookup = table.subTableAt(i);
          if (lookup != null) {
            tagTable(lookup);
          }
        }
      }
    });

    register(new TagMethod(LookupTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        LookupTable table = (LookupTable) fdt;
        td.tagRangeField(FieldType.SHORT, "lookup type");
        td.tagRangeField(FieldType.SHORT, "lookup flags");
        int subTableCount = td.tagRangeField(FieldType.SHORT, "subtable count");
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (int i = 0; i < subTableCount; ++i) {
          SubstSubtable subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    });

    register(new TagMethod(LigatureSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        LigatureSubst table = (LigatureSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "subst format");
        td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
        tagTable(table.coverage());
        td.tagRangeField(FieldType.SHORT, "subtable count");

        int subTableCount = table.subTableCount();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }

        for (int i = 0; i < subTableCount; ++i) {
          LigatureSet subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    });

    register(new TagMethod(LigatureSet.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        LigatureSet table = (LigatureSet) fdt;
        td.tagRangeField(FieldType.SHORT, "lookup count");
        for (int i = 0; i < table.recordList.count(); ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (int i = 0; i < table.recordList.count(); ++i) {
          Ligature lookup = table.subTableAt(i);
          if (lookup != null) {
            tagTable(lookup);
          }
        }
      }
    });

    register(new TagMethod(Ligature.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        Ligature table = (Ligature) fdt;
        td.tagRangeField(FieldType.GLYPH, "lig glyph");
        td.tagRangeField(FieldType.SHORT, "glyph count + 1");
        for (int i = 0; i < table.recordList.count(); ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }
      }
    });

    register(new TagMethod(SingleSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        SingleSubst table = (SingleSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "format");
        switch (table.format) {
        case 1:
          HeaderFmt1 tableFmt1 = table.fmt1Table();
          td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
          tagTable(tableFmt1.coverage);
          td.tagRangeField(FieldType.SHORT, "delta glyph id");
          break;
        case 2:
          com.google.typography.font.sfntly.table.opentype.singlesubst.InnerArrayFmt2 tableFmt2 =
              table.fmt2Table();
          td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
          tagTable(tableFmt2.coverage);
          td.tagRangeField(FieldType.SHORT, "glyph count");
          for (int i = 0; i < tableFmt2.recordList.count(); ++i) {
            td.tagRangeField(FieldType.GLYPH, null);
          }
          break;
        }
      }
    });

    register(new TagMethod(MultipleSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        OneToManySubst table = (OneToManySubst) fdt;
        td.tagRangeField(FieldType.SHORT, "subst format");
        td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
        tagTable(table.coverage());
        td.tagRangeField(FieldType.SHORT, "sequence count");

        int subTableCount = table.recordList().count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }

        for (int i = 0; i < subTableCount; ++i) {
          NumRecordTable subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    }, AlternateSubst.class);

    register(new TagMethod(NumRecordTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        NumRecordTable table = (NumRecordTable) fdt;
        td.tagRangeField(FieldType.SHORT, "glyph count");
        for (int i = 0; i < table.recordList.count(); ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }
      }
    });

    register(new TagMethod(ContextSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ContextSubst table = (ContextSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "subst format");
        td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
        tagTable(table.coverage());
        if (table.format == 2) {
          td.tagRangeField(FieldType.OFFSET_NONZERO, "class def offset");
          tagTable(table.classDef());
        }
        td.tagRangeField(FieldType.SHORT, "sub rule set count");

        int subTableCount = table.recordList().count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET_NONZERO, "for inital class: " + i);
        }
        for (int i = 0; i < subTableCount; ++i) {
          SubGenericRuleSet<?> subTable = table.subTableAt(i);
          if (subTable != null) {
            tagTable(subTable);
          }
        }
      }
    });

    register(new TagMethod(SubRuleSet.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        SubGenericRuleSet<?> table = (SubGenericRuleSet<?>) fdt;
        td.tagRangeField(FieldType.SHORT, "sub rule count");
        int subTableCount = table.recordList.count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (int i = 0; i < subTableCount; ++i) {
          DoubleRecordTable subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    }, SubClassSet.class);

    register(new TagMethod(SubRule.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        SubRule table = (SubRule) fdt;
        td.tagRangeField(FieldType.SHORT, "input glyph count + 1");
        td.tagRangeField(FieldType.SHORT, "subst lookup record count");
        int glyphCount = table.inputGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.GLYPH, "glyph id");
        }
        int lookupCount = table.lookupRecords.count();
        for (int i = 0; i < lookupCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "sequence index");
          td.tagRangeField(FieldType.SHORT, "lookup list index");
        }
      }
    });

    register(new TagMethod(SubClassRule.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        SubClassRule table = (SubClassRule) fdt;
        td.tagRangeField(FieldType.SHORT, "input class count + 1");
        td.tagRangeField(FieldType.SHORT, "subst lookup record count");
        int glyphCount = table.inputGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "class id");
        }
        int lookupCount = table.lookupRecords.count();
        for (int i = 0; i < lookupCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "sequence index");
          td.tagRangeField(FieldType.SHORT, "lookup list index");
        }
      }
    });

    register(new TagMethod(ChainContextSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ChainContextSubst table = (ChainContextSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "subst format");
        if (table.format == 1 || table.format == 2) {
          td.tagRangeField(FieldType.OFFSET_NONZERO, "coverage offset");
          tagTable(table.coverage());
          int subTableCount = table.recordList().count();
          if (table.format == 1) {
            td.tagRangeField(FieldType.SHORT, "chain sub rule set count");
          }
          if (table.format == 2) {
            td.tagRangeField(FieldType.OFFSET_NONZERO, "backtrack class def offset");
            tagTable(table.backtrackClassDef());
            td.tagRangeField(FieldType.OFFSET_NONZERO, "input class def offset");
            tagTable(table.inputClassDef());
            td.tagRangeField(FieldType.OFFSET_NONZERO, "look ahead class def offset");
            tagTable(table.lookAheadClassDef());
            td.tagRangeField(FieldType.SHORT, "chain sub class set count");
          }
          for (int i = 0; i < subTableCount; ++i) {
            td.tagRangeField(FieldType.OFFSET_NONZERO, null);
          }
          for (int i = 0; i < subTableCount; ++i) {
            ChainSubGenericRuleSet<?> subTable = table.subTableAt(i);
            if (subTable != null) {
              tagTable(subTable);
            }
          }
        }
        if (table.format == 3) {
          td.tagRangeField(FieldType.SHORT, "backtrackGlyphs coverage count");
          int subTableCount = table.fmt3Array.backtrackGlyphs.recordList.count();
          for (int i = 0; i < subTableCount; ++i) {
            td.tagRangeField(FieldType.OFFSET_NONZERO, null);
            CoverageTable subTable = table.fmt3Array.backtrackGlyphs.subTableAt(i);
            if (subTable != null) {
              tagTable(subTable);
            }
          }

          td.tagRangeField(FieldType.SHORT, "input glyphs coverage count");
          subTableCount = table.fmt3Array.inputGlyphs.recordList.count();
          for (int i = 0; i < subTableCount; ++i) {
            td.tagRangeField(FieldType.OFFSET_NONZERO, null);
            CoverageTable subTable = table.fmt3Array.inputGlyphs.subTableAt(i);
            if (subTable != null) {
              tagTable(subTable);
            }
          }

          td.tagRangeField(FieldType.SHORT, "lookahead glyphs coverage count");
          subTableCount = table.fmt3Array.lookAheadGlyphs.recordList.count();
          for (int i = 0; i < subTableCount; ++i) {
            td.tagRangeField(FieldType.OFFSET_NONZERO, null);
            CoverageTable subTable = table.fmt3Array.lookAheadGlyphs.subTableAt(i);
            if (subTable != null) {
              tagTable(subTable);
            }
          }

          td.tagRangeField(FieldType.SHORT, "subst lookup record count");
          int lookupCount = table.fmt3Array.lookupRecords.count();
          for (int i = 0; i < lookupCount; ++i) {
            td.tagRangeField(FieldType.SHORT, "sequence index");
            td.tagRangeField(FieldType.SHORT, "lookup list index");
          }
        }
      }
    });

    register(new TagMethod(ChainSubRuleSet.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ChainSubRuleSet table = (ChainSubRuleSet) fdt;
        td.tagRangeField(FieldType.SHORT, "sub rule count");
        int subTableCount = table.recordList.count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (int i = 0; i < subTableCount; ++i) {
          ChainSubRule subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    });

    register(new TagMethod(ChainSubRule.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ChainSubRule table = (ChainSubRule) fdt;
        td.tagRangeField(FieldType.SHORT, "backtrack glyph count");
        int glyphCount = table.backtrackGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }

        td.tagRangeField(FieldType.SHORT, "input glyph count");
        glyphCount = table.inputClasses.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }

        td.tagRangeField(FieldType.SHORT, "look ahead glyph count");
        glyphCount = table.lookAheadGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }

        td.tagRangeField(FieldType.SHORT, "subst lookup record count");
        int lookupCount = table.lookupRecords.count();
        for (int i = 0; i < lookupCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "sequence index");
          td.tagRangeField(FieldType.SHORT, "lookup list index");
        }
      }
    });

    register(new TagMethod(ChainSubClassSet.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ChainSubClassSet table = (ChainSubClassSet) fdt;
        td.tagRangeField(FieldType.SHORT, "sub class count");
        int subTableCount = table.recordList.count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET, null);
        }
        for (int i = 0; i < subTableCount; ++i) {
          ChainSubClassRule subTable = table.subTableAt(i);
          tagTable(subTable);
        }
      }
    });

    register(new TagMethod(ChainSubClassRule.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ChainSubClassRule table = (ChainSubClassRule) fdt;
        td.tagRangeField(FieldType.SHORT, "backtrack glyph class count");
        int glyphCount = table.backtrackGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "class id");
        }

        td.tagRangeField(FieldType.SHORT, "input glyph class count");
        glyphCount = table.inputClasses.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "class id");
        }

        td.tagRangeField(FieldType.SHORT, "look ahead glyph class count");
        glyphCount = table.lookAheadGlyphs.count();
        for (int i = 0; i < glyphCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "class id");
        }

        td.tagRangeField(FieldType.SHORT, "subst lookup record count");
        int lookupCount = table.lookupRecords.count();
        for (int i = 0; i < lookupCount; ++i) {
          td.tagRangeField(FieldType.SHORT, "sequence index");
          td.tagRangeField(FieldType.SHORT, "lookup list index");
        }
      }
    });

    register(new TagMethod(ExtensionSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ExtensionSubst table = (ExtensionSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "format");
        td.tagRangeField(FieldType.SHORT, "lookup type");
        td.tagRangeField(FieldType.OFFSET32, "lookup offset");
        SubstSubtable subTable = table.subTable();
        tagTable(subTable);
      }
    });

    register(new TagMethod(ReverseChainSingleSubst.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ReverseChainSingleSubst table = (ReverseChainSingleSubst) fdt;
        td.tagRangeField(FieldType.SHORT, "subst format");
        td.tagRangeField(FieldType.OFFSET_NONZERO, "input coverage offset");
        tagTable(table.coverage);

        td.tagRangeField(FieldType.SHORT, "backtrack glyphs coverages count");
        int subTableCount = table.backtrackGlyphs.recordList.count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET_NONZERO, null);
          CoverageTable subTable = table.backtrackGlyphs.subTableAt(i);
          if (subTable != null) {
            tagTable(subTable);
          }
        }

        td.tagRangeField(FieldType.SHORT, "lookahead glyphs coverages count");
        subTableCount = table.lookAheadGlyphs.recordList.count();
        for (int i = 0; i < subTableCount; ++i) {
          td.tagRangeField(FieldType.OFFSET_NONZERO, null);
          CoverageTable subTable = table.lookAheadGlyphs.subTableAt(i);
          if (subTable != null) {
            tagTable(subTable);
          }
        }

        td.tagRangeField(FieldType.SHORT, "subst glyph count");
        for (int i = 0; i < table.substitutes.recordList.count(); ++i) {
          td.tagRangeField(FieldType.GLYPH, null);
        }
      }
    });

    register(new TagMethod(CoverageTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        CoverageTable table = (CoverageTable) fdt;
        td.tagRangeField(FieldType.SHORT, "format");
        if (table.format == 1) {
          NumRecordTable tableFmt1 = (NumRecordTable) table.array;
          td.tagRangeField(FieldType.SHORT, "glyph count");
          for (int i = 0; i < tableFmt1.recordList.count(); ++i) {
            td.tagRangeField(FieldType.GLYPH, null);
          }
        }
        if (table.format == 2) {
          RangeRecordTable tableFmt2 = (RangeRecordTable) table.array;
          td.tagRangeField(FieldType.SHORT, "range count");
          for (int i = 0; i < tableFmt2.recordList.count(); ++i) {
            td.tagRangeField(FieldType.SHORT, "start");
            td.tagRangeField(FieldType.SHORT, "end");
            td.tagRangeField(FieldType.SHORT, "offset");
          }
        }
      }
    });

    register(new TagMethod(ClassDefTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
        ClassDefTable table = (ClassDefTable) fdt;
        td.tagRangeField(FieldType.SHORT, "format");
        if (table.format == 1) {
          InnerArrayFmt1 tableFmt1 = (InnerArrayFmt1) table.array;
          td.tagRangeField(FieldType.SHORT, "start glyph");
          td.tagRangeField(FieldType.SHORT, "glyph count");
          for (int i = 0; i < tableFmt1.recordList.count(); ++i) {
            td.tagRangeField(FieldType.SHORT, null);
          }
        }
        if (table.format == 2) {
          RangeRecordTable tableFmt2 = (RangeRecordTable) table.array;
          td.tagRangeField(FieldType.SHORT, "class range count");
          for (int i = 0; i < tableFmt2.recordList.count(); ++i) {
            td.tagRangeField(FieldType.SHORT, "start");
            td.tagRangeField(FieldType.SHORT, "end");
            td.tagRangeField(FieldType.SHORT, "class");
          }
        }
      }
    });

    register(new TagMethod(NullTable.class) {
      @Override
      protected
      void tag(FontDataTable fdt) {
      }
    });
  }

  private static final Comparator<Class<? extends FontDataTable>> CLASS_NAME_COMPARATOR =
      new Comparator<Class<? extends FontDataTable>>() {
    @Override
    public int compare(Class<? extends FontDataTable> o1, Class<? extends FontDataTable> o2) {
      return o1.getName().compareTo(o2.getName());
    }
  };

  private static Set<Class<? extends FontDataTable>>
  missedClasses = new TreeSet<Class<? extends FontDataTable>>(CLASS_NAME_COMPARATOR);

  private TagMethod getTagMethod(FontDataTable table) {
    Class<? extends FontDataTable> clzz = table.getClass();
    TagMethod tm = tagMethodRegistry.get(clzz);
    if (tm == null) {
      if (!missedClasses.contains(clzz)) {
        missedClasses.add(clzz);
        System.out.println("unregistered class: " + clzz.getName());
      }
    }
    return tm;
  }
}
