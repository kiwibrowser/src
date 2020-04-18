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

import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMap.CMapFormat;
import com.google.typography.font.sfntly.table.core.CMapFormat4;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.File;
import java.util.Iterator;
import java.util.List;


/**
 * @author Stuart Gill
 *
 */
public class CMapEditingTests extends TestCase {
  private static final boolean DEBUG = false;

  private static final File TEST_FONT_FILE = TestFont.TestFontNames.OPENSANS.getFile();

  /**
   * Constructor.
   */
  public CMapEditingTests() {
    super();
  }

  public CMapEditingTests(String name) {
    super(name);
  }

  public void testRemoveAllButOneCMap() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);

    CMapTable.Builder cmapTableBuilder = (CMapTable.Builder) fontBuilder.getTableBuilder(Tag.cmap);

    Iterator<? extends CMap.Builder<? extends CMap>> cmapBuilderIter =
      cmapTableBuilder.iterator();
    while (cmapBuilderIter.hasNext()) {
      CMap.Builder<? extends CMap> cmapBuilder = cmapBuilderIter.next();
      if (cmapBuilder.cmapId().equals(CMapId.WINDOWS_BMP)) {
        continue; // keep this one
      }
      cmapBuilderIter.remove(); // delete the rest
    }

    Font font = fontBuilder.build();

    CMapTable cmapTable = font.getTable(Tag.cmap);
    assertEquals(1, cmapTable.numCMaps());
    CMap cmap = cmapTable.cmap(CMapId.WINDOWS_BMP);

    assertEquals(CMapId.WINDOWS_BMP, cmap.cmapId());
  }

  public void testCopyAllCMapToNewFont() throws Exception {

    FontFactory factory = FontFactory.getInstance();

    Font font = TestFontUtils.loadFont(TEST_FONT_FILE)[0];
    CMapTable cmapTable = font.getTable(Tag.cmap);

    Font.Builder fontBuilder = factory.newFontBuilder();

    CMapTable.Builder cmapTableBuilder = (CMapTable.Builder) fontBuilder.newTableBuilder(Tag.cmap);

    Iterator<CMap> cmapIter = cmapTable.iterator();
    while (cmapIter.hasNext()) {
      CMap cmap = cmapIter.next();
      cmapTableBuilder.newCMapBuilder(cmap.cmapId(), cmap.readFontData());
    }

    Font newFont = fontBuilder.build();

    CMapTable newCMapTable = font.getTable(Tag.cmap);
    assertEquals(cmapTable.numCMaps(), newCMapTable.numCMaps());

    CMap cmap = cmapTable.cmap(CMapId.WINDOWS_BMP);
    assertEquals(CMapId.WINDOWS_BMP, cmap.cmapId());
  }

  public void testCMap4WithNoEditing() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    CMapTable.Builder cmapTableBuilder = (CMapTable.Builder) fontBuilder.getTableBuilder(Tag.cmap);

    CMap.Builder<? extends CMap> cmapBuilder =
        cmapTableBuilder.cmapBuilder(CMapId.WINDOWS_BMP);
    assertEquals(cmapBuilder.format(), CMapFormat.Format4);
    
    // build and test the changed font
    Font newFont = fontBuilder.build();
    if (DEBUG) {
      // serialize changed font for debugging
      File dstFontFile = TestFontUtils.serializeFont(newFont, ".ttf");
      System.out.println(dstFontFile);
    }
    CMapTable newCMapTable = newFont.getTable(Tag.cmap);
    CMap newCMap = newCMapTable.cmap(CMapId.WINDOWS_BMP);
    assertNotNull(newCMap);
  }
  
  public void testCMap4Editing() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    CMapTable.Builder cmapTableBuilder = (CMapTable.Builder) fontBuilder.getTableBuilder(Tag.cmap);

    CMap.Builder<? extends CMap> cmapBuilder = 
      cmapTableBuilder.cmapBuilder(CMapId.WINDOWS_BMP);
    if (cmapBuilder.format() != CMapFormat.Format4) {
      fail("Windows BMP CMap is not Format 4.");
    }

    if (DEBUG) {
      System.out.println(cmapBuilder.toString());
    }

    CMapFormat4.Builder cmapFormat4Builder = (CMapFormat4.Builder) cmapBuilder;
    List<CMapFormat4.Builder.Segment> segments = cmapFormat4Builder.getSegments();
    List<Integer> glyphIdArray = cmapFormat4Builder.getGlyphIdArray();

    int segmentModified = -1;
    int newStartCode = 'd';
    int newIdDelta = 0;
    for (int i = 0; i < segments.size(); i++) {
      CMapFormat4.Builder.Segment segment = segments.get(i);
      if ('a' > segment.getStartCount() && 'a' < segment.getEndCount()) {
        if (DEBUG) {
          System.out.println(segment);
        }
        segmentModified = i;
        newIdDelta = segment.getIdDelta() + 1;
        segment.setIdDelta(newIdDelta);
        segment.setStartCount(newStartCode);
      }
    }
    cmapFormat4Builder.setSegments(segments);

    // build and test the changed font
    Font newFont = fontBuilder.build();    
    if (DEBUG) {
      // serialize changed font for debugging
      File dstFontFile = TestFontUtils.serializeFont(newFont, ".ttf");
      System.out.println(dstFontFile);
    }
    CMapTable newCMapTable = newFont.getTable(Tag.cmap);
    CMap newCMap = newCMapTable.cmap(CMapId.WINDOWS_BMP);

    //Font originalFont = TestFontUtils.loadFont(TEST_FONT_FILE)[0];
    //CMapTable cmapTable = originalFont.table(Tag.cmap);
    //CMapTable.CMap cmap = cmapTable.cmap(CMapId.WINDOWS_BMP);
    assertEquals(CMapFormat.Format4.value(), newCMap.format());
    assertTrue(segmentModified >= 0);
    CMapFormat4 cmap4 = (CMapFormat4) newCMap;
    assertEquals(newStartCode, cmap4.startCode(segmentModified));
    assertEquals(newIdDelta, cmap4.idDelta(segmentModified));
    if (DEBUG) {
      int c = 32;
      int gid = newCMap.glyphId(c);
      int newGid = newCMap.glyphId(c);
      System.out.printf("char = %x => original gid = %x, new gid = %x\n", c, gid, newGid);
    }
  }
}
