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

import com.google.typography.font.sfntly.table.core.HorizontalDeviceMetricsTable;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;

/**
 * @author Raph Levien
 */
public class HdmxTests extends TestCase {
  private static final File TEST_FONT_FILE = TestFont.TestFontNames.DROIDSANS.getFile();
  
  public void testBasicHdmx() throws IOException {
    Font font = TestFontUtils.loadFont(TEST_FONT_FILE)[0];
    HorizontalDeviceMetricsTable hdmxTable = font.getTable(Tag.hdmx);
    assertEquals(0, hdmxTable.version());
    assertEquals(19, hdmxTable.numRecords());
    assertEquals(900, hdmxTable.recordSize());
    
    assertEquals(6, hdmxTable.pixelSize(0));
    assertEquals(7, hdmxTable.pixelSize(1));
    assertEquals(24, hdmxTable.pixelSize(18));
    
    assertEquals(7, hdmxTable.maxWidth(0));
    assertEquals(28, hdmxTable.maxWidth(18));
    
    assertEquals(4, hdmxTable.width(0, 0));  // .notdef
    assertEquals(4, hdmxTable.width(1, 0));
    assertEquals(5, hdmxTable.width(2, 0));
    assertEquals(14, hdmxTable.width(18, 0));

    assertEquals(4, hdmxTable.width(0, 36));  // A
    assertEquals(5, hdmxTable.width(1, 36));
    assertEquals(6, hdmxTable.width(2, 36));
    assertEquals(15, hdmxTable.width(18, 36));
  }
  
  public void testHdmxBounds() throws IOException {
    Font font = TestFontUtils.loadFont(TEST_FONT_FILE)[0];
    HorizontalDeviceMetricsTable hdmxTable = font.getTable(Tag.hdmx);

    try {
      int x = hdmxTable.pixelSize(19);
      fail("Expected IndexOutOfBoundsException");
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      int x = hdmxTable.pixelSize(-1);
      fail("Expected IndexOutOfBoundsException");
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      int x = hdmxTable.maxWidth(19);
      fail("Expected IndexOutOfBoundsException");
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      int x = hdmxTable.width(0, 898);
      fail("Expected IndexOutOfBoundsException");
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      int x = hdmxTable.width(1, -1);
      fail("Expected IndexOutOfBoundsException");
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

  }
}
