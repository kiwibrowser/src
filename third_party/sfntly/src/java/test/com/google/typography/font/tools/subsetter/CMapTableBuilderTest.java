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
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;

import junit.framework.TestCase;

import java.util.HashMap;
import java.util.Map;

/**
 * @author Raph Levien
 */
public class CMapTableBuilderTest extends TestCase {

  private static void verifyCmap(Map<Integer, Integer> mapping) {
    FontFactory fontFactory = FontFactory.getInstance();
    Font.Builder fontBuilder = fontFactory.newFontBuilder();
    new CMapTableBuilder(fontBuilder, mapping).build();
    
    Font font = fontBuilder.build();
    CMapTable cmapTable = font.getTable(Tag.cmap);
    CMap cmap = cmapTable.cmap(3, 1);
    for (Map.Entry<Integer,Integer> entry : mapping.entrySet()) {
      int unicode = entry.getKey();
      int glyphId = entry.getValue();
      assertEquals(glyphId, cmap.glyphId(unicode));
    }
    assertEquals(CMapTable.NOTDEF, cmap.glyphId(0xffff));
    assertEquals(CMapTable.NOTDEF, cmap.glyphId(0xfffe));
  }
  
  public void testCmapBuilding() {
    Map<Integer, Integer> mapping = new HashMap<Integer, Integer>();
    mapping.put(32, 0);
    mapping.put(33, 1);
    mapping.put(34, 2);
    mapping.put(42, 3);
    mapping.put(43, 2);
    mapping.put(0x1234, 4);
    verifyCmap(mapping);
  }
}
