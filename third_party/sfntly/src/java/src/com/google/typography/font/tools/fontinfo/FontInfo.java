// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.tools.fontinfo;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Font.MacintoshEncodingId;
import com.google.typography.font.sfntly.Font.PlatformId;
import com.google.typography.font.sfntly.Font.UnicodeEncodingId;
import com.google.typography.font.sfntly.Font.WindowsEncodingId;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.math.Fixed1616;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalHeaderTable;
import com.google.typography.font.sfntly.table.core.MaximumProfileTable;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.table.core.NameTable.MacintoshLanguageId;
import com.google.typography.font.sfntly.table.core.NameTable.NameEntry;
import com.google.typography.font.sfntly.table.core.NameTable.NameId;
import com.google.typography.font.sfntly.table.core.NameTable.UnicodeLanguageId;
import com.google.typography.font.sfntly.table.core.NameTable.WindowsLanguageId;
import com.google.typography.font.sfntly.table.core.OS2Table;
import com.google.typography.font.sfntly.table.truetype.CompositeGlyph;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.Glyph.GlyphType;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;
import com.google.typography.font.tools.fontinfo.DataDisplayTable.Align;

import com.ibm.icu.impl.IllegalIcuArgumentException;
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.lang.UScript;
import com.ibm.icu.text.UnicodeSet;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Class of static functions that return information about a given font
 *
 * @author Brian Stell, Han-Wen Yeh
 *
 */
// TODO Make abstract FontInfo class with nonstatic functions and subclass this
// as TrueTypeFontInfo
public class FontInfo {
  private static final int CHECKSUM_LENGTH = 8;
  private static final DecimalFormat twoDecimalPlaces = new DecimalFormat("#.##");

  /**
   * @param font
   *          the source font
   * @return the sfnt version of the font
   */
  public static String sfntVersion(Font font) {
    double version = Fixed1616.doubleValue(font.sfntVersion());
    NumberFormat numberFormatter = NumberFormat.getInstance();
    numberFormatter.setMinimumFractionDigits(2);
    numberFormatter.setGroupingUsed(false);
    return numberFormatter.format(version);
  }

  /**
   * Gets a list of information regarding various dimensions about the given
   * font from the head, hhea, and OS/2 font tables
   *
   * @param font
   *          the source font
   * @return a list of dimensional information about the font
   */
  public static DataDisplayTable listFontMetrics(Font font) {
    String[] header = { "Name", "Value" };
    Align[] displayAlignment = { Align.Left, Align.Left };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Retrieve necessary tables
    FontHeaderTable headTable = (FontHeaderTable) FontUtils.getTable(font, Tag.head);
    HorizontalHeaderTable hheaTable = (HorizontalHeaderTable) FontUtils.getTable(font, Tag.hhea);
    OS2Table os2Table = (OS2Table) FontUtils.getTable(font, Tag.OS_2);

    table.add(Arrays.asList(
        new String[] { "Units per em", String.format("%d", headTable.unitsPerEm()) }));
    table.add(Arrays.asList(new String[] {
        "[xMin, xMax]", String.format("[%d, %d]", headTable.xMin(), headTable.xMax()) }));
    table.add(Arrays.asList(new String[] {
        "[yMin, yMax]", String.format("[%d, %d]", headTable.yMin(), headTable.yMax()) }));
    table.add(Arrays.asList(new String[] {
        "Smallest readable size (px per em)", String.format("%d", headTable.lowestRecPPEM()) }));
    table.add(
        Arrays.asList(new String[] { "hhea ascender", String.format("%d", hheaTable.ascender()) }));
    table.add(Arrays.asList(
        new String[] { "hhea descender", String.format("%d", hheaTable.descender()) }));
    table.add(Arrays.asList(
        new String[] { "hhea typographic line gap", String.format("%d", hheaTable.lineGap()) }));
    table.add(Arrays.asList(
        new String[] { "OS/2 Windows ascender", String.format("%d", os2Table.usWinAscent()) }));
    table.add(Arrays.asList(
        new String[] { "OS/2 Windows descender", String.format("%d", os2Table.usWinDescent()) }));
    table.add(Arrays.asList(new String[] {
        "OS/2 typographic ascender", String.format("%d", os2Table.sTypoAscender()) }));
    table.add(Arrays.asList(new String[] {
        "OS/2 typographic ascender", String.format("%d", os2Table.sTypoDescender()) }));
    table.add(Arrays.asList(new String[] {
        "OS/2 typographic line gap", String.format("%d", os2Table.sTypoLineGap()) }));

    return table;
  }

  /**
   * Gets a list of tables in the font as well as their sizes.
   *
   *  This function returns a list of tables in the given font as well as their
   * sizes, both in bytes and as fractions of the overall font size. The
   * information for each font is contained in a TableInfo object.
   *
   * @param font
   *          the source font
   * @return a list of information about tables in the font provided
   */
  public static DataDisplayTable listTables(Font font) {
    String[] header = { "tag", "checksum", "length", "offset" };
    Align[] displayAlignment = { Align.Left, Align.Right, Align.Right, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Total size of font
    int fontSize = 0;

    // Calculate font size
    Iterator<? extends Table> fontTableIter = font.iterator();
    while (fontTableIter.hasNext()) {
      Table fontTable = fontTableIter.next();
      fontSize += fontTable.headerLength();
    }

    // Add table data to output string
    fontTableIter = font.iterator();
    while (fontTableIter.hasNext()) {
      Table fontTable = fontTableIter.next();
      String name = Tag.stringValue(fontTable.headerTag());
      String checksum = String.format("0x%0" + CHECKSUM_LENGTH + "X", fontTable.headerChecksum());
      int length = fontTable.headerLength();
      double lengthPercent = length * 100.0 / fontSize;
      int offset = fontTable.headerOffset();

      // Add table data
      String[] data = { name, checksum,
          String.format("%s (%s%%)", length, twoDecimalPlaces.format(lengthPercent)),
          String.format("%d", offset) };
      table.add(Arrays.asList(data));
    }

    return table;
  }

  /**
   * Gets a list of entries in the name table of a font. These entries contain
   * information related to the font, such as the font name, style name, and
   * copyright notices.
   *
   * @param font
   *          the source font
   * @return a list of entries in the name table of the font
   */
  public static DataDisplayTable listNameEntries(Font font) {
    String[] header = { "Platform", "Encoding", "Language", "Name", "Value" };
    Align[] displayAlignment = { Align.Left, Align.Left, Align.Left, Align.Left, Align.Left };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    NameTable nameTable = (NameTable) FontUtils.getTable(font, Tag.name);
    for (NameEntry entry : nameTable) {

      String eidEntry = ""; // Platform-specific encoding
      String lidEntry = ""; // Language

      switch (PlatformId.valueOf(entry.platformId())) {
      case Unicode:
        eidEntry = UnicodeEncodingId.valueOf(entry.encodingId()).toString();
        lidEntry = UnicodeLanguageId.valueOf(entry.languageId()).toString();
        break;
      case Macintosh:
        eidEntry = MacintoshEncodingId.valueOf(entry.encodingId()).toString();
        lidEntry = MacintoshLanguageId.valueOf(entry.languageId()).toString();
        break;
      case Windows:
        eidEntry = WindowsEncodingId.valueOf(entry.encodingId()).toString();
        lidEntry = WindowsLanguageId.valueOf(entry.languageId()).toString();
        break;
      default:
        break;
      }

      String[] data = { String.format(
          "%s (id=%d)", PlatformId.valueOf(entry.platformId()).toString(), entry.platformId()),
          String.format("%s (id=%d)", eidEntry, entry.encodingId()),
          String.format("%s (id=%d)", lidEntry, entry.languageId()),
          NameId.valueOf(entry.nameId()).toString(), entry.name() };
      table.add(Arrays.asList(data));
    }

    return table;
  }

  /**
   * Gets a list containing the platform ID, encoding ID, and format of all the
   * cmaps in a font
   *
   * @param font
   *          the source font
   * @return a list of information about the cmaps in the font
   */
  public static DataDisplayTable listCmaps(Font font) {
    String[] header = { "Platform ID", "Encoding ID", "Format" };
    Align[] displayAlignment = { Align.Right, Align.Right, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Add information about each individual cmap in the table
    CMapTable cmapTable = FontUtils.getCMapTable(font);
    for (CMap cmap : cmapTable) {
      String[] data = { String.format("%d", cmap.platformId()),
          String.format("%d", cmap.encodingId()), String.format("%d", cmap.format()) };
      table.add(Arrays.asList(data));
    }

    return table;
  }

  /**
   * Gets the number of valid characters in the given font
   *
   * @param font
   *          the source font
   * @return the number of valid characters in the given font
   * @throws UnsupportedOperationException
   *           if font does not contain a UCS-4 or UCS-2 cmap
   */
  public static int numChars(Font font) {
    int numChars = 0;
    CMap cmap = FontUtils.getUCSCMap(font);

    // Find the number of characters that point to a valid glyph
    for (int charId : cmap) {
      if (cmap.glyphId(charId) != CMapTable.NOTDEF) {
        // Valid glyph
        numChars++;
      }
    }

    return numChars;
  }

  /**
   * Gets a list of code points of valid characters and their names in the given
   * font.
   *
   * @param font
   *          the source font
   * @return a list of code points of valid characters and their names in the
   *         given font.
   * @throws UnsupportedOperationException
   *           if font does not contain a UCS-4 or UCS-2 cmap
   */
  public static DataDisplayTable listChars(Font font) {
    String[] header = { "Code point", "Glyph ID", "Unicode-designated name for code point" };
    Align[] displayAlignment = { Align.Right, Align.Right, Align.Left };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Iterate through all code points
    CMap cmap = FontUtils.getUCSCMap(font);
    for (int charId : cmap) {
      int glyphId = cmap.glyphId(charId);
      if (glyphId != CMapTable.NOTDEF) {
        String[] data = { FontUtils.getFormattedCodePointString(charId),
            String.format("%d", glyphId), UCharacter.getExtendedName(charId) };
        table.add(Arrays.asList(data));
      }
    }

    return table;
  }

  // Gets the code point and name of all the characters in the provided string
  // for the font
  // TODO public static DataDisplayTable listChars(Font font, String charString)

  /**
   * Gets a list of Unicode blocks covered by the font and the amount each block
   * is covered.
   *
   * @param font
   *          the source font
   * @return a list of Unicode blocks covered by the font
   */
  // FIXME Find more elegant method of retrieving block data
  public static DataDisplayTable listCharBlockCoverage(Font font) {
    String[] header = { "Block", "Coverage" };
    Align[] displayAlignment = { Align.Left, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Iterate through each block to check for coverage
    CMap cmap = FontUtils.getUCSCMap(font);
    int totalCount = 0;
    for (int i = 0; i < UnicodeBlockData.numBlocks(); i++) {
      String block = UnicodeBlockData.getBlockName(i);
      UnicodeSet set = null;
      try {
        set = new UnicodeSet("[[:Block=" + block + ":]-[:gc=Unassigned:]-[:gc=Control:]]");
      } catch (IllegalIcuArgumentException e) {
        continue;
      }
      int count = 0;
      for (String charStr : set) {
        if (cmap.glyphId(UCharacter.codePointAt(charStr, 0)) > 0) {
          count++;
        }
      }
      if (count > 0) {
        table.add(Arrays.asList(new String[] { String.format(
            "%s [%s, %s]", block, UnicodeBlockData.getBlockStartCode(i),
            UnicodeBlockData.getBlockEndCode(i)), String.format("%d / %d", count, set.size()) }));
      }
      totalCount += count;
    }

    // Add control code points with valid glyphs to find the total number of
    // unicode characters with valid glyphs
    UnicodeSet controlSet = new UnicodeSet("[[:gc=Control:]]");
    for (String charStr : controlSet) {
      if (cmap.glyphId(UCharacter.codePointAt(charStr, 0)) > 0) {
        totalCount++;
      }
    }
    int nonUnicodeCount = numChars(font) - totalCount;
    if (nonUnicodeCount > 0) {
      table.add(Arrays.asList(new String[] { "Unknown", String.format("%d", nonUnicodeCount) }));
    }

    return table;
  }

  /**
   * Gets a list of scripts covered by the font and the amount each block is
   * covered.
   *
   * @param font
   *          the source font
   * @return a list of scripts covered by the font
   */
  public static DataDisplayTable listScriptCoverage(Font font) {
    String[] header = { "Script", "Coverage" };
    Align[] displayAlignment = { Align.Left, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));
    HashMap<Integer, Integer> coveredScripts = new HashMap<Integer, Integer>();

    // Add to script count for the script each code point belongs to
    CMap cmap = FontUtils.getUCSCMap(font);
    for (int charId : cmap) {
      if (cmap.glyphId(charId) != CMapTable.NOTDEF) {
        int scriptCode = UScript.getScript(charId);
        int scriptCount = 1;
        if (coveredScripts.containsKey(scriptCode)) {
          scriptCount += coveredScripts.get(scriptCode);
        }
        coveredScripts.put(scriptCode, scriptCount);
      }
    }

    // For each covered script, find the total size of the script and add
    // coverage to table
    Set<Integer> sortedScripts = new TreeSet<Integer>(coveredScripts.keySet());
    int unknown = 0;
    for (Integer scriptCode : sortedScripts) {
      UnicodeSet scriptSet = null;
      String scriptName = UScript.getName(scriptCode);
      try {
        scriptSet = new UnicodeSet("[[:" + scriptName + ":]]");
      } catch (IllegalIcuArgumentException e) {
        unknown += coveredScripts.get(scriptCode);
        continue;
      }

      table.add(Arrays.asList(new String[] { scriptName,
          String.format("%d / %d", coveredScripts.get(scriptCode), scriptSet.size()) }));
    }
    if (unknown > 0) {
      table.add(Arrays.asList(new String[] { "Unsupported script", String.format("%d", unknown) }));
    }

    return table;
  }

  /**
   * Gets a list of characters needed to fully cover scripts partially covered
   * by the font
   *
   * @param font
   *          the source font
   * @return a list of characters needed to fully cover partially-covered
   *         scripts
   */
  public static DataDisplayTable listCharsNeededToCoverScript(Font font) {
    String[] header = { "Script", "Code Point", "Name" };
    Align[] displayAlignment = { Align.Left, Align.Right, Align.Left };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));
    HashMap<Integer, UnicodeSet> coveredScripts = new HashMap<Integer, UnicodeSet>();

    // Iterate through each set
    CMap cmap = FontUtils.getUCSCMap(font);
    for (int charId : cmap) {
      if (cmap.glyphId(charId) != CMapTable.NOTDEF) {
        int scriptCode = UScript.getScript(charId);
        if (scriptCode == UScript.UNKNOWN) {
          continue;
        }

        UnicodeSet scriptSet = null;
        if (!coveredScripts.containsKey(scriptCode)) {
          // New covered script found, create set
          try {
            scriptSet = new UnicodeSet(
                "[[:" + UScript.getName(scriptCode) + ":]-[:gc=Unassigned:]-[:gc=Control:]]");
          } catch (IllegalIcuArgumentException e) {
            continue;
          }
          coveredScripts.put(scriptCode, scriptSet);
        } else {
          // Set for script already exists, retrieve for character removal
          scriptSet = coveredScripts.get(scriptCode);
        }
        scriptSet.remove(UCharacter.toString(charId));
      }
    }

    // Insert into table in order
    Set<Integer> sortedScripts = new TreeSet<Integer>(coveredScripts.keySet());
    for (Integer scriptCode : sortedScripts) {
      UnicodeSet uSet = coveredScripts.get(scriptCode);
      for (String charStr : uSet) {
        int codePoint = UCharacter.codePointAt(charStr, 0);
        table.add(Arrays.asList(new String[] { String.format("%s", UScript.getName(scriptCode)),
            FontUtils.getFormattedCodePointString(codePoint),
            UCharacter.getExtendedName(codePoint) }));
      }
    }

    return table;
  }

  /**
   * Gets the number of glyphs in the given font
   *
   * @param font
   *          the source font
   * @return the number of glyphs in the font
   * @throws UnsupportedOperationException
   *           if font does not contain a valid glyf table
   */
  public static int numGlyphs(Font font) {
    return ((MaximumProfileTable) FontUtils.getTable(font, Tag.maxp)).numGlyphs();
  }

  /**
   * Gets a list of minimum and maximum x and y dimensions for the glyphs in the
   * font. This is based on the reported min and max values for each glyph and
   * not on the actual outline sizes.
   *
   * @param font
   *          the source font
   * @return a list of glyph dimensions for the font
   */
  public static DataDisplayTable listGlyphDimensionBounds(Font font) {
    String[] header = { "Dimension", "Value" };
    Align[] displayAlignment = { Align.Left, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    LocaTable locaTable = FontUtils.getLocaTable(font);
    GlyphTable glyfTable = FontUtils.getGlyphTable(font);

    // Initialise boundaries
    int xMin = Integer.MAX_VALUE;
    int yMin = Integer.MAX_VALUE;
    int xMax = Integer.MIN_VALUE;
    int yMax = Integer.MIN_VALUE;

    // Find boundaries
    for (int i = 0; i < locaTable.numGlyphs(); i++) {
      Glyph glyph = glyfTable.glyph(locaTable.glyphOffset(i), locaTable.glyphLength(i));
      if (glyph.xMin() < xMin) {
        xMin = glyph.xMin();
      }
      if (glyph.yMin() < yMin) {
        yMin = glyph.yMin();
      }
      if (glyph.xMax() > xMax) {
        xMax = glyph.xMax();
      }
      if (glyph.yMax() > yMax) {
        yMax = glyph.yMax();
      }
    }

    table.add(Arrays.asList(new String[] { "xMin", String.format("%d", xMin) }));
    table.add(Arrays.asList(new String[] { "xMax", String.format("%d", xMax) }));
    table.add(Arrays.asList(new String[] { "yMin", String.format("%d", yMin) }));
    table.add(Arrays.asList(new String[] { "yMax", String.format("%d", yMax) }));

    return table;
  }

  /**
   * Gets the size of hinting instructions in the glyph table, both in bytes and
   * as a fraction of the glyph table size.
   *
   * @param font
   *          the source font
   * @return the amount of hinting that is contained in the font
   */
  public static String hintingSize(Font font) {
    int instrSize = 0;

    LocaTable locaTable = FontUtils.getLocaTable(font);
    GlyphTable glyfTable = FontUtils.getGlyphTable(font);

    // Get hinting information from each glyph
    for (int i = 0; i < locaTable.numGlyphs(); i++) {
      Glyph glyph = glyfTable.glyph(locaTable.glyphOffset(i), locaTable.glyphLength(i));
      instrSize += glyph.instructionSize();
    }

    double percentage = instrSize * 100.0 / glyfTable.headerLength();
    return String.format(
        "%d bytes (%s%% of glyf table)", instrSize, twoDecimalPlaces.format(percentage));
  }

  /**
   * Gets a list of glyphs in the font that are used as subglyphs and the number
   * of times each subglyph is used as a subglyph
   *
   * @param font
   *          the source font
   * @return the number of glyphs in the font that are used as subglyphs of
   *         other glyphs more than once
   */
  public static DataDisplayTable listSubglyphFrequency(Font font) {
    Map<Integer, Integer> subglyphFreq = new HashMap<Integer, Integer>();
    String[] header = { "Glyph ID", "Frequency" };
    Align[] displayAlignment = { Align.Right, Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    LocaTable locaTable = FontUtils.getLocaTable(font);
    GlyphTable glyfTable = FontUtils.getGlyphTable(font);

    // Add subglyphs of all composite glyphs to hashmap
    for (int i = 0; i < locaTable.numGlyphs(); i++) {
      Glyph glyph = glyfTable.glyph(locaTable.glyphOffset(i), locaTable.glyphLength(i));
      if (glyph.glyphType() == GlyphType.Composite) {
        CompositeGlyph cGlyph = (CompositeGlyph) glyph;

        // Add all subglyphs of this glyph to hashmap
        for (int j = 0; j < cGlyph.numGlyphs(); j++) {
          int subglyphId = cGlyph.glyphIndex(j);
          int frequency = 1;
          if (subglyphFreq.containsKey(subglyphId)) {
            frequency += subglyphFreq.get(subglyphId);
          }
          subglyphFreq.put(subglyphId, frequency);
        }
      }
    }

    // Add frequency data to table
    int numSubglyphs = 0;
    Set<Integer> sortedKeySet = new TreeSet<Integer>(subglyphFreq.keySet());
    for (Integer key : sortedKeySet) {
      String[] data = { key.toString(), subglyphFreq.get(key).toString() };
      table.add(Arrays.asList(data));
    }

    return table;
  }

  /**
   * Gets a list of IDs for glyphs that are not mapped by any cmap in the font
   *
   * @param font
   *          the source font
   * @return a list of unmapped glyphs
   */
  public static DataDisplayTable listUnmappedGlyphs(Font font) {
    String[] header = { "Glyph ID" };
    Align[] displayAlignment = { Align.Right };
    DataDisplayTable table = new DataDisplayTable(Arrays.asList(header));
    table.setAlignment(Arrays.asList(displayAlignment));

    // Get a set of all mapped glyph IDs
    Set<Integer> mappedGlyphs = new HashSet<Integer>();
    CMapTable cmapTable = FontUtils.getCMapTable(font);
    for (CMap cmap : cmapTable) {
      for (Integer codePoint : cmap) {
        mappedGlyphs.add(cmap.glyphId(codePoint));
      }
    }

    // Iterate through all glyph IDs and check if in the set
    LocaTable locaTable = FontUtils.getLocaTable(font);
    for (int i = 0; i < locaTable.numGlyphs(); i++) {
      if (!mappedGlyphs.contains(i)) {
        table.add(Arrays.asList(new String[] { String.format("%d", i) }));
      }
    }

    return table;
  }

  // TODO Calculate savings of subglyphs
  // public static int subglyphSavings(Font font) {}

  // TODO Find the maximum glyph nesting depth in a font
  // public static int glyphNestingDepth(Font font) {}

  // TODO Find the maximum glyph nexting depth in a font using the maxp table
  // public static int glyphNestingDepthMaxp(Font font) {}

  // TODO Find number of code points that use simple glyphs and number of code
  // points that use composite glyphs (and provide a list of code points for
  // each one)
  // public static int listSimpleGlyphs(Font font) {}
  // public static int listCompositeGlyphs(Font font) {}

}
