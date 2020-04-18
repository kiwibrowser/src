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

package com.google.typography.font.tools.fontinfo;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;

import com.ibm.icu.lang.UCharacter;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Font Utility functions
 * @author Brian Stell, Han-Wen Yeh
 */
public class FontUtils {
  /**
   * Gets a Font object for a font file in the given path
   *
   * @param fontFile
   *          the path to the font file
   * @return the Font object representing the font
   * @throws IOException
   *           if font file does not exist or is invalid
   */
  public static Font[] getFonts(String fontFile) throws IOException {
    return getFonts(new FileInputStream(fontFile));
  }

  /**
   * Gets a Font object for a font file in the InputStream
   *
   * @param is
   *          an InputStream containing the font file
   * @return the Font object representing the font
   * @throws IOException
   *           if font file or is invalid
   */
  public static Font[] getFonts(InputStream is) throws IOException {
    FontFactory fontFactory = FontFactory.getInstance();
    fontFactory.fingerprintFont(true);
    Font[] fonts = null;
  
    try {
      fonts = fontFactory.loadFonts(is);
    } finally {
      is.close();
    }
  
    return fonts;
  }

  /**
   * Gets the table with the specified tag for the given font
   *
   * @param font
   *          the source font
   * @param tag
   *          the tag for the table to return
   * @return the specified table for the given font
   * @throws UnsupportedOperationException
   *           if the font does not contain the table with the specified tag
   * @see Tag
   */
  public static Table getTable(Font font, int tag) {
    Table table = font.getTable(tag);
    if (table == null) {
      throw new RuntimeException("Font has no " + Tag.stringValue(tag) + " table");
    }
    return table;
  }

  /**
   * Gets the cmap table for the given font
   *
   * @param font
   *          the source font
   * @return the cmap table for the given font
   * @throws UnsupportedOperationException
   *           if the font does not contain a valid cmap table
   */
  public static CMapTable getCMapTable(Font font) {
    return (CMapTable) getTable(font, Tag.cmap);
  }

  /**
   * Gets either a UCS4 or UCS2 cmap, if available
   *
   * @param font
   *          the source font
   * @return the UCS4 or UCS2 cmap
   * @throws UnsupportedOperationException
   *           if font does not contain a UCS-4 or UCS-2 cmap
   */
  public static CMap getUCSCMap(Font font) {
    CMapTable cmapTable = getCMapTable(font);
  
    // Obtain the UCS-4 cmap. If it doesn't exist, then obtain the UCS-2 cmap
    CMap cmap = null;
    cmap = cmapTable.cmap(
        Font.PlatformId.Windows.value(), Font.WindowsEncodingId.UnicodeUCS4.value());
    if (cmap == null) {
      cmap = cmapTable.cmap(
          Font.PlatformId.Windows.value(), Font.WindowsEncodingId.UnicodeUCS2.value());
    }
    if (cmap == null) {
      throw new UnsupportedOperationException("Font has no UCS-4 or UCS-2 cmap");
    }
  
    return cmap;
  }

  /**
   * Gets the loca table for the given font
   *
   * @param font
   *          the source font
   * @return the loca table for the given font
   * @throws UnsupportedOperationException
   *           if the font does not contain a valid loca table
   */
  public static LocaTable getLocaTable(Font font) {
    return (LocaTable) getTable(font, Tag.loca);
  }

  /**
   * Gets the glyph table for the given font
   *
   * @param font
   *          the source font
   * @return the glyph table for the given font
   * @throws UnsupportedOperationException
   *           if the font does not contain a valid glyph table
   */
  public static GlyphTable getGlyphTable(Font font) {
    return (GlyphTable) getTable(font, Tag.glyf);
  }

  /**
   * Gets a string version of the code point formatted as "U+hhhh" or "U+hhhhhh"
   *
   * @param codePoint
   *          the code point to format
   * @return a formatted version of the code point as a string
   */
  public static String getFormattedCodePointString(int codePoint) {
    if (UCharacter.isValidCodePoint(codePoint)) {
      if (UCharacter.isBMP(codePoint)) {
        return String.format("U+%04X", codePoint);
      }
      return String.format("U+%06X", codePoint);
    }
    throw new IllegalArgumentException("Invalid code point " + codePoint);
  }
}