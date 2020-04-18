/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

import com.google.typography.font.sfntly.Font.PlatformId;
import com.google.typography.font.sfntly.Font.WindowsEncodingId;
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;
import com.google.typography.font.sfntly.testutils.TestUtils;

import junit.framework.TestCase;

import java.io.File;
import java.nio.charset.CharsetEncoder;

/**
 * @author Stuart Gill
 */
public class GlyphTests extends TestCase {

  public GlyphTests(String name) {
    super(name);
  }

  private static final class TestSet {
    private final File fontFile;
    private final int platformId;
    private final int encodingId;
    private final String charSetName;
    private final int lowChar;
    private final int highChar;

    /**
     * @param fontFile
     * @param platformId
     * @param encodingId
     * @param charSetName
     * @param lowChar
     * @param highChar
     */
    public TestSet(final File fontFile,
        final int platformId,
        final int encodingId,
        final String charSetName,
        final int lowChar,
        final int highChar) {
      this.fontFile = fontFile;
      this.platformId = platformId;
      this.encodingId = encodingId;
      this.charSetName = charSetName;
      this.lowChar = lowChar;
      this.highChar = highChar;
    }

    /**
     * @param fontFile
     * @param platformId
     * @param encodingId
     * @param lowChar
     * @param highChar
     */
    public TestSet(final File fontFile,
        final int platformId,
        final int encodingId,
        final int lowChar,
        final int highChar) {
      this(fontFile, 
          platformId, encodingId, "",
          lowChar, highChar);
    }
  }

  private static final TestSet[] GLYPH_TESTS = {
    new TestSet(TestFont.TestFontNames.OPENSANS.getFile(),
        PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(), 
        0x20, 0x7f),

    // TODO: reinstate Cambria, Batang, and Arial for internal tests, or replace with open-source
  };

  public void testGlyphs() throws Exception {
    for (TestSet test : GLYPH_TESTS) {
      checkTestSet(test);
    }
  }

  private void checkTestSet(TestSet test) throws Exception {
    Font[] fonts = TestFontUtils.loadFont(test.fontFile);
    Font font = fonts[0];

    CMapTable cmapTable = font.getTable(Tag.cmap);
    LocaTable locaTable = font.getTable(Tag.loca);
    GlyphTable glyphTable = font.getTable(Tag.glyf);

    CMap cmap = cmapTable.cmap(test.platformId, test.encodingId);
    assertNotNull(cmap);
    CharsetEncoder encoder = TestUtils.getEncoder(test.charSetName);
    for (int uchar = test.lowChar; uchar < test.highChar; uchar++) {
      int encChar = uchar;
      if (encoder != null) {
        encChar = TestUtils.encodeOneChar(encoder, uchar);
      }
      int glyphId = cmap.glyphId(encChar);
      if (glyphId != CMapTable.NOTDEF) {
        int offset = locaTable.glyphOffset(glyphId);
        int length = locaTable.glyphLength(glyphId);
        checkGlyph(glyphTable, offset, length);
      }
    }
  }

  /**
   * Basic glyph checking.
   * 
   * Currently only instantiates the glyph from the table and verifies that it
   * is non-null. This does ensure that the glyph data was able to be parsed correctly.
   * 
   * @param table glyph table
   * @param offset glyph offset in table
   * @param length length of glyph data
   */
  private void checkGlyph(GlyphTable table, int offset, int length) {
    Glyph glyph = table.glyph(offset, length);
    if (length != 0) {
      assertNotNull(glyph);
    }
    // Check that padding is computed consistently.
    int padding1 = glyph.padding();
    glyph.instructionSize();
    int padding2 = glyph.padding();
    assertEquals(padding1, padding2);
  }
}
