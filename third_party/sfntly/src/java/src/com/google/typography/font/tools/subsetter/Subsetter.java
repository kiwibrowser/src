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
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;

import java.io.IOException;
import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * sfntly sample code demonstrating subsetting. Work in progress.
 *
 * @author Stuart Gill
 */
public class Subsetter {

  protected final Font font;

  // TODO(stuartg): add SmartFontBuilder
  //private SmartFontBuilder fontBuilder;
  private FontFactory fontFactory;
  // TODO(stuartg): can TableSubsetter become TableProcessor?
  protected Set<TableSubsetter> tableSubsetters;

  // settings from user
  private Set<Integer> removeTables;
  private List<Integer> newToOldGlyphs;
  private List<CMapTable.CMapId> cmapIds;

  // inverse of mapping, computed lazily
  private Map<Integer, Integer> oldToNewGlyphs = null;

  protected Subsetter(Font font, FontFactory fontFactory) {
    this.font = font;
    this.fontFactory = fontFactory;
  }

  public void setGlyphs(List<Integer> glyphs) {
    this.newToOldGlyphs = new ArrayList<Integer>(glyphs);
  }

  /**
   * Set the cmaps to be used in the subsetted font. The cmaps are listed in
   * order of priority and the number parameter gives a count of how many of the
   * list should be put into the subsetted font. If there are no matches in the
   * font for any of the provided cmap ids which would lead to a font with no
   * cmap then an error will be thrown during subsetting.
   *
   * The two most common cases would be: <list>
   * <ul>
   * a list of one or more cmap ids with a count setting of 1 <br>This will use
   * the list of cmap ids as an ordered priority and look for an available cmap
   * in the font that matches the requests. Only the first such match will be
   * placed in the subsetted font.
   * </ul>
   * <ul>
   * a list of one or more cmap ids with a count setting equal to the list
   * length <br>This will use the list of cmap ids and try to place each one
   * specified into the subsetted font.
   * </ul>
   * </list>
   *
   * @param cmapIds the cmap ids to use for the subsetted font
   * @param number the maximum number of cmaps to place in the subsetted font
   */
  public void setCMaps(List<CMapTable.CMapId> cmapIds, int number) {
    this.cmapIds = new ArrayList<CMapTable.CMapId>();
    CMapTable cmapTable = this.font.getTable(Tag.cmap);
    if (cmapTable == null) {
      throw new InvalidParameterException("Font has no cmap table.");
    }
    for (CMapTable.CMapId cmapId : cmapIds) {
      CMap cmap = cmapTable.cmap(cmapId);
      if (cmap != null) {
        this.cmapIds.add(cmap.cmapId());
        number--;
        if (number <= 0) {
          break;
        }
      }
    }
    if (this.cmapIds.size() == 0) {
      this.cmapIds = null;
      throw new InvalidParameterException(
          "CMap Id settings would generate font with no cmap sub-table.");
    }
  }

  public void setRemoveTables(Set<Integer> removeTables) {
    this.removeTables = new HashSet<Integer>(removeTables);
  }

  public Font.Builder subset() throws IOException {
    Font.Builder fontBuilder = this.fontFactory.newFontBuilder();

    setUpTables(fontBuilder);

    Set<Integer> tableTags = new TreeSet<Integer>(this.font.tableMap().keySet());
    if (this.removeTables != null) {
      tableTags.removeAll(this.removeTables);
    }

    for (TableSubsetter tableSubsetter : this.tableSubsetters) {
      boolean handled = tableSubsetter.subset(this, this.font, fontBuilder);
      if (handled) {
        tableTags.removeAll(tableSubsetter.tagsHandled());
      }
    }
    for (Integer tag : tableTags) {
      Table table = this.font.getTable(tag);
      if (table != null) {
        fontBuilder.newTableBuilder(tag, table.readFontData());
      }
    }
    return fontBuilder;
  }

  /**
   * Get the permutation table of the old glyph id to the new glyph id.
   *
   * @return the permutation table
   */
  List<Integer> glyphMappingTable() {
    return this.newToOldGlyphs;
  }

  /**
   * Get the inverse mapping, from new glyph id to old.
   *
   * @return the inverse mapping
   */
  Map<Integer, Integer> getInverseMapping() {
    if (oldToNewGlyphs == null) {
      oldToNewGlyphs = new HashMap<Integer, Integer>();
      List<Integer> mapping = glyphMappingTable();
      for (int i = 0; i < mapping.size(); i++) {
        oldToNewGlyphs.put(mapping.get(i), i);
      }
    }
    return oldToNewGlyphs;
  }

  List<CMapTable.CMapId> cmapId() {
    return this.cmapIds;
  }

  // A hook for subclasses to override, to set up tables.
  protected void setUpTables(Font.Builder fontBuilder) {
  }
}
