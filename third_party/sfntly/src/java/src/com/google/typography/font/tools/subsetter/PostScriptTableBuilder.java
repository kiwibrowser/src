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

package com.google.typography.font.tools.subsetter;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.PostScriptTable;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Builder for PostScript table. This is currently outside the main sfntly
 * builder hierarchy, but should be migrated into it.
 * 
 * @author Raph Levien
 */
public class PostScriptTableBuilder {

  private static final int VERSION_2 = 0x20000;
  private static final int NUM_STANDARD_NAMES = 258;
  private static final int V1_TABLE_SIZE = 32;

  // Note, this is cut'n'pasted from the PostScriptTable implementation.
  // This is a temporary situation, as the actual logic will be refactored
  // to be part of a builder associated with that type.
  private enum Offset {
    version(0),
    italicAngle(4),
    underlinePosition(8),
    underlineThickness(10),
    isFixedPitch(12),
    minMemType42(16),
    maxMemType42(20),
    minMemType1(24),
    maxMemType1(28),

    // TODO: add support for these versions of the table?
    // Version 2.0 table
    numberOfGlyphs(32),
    glyphNameIndex(34);  // start of table

    // Version 2.5 table

    // Version 4.0 table

    private final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  /**
   * These are the standard PostScript names from the OpenType spec. They are a prefix of the
   * Adobe Glyph List.
   */
  private static final String[] STANDARD_NAMES = {
    ".notdef",
    ".null",
    "nonmarkingreturn",
    "space",
    "exclam",
    "quotedbl",
    "numbersign",
    "dollar",
    "percent",
    "ampersand",
    "quotesingle",
    "parenleft",
    "parenright",
    "asterisk",
    "plus",
    "comma",
    "hyphen",
    "period",
    "slash",
    "zero",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "colon",
    "semicolon",
    "less",
    "equal",
    "greater",
    "question",
    "at",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "bracketleft",
    "backslash",
    "bracketright",
    "asciicircum",
    "underscore",
    "grave",
    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
    "g",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "o",
    "p",
    "q",
    "r",
    "s",
    "t",
    "u",
    "v",
    "w",
    "x",
    "y",
    "z",
    "braceleft",
    "bar",
    "braceright",
    "asciitilde",
    "Adieresis",
    "Aring",
    "Ccedilla",
    "Eacute",
    "Ntilde",
    "Odieresis",
    "Udieresis",
    "aacute",
    "agrave",
    "acircumflex",
    "adieresis",
    "atilde",
    "aring",
    "ccedilla",
    "eacute",
    "egrave",
    "ecircumflex",
    "edieresis",
    "iacute",
    "igrave",
    "icircumflex",
    "idieresis",
    "ntilde",
    "oacute",
    "ograve",
    "ocircumflex",
    "odieresis",
    "otilde",
    "uacute",
    "ugrave",
    "ucircumflex",
    "udieresis",
    "dagger",
    "degree",
    "cent",
    "sterling",
    "section",
    "bullet",
    "paragraph",
    "germandbls",
    "registered",
    "copyright",
    "trademark",
    "acute",
    "dieresis",
    "notequal",
    "AE",
    "Oslash",
    "infinity",
    "plusminus",
    "lessequal",
    "greaterequal",
    "yen",
    "mu",
    "partialdiff",
    "summation",
    "product",
    "pi",
    "integral",
    "ordfeminine",
    "ordmasculine",
    "Omega",
    "ae",
    "oslash",
    "questiondown",
    "exclamdown",
    "logicalnot",
    "radical",
    "florin",
    "approxequal",
    "Delta",
    "guillemotleft",
    "guillemotright",
    "ellipsis",
    "nonbreakingspace",
    "Agrave",
    "Atilde",
    "Otilde",
    "OE",
    "oe",
    "endash",
    "emdash",
    "quotedblleft",
    "quotedblright",
    "quoteleft",
    "quoteright",
    "divide",
    "lozenge",
    "ydieresis",
    "Ydieresis",
    "fraction",
    "currency",
    "guilsinglleft",
    "guilsinglright",
    "fi",
    "fl",
    "daggerdbl",
    "periodcentered",
    "quotesinglbase",
    "quotedblbase",
    "perthousand",
    "Acircumflex",
    "Ecircumflex",
    "Aacute",
    "Edieresis",
    "Egrave",
    "Iacute",
    "Icircumflex",
    "Idieresis",
    "Igrave",
    "Oacute",
    "Ocircumflex",
    "apple",
    "Ograve",
    "Uacute",
    "Ucircumflex",
    "Ugrave",
    "dotlessi",
    "circumflex",
    "tilde",
    "macron",
    "breve",
    "dotaccent",
    "ring",
    "cedilla",
    "hungarumlaut",
    "ogonek",
    "caron",
    "Lslash",
    "lslash",
    "Scaron",
    "scaron",
    "Zcaron",
    "zcaron",
    "brokenbar",
    "Eth",
    "eth",
    "Yacute",
    "yacute",
    "Thorn",
    "thorn",
    "minus",
    "multiply",
    "onesuperior",
    "twosuperior",
    "threesuperior",
    "onehalf",
    "onequarter",
    "threequarters",
    "franc",
    "Gbreve",
    "gbreve",
    "Idotaccent",
    "Scedilla",
    "scedilla",
    "Cacute",
    "cacute",
    "Ccaron",
    "ccaron",
    "dcroat"
  };
  
  private static final Map<String, Integer> INVERTED_STANDARD_NAMES = invertNameMap(STANDARD_NAMES);

  private static Map<String, Integer> invertNameMap(String[] names) {
    Map<String, Integer> nameMap = new HashMap<String, Integer>();
    for (int i = 0; i < names.length; i++) {
      nameMap.put(names[i], i);
    }
    return nameMap;
  }

  private final WritableFontData v1Data;
  private List<String> names;

  public PostScriptTableBuilder() {
    v1Data = WritableFontData.createWritableFontData(V1_TABLE_SIZE);
  }

  /**
   * Initialize the scalar values (underline position, etc) to those from the source post
   * table.
   * 
   * @param src The source table to initialize from.
   */
  public void initV1From(PostScriptTable src) {
    src.readFontData().slice(0, V1_TABLE_SIZE).copyTo(v1Data);
  }

  // TODO: more setters

  public void setNames(List<String> names) {
    this.names = names;
  }
  
  public ReadableFontData build() {
    // Note: we always build a version 2 table. This will be the right thing to do almost all the
    // time, as long as we're dealing with TrueType (as opposed to CFF) fonts.
    if (names == null) {
      return v1Data;
    }
    List<Integer> glyphNameIndices = new ArrayList<Integer>();
    ByteArrayOutputStream nameBos = new ByteArrayOutputStream();
    int nGlyphs = names.size();
    int tableIndex = NUM_STANDARD_NAMES;
    for (String name : names) {
      int glyphNameIndex;
      if (INVERTED_STANDARD_NAMES.containsKey(name)) {
        glyphNameIndex = INVERTED_STANDARD_NAMES.get(name);
      } else {
        glyphNameIndex = tableIndex++;
        // write name as Pascal-style string
        nameBos.write(name.length());
        try {
          nameBos.write(name.getBytes("ISO-8859-1"));
        } catch (UnsupportedEncodingException e) {
          // Can't happen; ISO-8859-1 is one of the guaranteed encodings.
        } catch (IOException e) {
          throw new RuntimeException("Unable to write post table data", e);
        }
      }
      glyphNameIndices.add(glyphNameIndex);
    }
    byte[] nameBytes = nameBos.toByteArray();
    int newLength = 34 + 2 * nGlyphs + nameBytes.length;
    WritableFontData data = WritableFontData.createWritableFontData(newLength);
    v1Data.copyTo(data);
    data.writeFixed(Offset.version.offset, VERSION_2);
    data.writeUShort(Offset.numberOfGlyphs.offset, nGlyphs);
    int index = Offset.glyphNameIndex.offset;
    for (Integer glyphNameIndex : glyphNameIndices) {
      index += data.writeUShort(index, glyphNameIndex);
    }
    if (nameBytes.length > 0) {
      data.writeBytes(index, nameBytes, 0, nameBytes.length);
    }
    return data;
  }
}
