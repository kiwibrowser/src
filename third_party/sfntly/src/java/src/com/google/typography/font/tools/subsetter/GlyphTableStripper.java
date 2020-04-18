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

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;

import java.io.IOException;
import java.util.List;

/**
 * @author Raph Levien
 */
public class GlyphTableStripper extends TableSubsetterImpl {

  public GlyphTableStripper() {
    super(Tag.glyf, Tag.loca);
  }

  @Override
  public boolean subset(Subsetter subsetter, Font font, Font.Builder fontBuilder)
      throws IOException {

    GlyphTable glyphTable = font.getTable(Tag.glyf);
    LocaTable locaTable = font.getTable(Tag.loca);
    if (glyphTable == null || locaTable == null) {
      throw new RuntimeException("Font to subset is not valid.");
    }
    ReadableFontData originalGlyfData = glyphTable.readFontData();

    GlyphTable.Builder glyphTableBuilder =
        (GlyphTable.Builder) fontBuilder.newTableBuilder(Tag.glyf);
    LocaTable.Builder locaTableBuilder = (LocaTable.Builder) fontBuilder.newTableBuilder(Tag.loca);
    List<Glyph.Builder<? extends Glyph>> glyphBuilders = glyphTableBuilder.glyphBuilders();

    GlyphStripper glyphStripper = new GlyphStripper(glyphTableBuilder);

    for (int i = 0; i < locaTable.numGlyphs(); i++) {
      int oldOffset = locaTable.glyphOffset(i);
      int oldLength = locaTable.glyphLength(i);
      Glyph glyph = glyphTable.glyph(oldOffset, oldLength);
      glyphBuilders.add(glyphStripper.stripGlyph(glyph));
    }

    List<Integer> locaList = glyphTableBuilder.generateLocaList();
    locaTableBuilder.setLocaList(locaList);
    return true;
  }
}
