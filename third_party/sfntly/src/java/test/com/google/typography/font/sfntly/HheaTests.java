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

import com.google.typography.font.sfntly.table.core.HorizontalHeaderTable;

import junit.framework.TestCase;


/**
 * @author Stuart Gill
 *
 */
public class HheaTests extends TestCase {

  public void testAdvanceWidthMax() throws Exception {
    FontFactory factory = FontFactory.getInstance();
    Font.Builder fontBuilder = factory.newFontBuilder();
    HorizontalHeaderTable.Builder hheaBuilder =
        (HorizontalHeaderTable.Builder) fontBuilder.newTableBuilder(Tag.hhea);
    hheaBuilder.setAdvanceWidthMax(1234);

    Font font = fontBuilder.build();
    HorizontalHeaderTable hheaTable = font.getTable(Tag.hhea);
    assertEquals(1234, hheaTable.advanceWidthMax());
  }

  public void testNumHMetrics() throws Exception {
    FontFactory factory = FontFactory.getInstance();
    Font.Builder fontBuilder = factory.newFontBuilder();
    HorizontalHeaderTable.Builder hheaBuilder =
        (HorizontalHeaderTable.Builder) fontBuilder.newTableBuilder(Tag.hhea);
    hheaBuilder.setNumberOfHMetrics(12);

    Font font = fontBuilder.build();
    HorizontalHeaderTable hheaTable = font.getTable(Tag.hhea);
    assertEquals(12, hheaTable.numberOfHMetrics());
  }
}
