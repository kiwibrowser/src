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

package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.TableBasedTableBuilder;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

/**
 * A PostScript table. 
 * 
 * @author Stuart Gill
 */
public final class PostScriptTable extends Table {

  private static final int VERSION_1 = 0x10000;
  private static final int VERSION_2 = 0x20000;
  private static final int NUM_STANDARD_NAMES = 258;

  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
   */
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

  private AtomicReference<List<String>> names = new AtomicReference<List<String>>();
  
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

  private PostScriptTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  // TODO: version enum
  public int version() {
    return this.data.readFixed(Offset.version.offset);
  }

  public int italicAngle() {
    return this.data.readFixed(Offset.italicAngle.offset);
  }

  public int underlinePosition() {
    return this.data.readFWord(Offset.underlinePosition.offset);
  }

  public long isFixedPitchRaw() {
    return this.data.readULong(Offset.isFixedPitch.offset);
  }

  public boolean isFixedPitch() {
    return this.isFixedPitchRaw() != 0;
  }

  public long minMemType42() {
    return this.data.readULong(Offset.minMemType42.offset);
  }

  public long maxMemType42() {
    return this.data.readULong(Offset.maxMemType42.offset);
  }

  public long minMemType1() {
    return this.data.readULong(Offset.minMemType1.offset);
  }

  public long maxMemType1() {
    return this.data.readULong(Offset.maxMemType1.offset);
  }

  public int numberOfGlyphs() {
    if (version() == VERSION_1) {
      return NUM_STANDARD_NAMES;
    } else if (version() == VERSION_2) {
      return this.data.readUShort(Offset.numberOfGlyphs.offset);
    } else {
      // TODO: should probably be better at signaling unsupported format
      return -1;
    }
  }
  
  public String glyphName(int glyphNum) {
    int numberOfGlyphs = numberOfGlyphs();
    if (numberOfGlyphs > 0 && (glyphNum < 0 || glyphNum >= numberOfGlyphs)) {
      throw new IndexOutOfBoundsException();
    }
    int glyphNameIndex = 0;
    if (version() == VERSION_1) {
      glyphNameIndex = glyphNum;
    } else if (version() == VERSION_2) {
      glyphNameIndex = this.data.readUShort(Offset.glyphNameIndex.offset + 2 * glyphNum);
    } else {
      return null;
    }
    if (glyphNameIndex < NUM_STANDARD_NAMES) {
      return STANDARD_NAMES[glyphNameIndex];
    }
    return getNames().get(glyphNameIndex - NUM_STANDARD_NAMES);
  }
  // TODO: add getters for 2.5 and possibly other tables?

  // Defer the actual parsing of the name strings until first use. Note that this
  // method can therefore throw various runtime exceptions if the table is corrupted.
  /**
   * Get a list containing the names in the table. Since parsing this list is potentially
   * expensive and may throw an exception when data is corrupted, parsing is deferred until
   * first use.
   * 
   * Also note that the return value is only valid for version 2 tables (potentially to be
   * expanded to other versions later). A non-null value is guaranteed for version 2 (only).
   */
  private List<String> getNames() {
    List<String> result = names.get();
    if (result == null && version() == VERSION_2) {
      synchronized (names) {
        result = names.get();
        if (result == null) {
          result = parse();
          names.compareAndSet(null, result);
        }
      }
    }
    return result;
  }

  private List<String> parse() {
    List<String> names = null;
    if (version() == VERSION_2) {
      names = new ArrayList<String>();
      int index = Offset.glyphNameIndex.offset + 2 * numberOfGlyphs();
      while (index < dataLength()) {
        int strLen = this.data.readUByte(index);
        byte[] nameBytes = new byte[strLen];
        this.data.readBytes(index + 1, nameBytes, 0, strLen);
        try {
          names.add(new String(nameBytes, "ISO-8859-1"));
        } catch (UnsupportedEncodingException e) {
          // Can't happen; ISO-8859-1 is one of the guaranteed encodings.
        }
        index += 1 + strLen;
      }
    } else if (version() == VERSION_1) {
      throw new IllegalStateException("Not meaningful to parse version 1 table");
    }
    return names;
  }
  
  public static class Builder extends TableBasedTableBuilder<PostScriptTable> {

    /**
     * Create a new builder using the header information and data provided.
     *
     * @param header the header information
     * @param data the data holding the table
     * @return a new builder
     */
    public static Builder createBuilder(Header header, WritableFontData data) {
      return new Builder(header, data);
    }
    
    protected Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
    }

    @Override
    protected PostScriptTable subBuildTable(ReadableFontData data) {
      return new PostScriptTable(this.header(), data);
    }
  }
}
