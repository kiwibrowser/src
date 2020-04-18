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
import com.google.typography.font.sfntly.table.truetype.CompositeGlyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;

import java.util.Map;

/**
 * A utility class for applying a mapping to glyph number references within a TrueType composite
 * glyph object.
 * 
 * @author Raph Levien
 */
public class GlyphRenumberer {

  /**
   * This is adapted from {@link GlyphTable}. If GlyphTable.Offset.offset were to be made public,
   * this could go away. Note that offsets are capitalized according to the OpenType spec, rather
   * than the usual Java constant convention.
   */
  private enum Offset {
    // Offsets relative to glyph header
    numberOfContours(0),
    headerEnd(10),

    // Offsets relative to composite glyph block
    compositeFlags(0),
    compositeGlyphIndex(2);
    
    private final int offset;
    
    private Offset(int offset) {
      this.offset = offset;
    }
  }

  /**
   * Apply a renumbering referenced glyphs in TrueType glyph data. Note that this method has a low
   * level interface (bytes) for efficiency, as, in many cases the glyph data does not need to be
   * fully parsed. We should benchmark this implementation against one written in terms of a higher
   * level interface and measure the actual performance gain - if not significant, this
   * implementation should be scrapped in favor of the higher level one.
   * 
   * @return renumbered glyph data
   */
  public static ReadableFontData renumberGlyph(ReadableFontData glyph,
                                               Map<Integer, Integer> mapping) {
    return isCompositeGlyph(glyph)
        ? renumberCompositeGlyph(glyph, mapping)
        : glyph;
  }

  /**
   * Determine whether the glyph data is a composite glyph. Should always give the same answer
   * as {@code parsedGlyph instanceof GlyphTable.CompositeGlyph}, but faster.
   * 
   * @param glyph
   * @return true if glyph is composite
   */
  private static boolean isCompositeGlyph(ReadableFontData glyph) {
    return glyph.length() > 0 && glyph.readShort(Offset.numberOfContours.offset) < 0;
  }
  
  private static ReadableFontData renumberCompositeGlyph(ReadableFontData glyph,
                                                         Map<Integer, Integer> mapping) {
    WritableFontData result = WritableFontData.createWritableFontData(glyph.length());
    glyph.copyTo(result);
    
    int flags = CompositeGlyph.FLAG_MORE_COMPONENTS;
    int index = Offset.headerEnd.offset;

    while ((flags & CompositeGlyph.FLAG_MORE_COMPONENTS) != 0) {
      flags = glyph.readUShort(index + Offset.compositeFlags.offset);
      int oldGlyphIndex = glyph.readUShort(index + Offset.compositeGlyphIndex.offset);
      int newGlyphIndex = mapping.get(oldGlyphIndex);
      result.writeUShort(index + Offset.compositeGlyphIndex.offset, newGlyphIndex);
      index += compositeReferenceSize(flags);
    }
    return result;
  }
  
  /**
   * Compute the size, in bytes, of a single composite reference.
   */
  private static int compositeReferenceSize(int flags) {
    int result = 6;
    if ((flags & CompositeGlyph.FLAG_ARG_1_AND_2_ARE_WORDS) != 0) {
      result += 2;
    }
    if ((flags & CompositeGlyph.FLAG_WE_HAVE_A_SCALE) != 0) {
      result += 2;
    } else if ((flags & CompositeGlyph.FLAG_WE_HAVE_AN_X_AND_Y_SCALE) != 0) {
      result += 4;
    } else if ((flags & CompositeGlyph.FLAG_WE_HAVE_A_TWO_BY_TWO) != 0) {
      result += 8;
    }
    return result;
  }
}
