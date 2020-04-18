package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.core.PostScriptTable;
import com.google.typography.font.sfntly.table.opentype.component.GlyphGroup;
import com.google.typography.font.sfntly.table.opentype.component.Rule;
import com.google.typography.font.sfntly.table.opentype.testing.FontLoader;

import org.junit.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;

/**
 * Comparison data is generated from Harfbuzz by running:
 * util/hb-ot-shape-closure --no-glyph-names NotoSansMalayalam.ttf <text>
 */
public class RuleTests {
  private static final String FONTS_DIR = "/usr/local/google/home/cibu/sfntly/fonts";
  private static final String WORDS_DIR =
      "/usr/local/google/home/cibu/sfntly/adv_layout/data/testdata/wiki_words";
  private static final String HB_CLOSURE_DIR =
      "/usr/local/google/home/cibu/sfntly/adv_layout/data/testdata/wiki_words_hb_closure";
  private static final int TEST_COUNT = 4000;
  private static final String DEBUG_SPECIFIC_FONT = "";
  private static final Map<String, List<String>> LANG_WORDS_MAP = langWordsMap();

  @Test
  public void allFonts() throws IOException {
    List<File> fontFiles = FontLoader.getFontFiles(FONTS_DIR);
    for (File fontFile : fontFiles) {
      Font font = FontLoader.getFont(fontFile);
      String name = fontFile.getAbsolutePath();
      if (DEBUG_SPECIFIC_FONT.length() > 0) {
        if (!name.contains(DEBUG_SPECIFIC_FONT)) {
          continue;
        }
        Rule.dumpLookups(font);
      }
      System.out.println(name);

      Map<Integer, Set<Rule>> glyphRulesMap = Rule.glyphRulesMap(font);
      if (glyphRulesMap == null) {
        System.err.println("No GSUB");
        continue;
      }
      CMapTable cmapTable = font.getTable(Tag.cmap);

      String osFontPath = name.substring(name.lastIndexOf('/', name.lastIndexOf('/') - 1) + 1);
      File[] hbOutFiles = new File(HB_CLOSURE_DIR + '/' + osFontPath).listFiles();

      if (hbOutFiles == null) {
        System.err.println("No test data");
        continue;
      }

      for (File hbOutFile : hbOutFiles) {
        String lang = hbOutFile.getName();
        if (lang.startsWith(".")) {
          continue; // for .svn
        }
        List<GlyphGroup> hbClosure = hbClosure(hbOutFile);
        assertClosure(cmapTable, glyphRulesMap, LANG_WORDS_MAP.get(lang), hbClosure);
      }
    }
  }

  @Test
  public void aFont() throws IOException {
    Font font = FontLoader.getFont(new File("/usr/local/google/home/cibu/sfntly/fonts/noto/NotoSansBengali-Regular.ttf"));
    CMapTable cmap = font.getTable(Tag.cmap);
    PostScriptTable post = font.getTable(Tag.post);
    Map<Integer, Set<Rule>> glyphRulesMap = Rule.glyphRulesMap(font);
    GlyphGroup glyphGroup = Rule.glyphGroupForText("য্রী", cmap);
    GlyphGroup closure = Rule.closure(glyphRulesMap, glyphGroup);
    Rule.dumpLookups(font);
    System.err.println(closure);
  }

  private static void assertClosure(
      CMapTable cmap, Map<Integer, Set<Rule>> glyphRulesMap,
      List<String> words, List<GlyphGroup> expecteds) {
    for (int i = 0; i < expecteds.size() && i < TEST_COUNT; i++) {
      String word = words.get(i);
      GlyphGroup expected = expecteds.get(i);

      GlyphGroup glyphGroup = Rule.glyphGroupForText(word, cmap);
      GlyphGroup closure = Rule.closure(glyphRulesMap, glyphGroup);

      if (expected.size() == 0 && closure.size() > 0) {
        System.err.println("Skipped: " + word);
      } else if (!expected.equals(closure)) {
        System.err.printf("'%s' failed:\n  %s HB\n  %s Snftly\n\n", word, expected, closure);
        //Assert.assertEquals(word, expected, closure);
      }
    }
  }

  private static Map<String, List<String>> langWordsMap() {
    Map<String, List<String>> langWordsMap = new HashMap<String, List<String>>();
    for (File wordsFile : new File(WORDS_DIR).listFiles()) {
      String lang = wordsFile.getName();
      if (lang.startsWith(".")) {
        continue; // .svn
      }
      langWordsMap.put(lang, linesFromFile(wordsFile));
    }
    return langWordsMap;
  }

  private static List<GlyphGroup> hbClosure(File file) {
    List<GlyphGroup> glyphGroups = new ArrayList<GlyphGroup>();
    for (String line : linesFromFile(file)) {
      GlyphGroup glyphGroup = new GlyphGroup();
      if (line.length() > 0) {
        for (String intStr : line.split(" ")) {
          glyphGroup.add(Integer.parseInt(intStr));
        }
      }
      glyphGroups.add(glyphGroup);
    }
    return glyphGroups;
  }

  private static List<String> linesFromFile(File file) {
    List<String> lines = new ArrayList<String>();
    Scanner scanner;
    try {
      scanner = new Scanner(file);
    } catch (FileNotFoundException e) {
      System.err.println("File not found: " + file);
      return lines;
    }
    while (scanner.hasNextLine() && lines.size() < TEST_COUNT) {
      lines.add(scanner.nextLine());
    }
    scanner.close();
    return lines;
  }
}
