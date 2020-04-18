/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.typography.font.sfntly;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.table.core.MaximumProfileTable;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;
import com.google.typography.font.sfntly.testutils.TestUtils;

import junit.framework.TestCase;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * @author Stuart Gill
 */
public class GlyphEditingTests extends TestCase {

  private static final boolean DEBUG = false;

  private static final File TEST_FONT_FILE = TestFont.TestFontNames.OPENSANS.getFile();

  /**
   * Constructor.
   */
  public GlyphEditingTests() {
    super();
  }

  public GlyphEditingTests(String name) {
    super(name);
  }

  public void testEditLocaTable() throws Exception {
    int locaSlots = 1024;
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    LocaTable.Builder locaBuilder = (LocaTable.Builder) fontBuilder.getTableBuilder(Tag.loca);

    List<Integer> locaList = locaBuilder.locaList();
    locaList.clear();

    List<Integer> newLoca = new ArrayList<Integer>();
    for (int glyphId = 0; glyphId < locaSlots; glyphId++) {
      newLoca.add(glyphId * 6);
    }
    locaBuilder.setLocaList(newLoca);
    MaximumProfileTable.Builder maxProfileBuilder =
        (MaximumProfileTable.Builder) fontBuilder.getTableBuilder(Tag.maxp);
    maxProfileBuilder.setNumGlyphs(locaSlots - 1);

    Font font = fontBuilder.build();

    LocaTable loca = font.getTable(Tag.loca);
    assertEquals(locaSlots - 1, loca.numGlyphs());
    for (int glyphId = 0; glyphId < loca.numGlyphs(); glyphId++) {
      assertEquals(glyphId * 6, loca.glyphOffset(glyphId));
    }
  }

  public void testRemoveOneGlyph() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    LocaTable.Builder locaTableBuilder = (LocaTable.Builder) fontBuilder.getTableBuilder(Tag.loca);
    FontHeaderTable.Builder headerTableBuilder =
        (FontHeaderTable.Builder) fontBuilder.getTableBuilder(Tag.head);
    MaximumProfileTable.Builder maxpTableBuilder =
        (MaximumProfileTable.Builder) fontBuilder.getTableBuilder(Tag.maxp);
    GlyphTable.Builder glyphTableBuilder =
        (GlyphTable.Builder) fontBuilder.getTableBuilder(Tag.glyf);
    List<Integer> originalLocas = locaTableBuilder.locaList();
    // for (int i = 0; i < originalLocas.size(); i++) {
    // System.out.println(i + " = " + originalLocas.get(i));
    // }
    glyphTableBuilder.setLoca(originalLocas);

    ReadableFontData glyphData = glyphTableBuilder.data();
    WritableFontData glyphBytes = WritableFontData.createWritableFontData(0);
    glyphData.copyTo(glyphBytes);

    int numLocas = locaTableBuilder.numLocas();
    int lastLoca = locaTableBuilder.loca(numLocas - 1);
    int numGlyphs = locaTableBuilder.numGlyphs();
    assertEquals(numLocas, numGlyphs + 1);
    int firstGlyphOffset = locaTableBuilder.glyphOffset(0);
    int firstGlyphLength = locaTableBuilder.glyphLength(0);
    int glyphTableSize = glyphTableBuilder.header().length();

    Header oldHeader = glyphTableBuilder.header();
    List<? extends Glyph.Builder<? extends Glyph>> glyphBuilders =
        glyphTableBuilder.glyphBuilders();
    glyphBuilders.remove(0);
    List<Integer> locaList = glyphTableBuilder.generateLocaList();
    locaTableBuilder.setLocaList(locaList);
    maxpTableBuilder.setNumGlyphs(locaTableBuilder.numGlyphs());

    Font font = fontBuilder.build();

    LocaTable locaTable = font.getTable(Tag.loca);
    GlyphTable glyphTable = font.getTable(Tag.glyf);
    ReadableFontData newGlyphData = glyphTable.readFontData();
    WritableFontData newGlyphBytes = WritableFontData.createWritableFontData(0);
    newGlyphData.copyTo(newGlyphBytes);

    if (DEBUG) {
      System.out.println("old length = " + glyphBytes.length());
      System.out.println("new length = " + newGlyphBytes.length());
      System.out.println("old first glyph length = " + firstGlyphLength);
    }
    assertEquals(glyphBytes.length() - firstGlyphLength, newGlyphBytes.length());
    TestUtils.equals(firstGlyphLength, glyphBytes, 0, newGlyphBytes, newGlyphBytes.length());

    Header newHeader = glyphTable.header();

    if (DEBUG) {
      System.out.println("old header = " + oldHeader);
      System.out.println("new header = " + newHeader);

      System.out.println("old numLocas = " + numLocas);
      System.out.println("locaTable.numLocas() = " + locaTable.numLocas());
      System.out.println(
          "locaTable.loca(locaTable.numLocas() - 1) = " + locaTable.loca(locaTable.numLocas() - 1));
      System.out.println("lastLoca = " + lastLoca);
    }
    assertEquals(locaTable.numGlyphs(), numGlyphs - 1);
    assertEquals(locaTable.loca(locaTable.numLocas() - 1), lastLoca - firstGlyphLength);
    if (DEBUG) {
      System.out.println(
          "glyphTable.dataLength() = " + glyphTable.dataLength() + ", glyphTableSize = "
              + glyphTableSize);
    }
    assertEquals(glyphTable.dataLength(), glyphTableSize - firstGlyphLength);
  }

  public void testClearAllGlyphs() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    LocaTable.Builder locaTableBuilder = (LocaTable.Builder) fontBuilder.getTableBuilder(Tag.loca);
    FontHeaderTable.Builder headerTableBuilder =
        (FontHeaderTable.Builder) fontBuilder.getTableBuilder(Tag.head);
    MaximumProfileTable.Builder maxpTableBuilder =
        (MaximumProfileTable.Builder) fontBuilder.getTableBuilder(Tag.maxp);
    GlyphTable.Builder glyphTableBuilder =
        (GlyphTable.Builder) fontBuilder.getTableBuilder(Tag.glyf);
    glyphTableBuilder.setLoca(locaTableBuilder.locaList());

    int lastGlyphId = locaTableBuilder.numGlyphs() - 1;
    int offset = locaTableBuilder.glyphOffset(lastGlyphId);
    int length = locaTableBuilder.glyphLength(lastGlyphId);

    List<? extends Glyph.Builder<? extends Glyph>> glyphBuilders =
        glyphTableBuilder.glyphBuilders();
    glyphBuilders.clear();
    List<Integer> locaList = glyphTableBuilder.generateLocaList();
    locaTableBuilder.setLocaList(locaList);

    Font font = fontBuilder.build();
    
    LocaTable locaTable = font.getTable(Tag.loca);
    GlyphTable glyphTable = font.getTable(Tag.glyf);

    // "empty" loca table should be 2 locas = 4 bytes long (for short loca format)
    assertEquals(4, locaTable.dataLength());
    assertEquals(0, glyphTable.dataLength());
  }
}
