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
import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.table.core.CMap.CMapFormat;
import com.google.typography.font.sfntly.table.core.CMapFormat4;
import com.google.typography.font.sfntly.table.core.CMapTable;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;

/**
 * This is a medium-level builder for CMap tables, given the mapping from Unicode codepoint
 * to glyph id.
 *
 * @author Raph Levien
 */
public class CMapTableBuilder {

  private static final int MAX_FORMAT4_ENDCODE = 0xffff;

  private final Font.Builder fontBuilder;
  private final Map<Integer, Integer> mapping;

  public CMapTableBuilder(Font.Builder fontBuilder, Map<Integer, Integer> mapping) {
    this.fontBuilder = fontBuilder;
    this.mapping = mapping;
  }

  private class CMap4Segment {
    private final int startCode;
    private int endCode;
    List<Integer> glyphIds;

    CMap4Segment(int startCode, int endCode) {
      this.startCode = startCode;
      this.endCode = endCode;
      this.glyphIds = new ArrayList<Integer>();
    }

    private boolean isContiguous() {
      int firstId = glyphIds.get(0);
      for (int index = 1; index < glyphIds.size(); index++) {
        if (glyphIds.get(index) != firstId + index) {
          return false;
        }
      }
      return true;
    }

    int idDelta() {
      return isContiguous() ? getGlyphIds().get(0) - getStartCode() : 0;
    }

    public int getStartCode() {
      return startCode;
    }

    public void setEndCode(int endCode) {
      this.endCode = endCode;
    }

    public int getEndCode() {
      return endCode;
    }

    public List<Integer> getGlyphIds() {
      return glyphIds;
    }
  }

  // TODO(raph): This currently uses a simplistic algorithm to compute segments.
  // The segments computed are the longest contiguous segments that actually map
  // glyph ids. A smarter approach would leave "holes", or short runs of glyphs
  // mapped to notdef, to reduce the number of segments.
  private List<CMap4Segment> getFormat4Segments() {
    List<CMap4Segment> result = new ArrayList<CMap4Segment>();
    SortedMap<Integer, Integer> sortedMap = new TreeMap<Integer, Integer>(mapping);
    if (!sortedMap.containsKey(MAX_FORMAT4_ENDCODE)) {
      sortedMap.put(MAX_FORMAT4_ENDCODE, 0);
    }

    CMap4Segment curSegment = null;
    for (Map.Entry<Integer, Integer> entry : sortedMap.entrySet()) {
      int unicode = entry.getKey();
      if (unicode > MAX_FORMAT4_ENDCODE) {
        break;
      }
      int glyphId = entry.getValue();
      if (curSegment == null || unicode != curSegment.getEndCode() + 1) {
        curSegment = new CMap4Segment(unicode, unicode);
        result.add(curSegment);
      } else {
        curSegment.setEndCode(unicode);
      }
      curSegment.getGlyphIds().add(glyphId);
    }
    return result;
  }

  private void buildCMapFormat4(CMapFormat4.Builder builder,
                                List<CMap4Segment> segments) {
    List<CMapFormat4.Builder.Segment> segmentList =
        new ArrayList<CMapFormat4.Builder.Segment>();
    List<Integer> glyphIdArray = new ArrayList<Integer>();

    // The glyphIndexArray immediately follows the idRangeOffset array, so idOffset counts the
    // offset (in shorts) from the beginning of the idRangeOffset array to the next block of
    // glyphIndexArray data.
    int idOffset = segments.size();
    for (int i = 0; i < segments.size(); i++) {
      CMap4Segment segment = segments.get(i);
      int idRangeOffset;
      if (segment.isContiguous()) {
        idRangeOffset = 0;
      } else {
        idRangeOffset = (idOffset - i) * FontData.DataSize.USHORT.size();
        glyphIdArray.addAll(segment.getGlyphIds());
        idOffset += segment.getGlyphIds().size();
      }
      segmentList.add(new CMapFormat4.Builder.Segment(segment.getStartCode(), segment.getEndCode(),
          segment.idDelta(), idRangeOffset));
    }
    builder.setGlyphIdArray(glyphIdArray);
    builder.setSegments(segmentList);
  }

  public void build() {
    CMapTable.Builder cmapTableBuilder = (CMapTable.Builder) fontBuilder.newTableBuilder(Tag.cmap);
    CMapFormat4.Builder cmapBuilder =
        (CMapFormat4.Builder) cmapTableBuilder.newCMapBuilder(CMapTable.CMapId.WINDOWS_BMP,
            CMapFormat.Format4);
    buildCMapFormat4(cmapBuilder, getFormat4Segments());
  }
}
