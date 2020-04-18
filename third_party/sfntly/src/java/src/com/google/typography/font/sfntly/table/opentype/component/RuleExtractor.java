package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.table.opentype.AlternateSubst;
import com.google.typography.font.sfntly.table.opentype.ChainContextSubst;
import com.google.typography.font.sfntly.table.opentype.ClassDefTable;
import com.google.typography.font.sfntly.table.opentype.ContextSubst;
import com.google.typography.font.sfntly.table.opentype.CoverageTable;
import com.google.typography.font.sfntly.table.opentype.ExtensionSubst;
import com.google.typography.font.sfntly.table.opentype.LigatureSubst;
import com.google.typography.font.sfntly.table.opentype.LookupListTable;
import com.google.typography.font.sfntly.table.opentype.LookupTable;
import com.google.typography.font.sfntly.table.opentype.MultipleSubst;
import com.google.typography.font.sfntly.table.opentype.ReverseChainSingleSubst;
import com.google.typography.font.sfntly.table.opentype.SingleSubst;
import com.google.typography.font.sfntly.table.opentype.SubstSubtable;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassRule;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassSet;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubClassSetArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRule;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRuleSet;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.ChainSubRuleSetArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.CoverageArray;
import com.google.typography.font.sfntly.table.opentype.chaincontextsubst.InnerArraysFmt3;
import com.google.typography.font.sfntly.table.opentype.classdef.InnerArrayFmt1;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassRule;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassSet;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubClassSetArray;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRule;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRuleSet;
import com.google.typography.font.sfntly.table.opentype.contextsubst.SubRuleSetArray;
import com.google.typography.font.sfntly.table.opentype.ligaturesubst.Ligature;
import com.google.typography.font.sfntly.table.opentype.ligaturesubst.LigatureSet;
import com.google.typography.font.sfntly.table.opentype.singlesubst.HeaderFmt1;
import com.google.typography.font.sfntly.table.opentype.singlesubst.InnerArrayFmt2;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

class RuleExtractor {
  private static Set<Rule> extract(LigatureSubst table) {
    Set<Rule> allRules = new LinkedHashSet<Rule>();
    List<Integer> prefixChars = extract(table.coverage());

    for (int i = 0; i < table.subTableCount(); i++) {
      List<Rule> subRules = extract(table.subTableAt(i));
      subRules = Rule.prependToInput(prefixChars.get(i), subRules);
      allRules.addAll(subRules);
    }
    return allRules;
  }

  private static GlyphList extract(CoverageTable table) {
    switch (table.format) {
    case 1:
      return extract(table.fmt1Table());
    case 2:
      RangeRecordTable array = table.fmt2Table();
      Map<Integer, GlyphGroup> map = extract(array);
      Collection<GlyphGroup> groups = map.values();
      GlyphList result = new GlyphList();
      for (GlyphGroup glyphIds : groups) {
        glyphIds.copyTo(result);
      }
      return result;
    default:
      throw new IllegalArgumentException("unimplemented format " + table.format);
    }
  }

  private static GlyphList extract(RecordsTable<NumRecord> table) {
    GlyphList result = new GlyphList();
    for (NumRecord record : table.recordList) {
      result.add(record.value);
    }
    return result;
  }

  private static Map<Integer, GlyphGroup> extract(RangeRecordTable table) {
    // Order is important.
    Map<Integer, GlyphGroup> result = new LinkedHashMap<Integer, GlyphGroup>();
    for (RangeRecord record : table.recordList) {
      if (!result.containsKey(record.property)) {
        result.put(record.property, new GlyphGroup());
      }
      GlyphGroup existingGlyphs = result.get(record.property);
      existingGlyphs.addAll(extract(record));
    }
    return result;
  }

  private static GlyphGroup extract(RangeRecord record) {
    int len = record.end - record.start + 1;
    GlyphGroup result = new GlyphGroup();
    for (int i = record.start; i <= record.end; i++) {
      result.add(i);
    }
    return result;
  }

  private static List<Rule> extract(LigatureSet table) {
    List<Rule> allRules = new ArrayList<Rule>();

    for (int i = 0; i < table.subTableCount(); i++) {
      Rule subRule = extract(table.subTableAt(i));
      allRules.add(subRule);
    }
    return allRules;
  }

  private static Rule extract(Ligature table) {

    int glyphId = table.getField(Ligature.LIG_GLYPH_INDEX);
    RuleSegment subst = new RuleSegment(glyphId);
    RuleSegment input = new RuleSegment();
    for (NumRecord record : table.recordList) {
      input.add(record.value);
    }
    return new Rule(null, input, null, subst);
  }

  private static Set<Rule> extract(SingleSubst table) {
    switch (table.format) {
    case 1:
      return extract(table.fmt1Table());
    case 2:
      return extract(table.fmt2Table());
    default:
      throw new IllegalArgumentException("unimplemented format " + table.format);
    }
  }

  private static Set<Rule> extract(HeaderFmt1 fmt1Table) {
    List<Integer> coverage = extract(fmt1Table.coverage);
    int delta = fmt1Table.getDelta();
    return Rule.deltaRules(coverage, delta);
  }

  private static Set<Rule> extract(InnerArrayFmt2 fmt2Table) {
    List<Integer> coverage = extract(fmt2Table.coverage);
    List<Integer> substs = extract((RecordsTable<NumRecord>) fmt2Table);
    return Rule.oneToOneRules(coverage, substs);
  }

  private static Set<Rule> extract(MultipleSubst table) {
    Set<Rule> result = new LinkedHashSet<Rule>();

    GlyphList coverage = extract(table.coverage());
    int i = 0;
    for (NumRecordTable glyphIds : table) {
      RuleSegment input = new RuleSegment(coverage.get(i));

      GlyphList glyphList = extract(glyphIds);
      RuleSegment subst = new RuleSegment(glyphList);

      Rule rule = new Rule(null, input, null, subst);
      result.add(rule);
      i++;
    }
    return result;
  }

  private static Set<Rule> extract(AlternateSubst table) {
    Set<Rule> result = new LinkedHashSet<Rule>();

    GlyphList coverage = extract(table.coverage());
    int i = 0;
    for (NumRecordTable glyphIds : table) {
      RuleSegment input = new RuleSegment(coverage.get(i));

      GlyphList glyphList = extract(glyphIds);
      GlyphGroup glyphGroup = new GlyphGroup(glyphList);
      RuleSegment subst = new RuleSegment(glyphGroup);

      Rule rule = new Rule(null, input, null, subst);
      result.add(rule);
      i++;
    }
    return result;
  }

  private static Set<Rule> extract(ContextSubst table, LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allLookupRules) {
    switch (table.format) {
    case 1:
      return extract(table.fmt1Table(), lookupListTable, allLookupRules);
    case 2:
      return extract(table.fmt2Table(), lookupListTable, allLookupRules);
    default:
      throw new IllegalArgumentException("unimplemented format " + table.format);
    }
  }

  private static Set<Rule> extract(SubRuleSetArray table, LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allLookupRules) {
    GlyphList coverage = extract(table.coverage);

    Set<Rule> result = new LinkedHashSet<Rule>();
    int i = 0;
    for (SubRuleSet subRuleSet : table) {
      Set<Rule> subRules = extract(coverage.get(i), subRuleSet, lookupListTable, allLookupRules);
      result.addAll(subRules);
      i++;
    }
    return result;
  }

  private static Set<Rule> extract(
      Integer firstGlyph, SubRuleSet table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    Set<Rule> result = new LinkedHashSet<Rule>();
    for (SubRule subRule : table) {
      Set<Rule> subrules = extract(firstGlyph, subRule, lookupListTable, allLookupRules);
      if (subrules == null) {
        return null;
      }
      result.addAll(subrules);
    }
    return result;
  }

  private static Set<Rule> extract(
      Integer firstGlyph, SubRule table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    RuleSegment inputRow = new RuleSegment(firstGlyph);
    for (NumRecord record : table.inputGlyphs) {
      inputRow.add(record.value);
    }

    Rule ruleSansSubst = new Rule(null, inputRow, null, null);
    return applyChainingLookup(ruleSansSubst, table.lookupRecords, lookupListTable, allLookupRules);
  }

  private static Set<Rule> extract(
      SubClassSetArray table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    GlyphList coverage = extract(table.coverage);
    Map<Integer, GlyphGroup> classDef = extract(table.classDef);

    Set<Rule> result = new LinkedHashSet<Rule>();
    int i = 0;
    for (SubClassSet subClassRuleSet : table) {
      if (subClassRuleSet != null) {
        Set<Rule> subRules = extract(subClassRuleSet, i, classDef, lookupListTable, allLookupRules);
        result.addAll(subRules);
      }
      i++;
    }
    return result;
  }

  private static Set<Rule> extract(SubClassSet table, int firstInputClass,
      Map<Integer, GlyphGroup> inputClassDef, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    Set<Rule> result = new LinkedHashSet<Rule>();
    for (SubClassRule subRule : table) {
      Set<Rule> subRules = extract(subRule, firstInputClass, inputClassDef, lookupListTable, allLookupRules);
      result.addAll(subRules);
    }
    return result;
  }

  private static Set<Rule> extract(SubClassRule table, int firstInputClass,
      Map<Integer, GlyphGroup> inputClassDef, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    RuleSegment input = extract(firstInputClass, table.inputClasses(), inputClassDef);

    Rule ruleSansSubst = new Rule(null, input, null, null);
    return applyChainingLookup(ruleSansSubst, table.lookupRecords, lookupListTable, allLookupRules);
  }

  private static Set<Rule> extract(
      ChainContextSubst table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    switch (table.format) {
    case 1:
      return extract(table.fmt1Table(), lookupListTable, allLookupRules);
    case 2:
      return extract(table.fmt2Table(), lookupListTable, allLookupRules);
    case 3:
      return extract(table.fmt3Table(), lookupListTable, allLookupRules);
    default:
      throw new IllegalArgumentException("unimplemented format " + table.format);
    }
  }

  private static Set<Rule> extract(
      ChainSubRuleSetArray table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    GlyphList coverage = extract(table.coverage);

    Set<Rule> result = new LinkedHashSet<Rule>();
    int i = 0;
    for (ChainSubRuleSet subRuleSet : table) {
      Set<Rule> subRules = extract(coverage.get(i), subRuleSet, lookupListTable, allLookupRules);
      result.addAll(subRules);
      i++;
    }
    return result;
  }

  private static Set<Rule> extract(
      Integer firstGlyph, ChainSubRuleSet table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    Set<Rule> result = new LinkedHashSet<Rule>();
    for (ChainSubRule subRule : table) {
      result.addAll(extract(firstGlyph, subRule, lookupListTable, allLookupRules));
    }
    return result;
  }

  private static Set<Rule> extract(
      Integer firstGlyph, ChainSubRule table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    RuleSegment inputRow = new RuleSegment(firstGlyph);
    for (NumRecord record : table.inputClasses) {
      inputRow.add(record.value);
    }

    RuleSegment backtrack = ruleSegmentFromGlyphs(table.backtrackGlyphs);
    RuleSegment lookAhead = ruleSegmentFromGlyphs(table.lookAheadGlyphs);

    Rule ruleSansSubst = new Rule(backtrack, inputRow, lookAhead, null);
    return applyChainingLookup(ruleSansSubst, table.lookupRecords, lookupListTable, allLookupRules);
  }

  private static RuleSegment ruleSegmentFromGlyphs(NumRecordList records) {
    RuleSegment segment = new RuleSegment();
    for (NumRecord record : records) {
      segment.add(new GlyphGroup(record.value));
    }
    return segment;
  }

  private static Set<Rule> extract(
      ChainSubClassSetArray table, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {
    Map<Integer, GlyphGroup> backtrackClassDef = extract(table.backtrackClassDef);
    Map<Integer, GlyphGroup> inputClassDef = extract(table.inputClassDef);
    Map<Integer, GlyphGroup> lookAheadClassDef = extract(table.lookAheadClassDef);

    Set<Rule> result = new LinkedHashSet<Rule>();
    int i = 0;
    for (ChainSubClassSet chainSubRuleSet : table) {
      if (chainSubRuleSet != null) {
        result.addAll(extract(chainSubRuleSet,
            backtrackClassDef,
            i,
            inputClassDef,
            lookAheadClassDef,
            lookupListTable,
            allLookupRules));
      }
      i++;
    }
    return result;
  }

  private static Map<Integer, GlyphGroup> extract(ClassDefTable table) {
    switch (table.format) {
    case 1:
      return extract(table.fmt1Table());
    case 2:
      return extract(table.fmt2Table());
    default:
      throw new IllegalArgumentException("unimplemented format " + table.format);
    }
  }

  private static Map<Integer, GlyphGroup> extract(InnerArrayFmt1 table) {
    Map<Integer, GlyphGroup> result = new HashMap<Integer, GlyphGroup>();
    int glyphId = table.getField(InnerArrayFmt1.START_GLYPH_INDEX);
    for (NumRecord record : table) {
      int classId = record.value;
      if (!result.containsKey(classId)) {
        result.put(classId, new GlyphGroup());
      }

      result.get(classId).add(glyphId);
      glyphId++;
    }
    return result;
  }

  private static List<Rule> extract(ChainSubClassSet table,
      Map<Integer, GlyphGroup> backtrackClassDef,
      int firstInputClass,
      Map<Integer, GlyphGroup> inputClassDef,
      Map<Integer, GlyphGroup> lookAheadClassDef,
      LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allLookupRules) {
    List<Rule> result = new ArrayList<Rule>();
    for (ChainSubClassRule chainSubRule : table) {
      result.addAll(extract(chainSubRule,
          backtrackClassDef,
          firstInputClass,
          inputClassDef,
          lookAheadClassDef,
          lookupListTable,
          allLookupRules));
    }
    return result;
  }

  private static Set<Rule> extract(ChainSubClassRule table,
      Map<Integer, GlyphGroup> backtrackClassDef,
      int firstInputClass,
      Map<Integer, GlyphGroup> inputClassDef,
      Map<Integer, GlyphGroup> lookAheadClassDef,
      LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allLookupRules) {
    RuleSegment backtrack = ruleSegmentFromClasses(table.backtrackGlyphs, backtrackClassDef);
    RuleSegment inputRow = extract(firstInputClass, table.inputClasses, inputClassDef);
    RuleSegment lookAhead = ruleSegmentFromClasses(table.lookAheadGlyphs, lookAheadClassDef);

    Rule ruleSansSubst = new Rule(backtrack, inputRow, lookAhead, null);
    return applyChainingLookup(ruleSansSubst, table.lookupRecords, lookupListTable, allLookupRules);
  }

  private static RuleSegment extract(
      int firstInputClass, NumRecordList inputClasses, Map<Integer, GlyphGroup> classDef) {
    RuleSegment input = new RuleSegment(classDef.get(firstInputClass));
    for (NumRecord inputClass : inputClasses) {
      int classId = inputClass.value;
      GlyphGroup glyphs = classDef.get(classId);
      if (glyphs == null && classId == 0) {
        // Any glyph not mentioned in the classes
        glyphs = GlyphGroup.inverseGlyphGroup(classDef.values());
      }
      input.add(glyphs);
    }
    return input;
  }

  private static RuleSegment ruleSegmentFromClasses(
      NumRecordList classes, Map<Integer, GlyphGroup> classDef) {
    RuleSegment segment = new RuleSegment();
    for (NumRecord classRecord : classes) {
      int classId = classRecord.value;
      GlyphGroup glyphs = classDef.get(classId);
      if (glyphs == null && classId == 0) {
        // Any glyph not mentioned in the classes
        glyphs = GlyphGroup.inverseGlyphGroup(classDef.values());
      }
      segment.add(glyphs);
    }
    return segment;
  }

  private static Set<Rule> extract(InnerArraysFmt3 table, LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allLookupRules) {
    RuleSegment backtrackContext = extract(table.backtrackGlyphs);
    RuleSegment input = extract(table.inputGlyphs);
    RuleSegment lookAheadContext = extract(table.lookAheadGlyphs);

    Rule ruleSansSubst = new Rule(backtrackContext, input, lookAheadContext, null);
    Set<Rule> result = applyChainingLookup(
        ruleSansSubst, table.lookupRecords, lookupListTable, allLookupRules);
    return result;
  }

  private static Set<Rule> extract(ReverseChainSingleSubst table) {
    List<Integer> coverage = extract(table.coverage);

    RuleSegment backtrackContext = new RuleSegment();
    backtrackContext.addAll(extract(table.backtrackGlyphs));

    RuleSegment lookAheadContext = new RuleSegment();
    lookAheadContext.addAll(extract(table.lookAheadGlyphs));

    List<Integer> substs = extract(table.substitutes);

    return Rule.oneToOneRules(backtrackContext, coverage, lookAheadContext, substs);
  }

  private static Set<Rule> applyChainingLookup(Rule ruleSansSubst,
      SubstLookupRecordList lookups, LookupListTable lookupListTable, Map<Integer, Set<Rule>> allLookupRules) {

    LinkedList<Rule> targetRules = new LinkedList<Rule>();
    targetRules.add(ruleSansSubst);
    for (SubstLookupRecord lookup : lookups) {
      int at = lookup.sequenceIndex;
      int lookupIndex = lookup.lookupListIndex;
      Set<Rule> rulesToApply = extract(lookupListTable, allLookupRules, lookupIndex);
      if (rulesToApply == null) {
        throw new IllegalArgumentException(
            "Out of bound lookup index for chaining lookup: " + lookupIndex);
      }
      LinkedList<Rule> newRules = Rule.applyRulesOnRules(rulesToApply, targetRules, at);

      LinkedList<Rule> result = new LinkedList<Rule>();
      result.addAll(newRules);
      result.addAll(targetRules);
      targetRules = result;
    }

    Set<Rule> result = new LinkedHashSet<Rule>();
    for (Rule rule : targetRules) {
      if (rule.subst == null) {
        continue;
      }
      result.add(rule);
    }
    return result;
  }

  static Map<Integer, Set<Rule>> extract(LookupListTable table) {
    Map<Integer, Set<Rule>> allRules = new TreeMap<Integer, Set<Rule>>();
    for (int i = 0; i < table.subTableCount(); i++) {
      extract(table, allRules, i);
    }
    return allRules;
  }

  private static Set<Rule> extract(LookupListTable lookupListTable,
      Map<Integer, Set<Rule>> allRules, int i) {
    if (allRules.containsKey(i)) {
      return allRules.get(i);
    }

    Set<Rule> rules = new LinkedHashSet<Rule>();

    LookupTable lookupTable = lookupListTable.subTableAt(i);
    GsubLookupType lookupType = lookupTable.lookupType();
    for (SubstSubtable substSubtable : lookupTable) {
      GsubLookupType subTableLookupType = lookupType;

      if (lookupType == GsubLookupType.GSUB_EXTENSION) {
        ExtensionSubst extensionSubst = (ExtensionSubst) substSubtable;
        substSubtable = extensionSubst.subTable();
        subTableLookupType = extensionSubst.lookupType();
      }

      Set<Rule> subrules = null;

      switch (subTableLookupType) {
      case GSUB_LIGATURE:
        subrules = extract((LigatureSubst) substSubtable);
        break;
      case GSUB_SINGLE:
        subrules = extract((SingleSubst) substSubtable);
        break;
      case GSUB_ALTERNATE:
        subrules = extract((AlternateSubst) substSubtable);
        break;
      case GSUB_MULTIPLE:
        subrules = extract((MultipleSubst) substSubtable);
        break;
      case GSUB_REVERSE_CHAINING_CONTEXTUAL_SINGLE:
        subrules = extract((ReverseChainSingleSubst) substSubtable);
        break;
      case GSUB_CHAINING_CONTEXTUAL:
        subrules = extract((ChainContextSubst) substSubtable, lookupListTable, allRules);
        break;
      case GSUB_CONTEXTUAL:
        subrules = extract((ContextSubst) substSubtable, lookupListTable, allRules);
        break;
      default:
        throw new IllegalStateException();
      }
      if (subrules == null) {
        throw new IllegalStateException();
      }
      rules.addAll(subrules);
    }

    if (rules.size() == 0) {
      System.err.println("There are no rules in lookup " + i);
    }
    allRules.put(i, rules);
    return rules;
  }

  private static RuleSegment extract(CoverageArray table) {
    RuleSegment result = new RuleSegment();
    for (CoverageTable coverage : table) {
      GlyphGroup glyphGroup = new GlyphGroup();
      glyphGroup.addAll(extract(coverage));
      result.add(glyphGroup);
    }
    return result;
  }
}
