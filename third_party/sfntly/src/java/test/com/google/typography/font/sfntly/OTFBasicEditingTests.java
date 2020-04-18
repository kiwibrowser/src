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

package com.google.typography.font.sfntly;

import com.google.typography.font.sfntly.Font.Builder;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.File;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

/**
 * @author Stuart Gill
 * 
 */
public class OTFBasicEditingTests extends TestCase {

  private static final File TEST_FONT_FILE = TestFont.TestFontNames.OPENSANS.getFile();

  public OTFBasicEditingTests(String name) {
    super(name);
  }

  /**
   * Simple building test. Ensures that all of the builders turn into tables and
   * that there are no extra tables. Also does a simple modification and ensures
   * that the edit makes it through the save process and that the checksum is
   * updated.
   * 
   * @throws Exception
   */
  public void testBuildersToTables() throws Exception {
    Font[] originalFont = TestFontUtils.loadFont(TEST_FONT_FILE);
    long originalChecksum = originalFont[0].checksum();

    Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    Set<Integer> builderTags = new HashSet<Integer>(fontBuilder.tableBuilderMap().keySet());
    FontHeaderTable.Builder headerBuilder = (FontHeaderTable.Builder) fontBuilder
        .getTableBuilder(Tag.head);
    long modDate = headerBuilder.modified();
    headerBuilder.setModified(modDate + 1);
    Font font = fontBuilder.build();

    // ensure that every table had a builder
    Iterator<? extends Table> iter = font.iterator();
    while (iter.hasNext()) {
      Table table = iter.next();
      Header header = table.header();
      assertTrue(builderTags.contains(header.tag()));
      builderTags.remove(header.tag());
    }
    // ensure that every builder turned into a table
    assertTrue(builderTags.isEmpty());

    FontHeaderTable header = font.getTable(Tag.head);
    long afterModDate = header.modified();
    assertEquals(modDate + 1, afterModDate);

    long fontChecksum = font.checksum();

    assertEquals(originalChecksum + 1, fontChecksum);
  }

  /**
   * Simple test of the font level checksum generation and the header table
   * checksum offset.
   * 
   * @throws Exception
   */
  public void testChecksum() throws Exception {
    Font originalFont = TestFontUtils.loadFont(TEST_FONT_FILE)[0];
    long originalChecksum = originalFont.checksum();
    long expectedChecksum = originalChecksum;
    long originalChecksumAdjustment = ((FontHeaderTable) originalFont.getTable(Tag.head))
        .checkSumAdjustment();

    Builder fontBuilder = TestFontUtils.builderForFontFile(TEST_FONT_FILE);
    for (int tag : fontBuilder.tableBuilderMap().keySet()) {
      Table.Builder<? extends Table> tableBuilder = fontBuilder.getTableBuilder(tag);
      WritableFontData data = tableBuilder.data();
      int l = data.readULongAsInt(0);
      
      // add 1 to the first long in every table 
      // => expected checksum should go up by one for each too
      data.writeULong(0, l + 1);
      tableBuilder.setData(data);
      expectedChecksum++;
    }

    Font builtFont = fontBuilder.build();
    long builtChecksum = builtFont.checksum();

    assertEquals(expectedChecksum, builtChecksum);

    FontHeaderTable header = builtFont.getTable(Tag.head);

    long headerAdjustment = (FontHeaderTable.CHECKSUM_ADJUSTMENT_BASE - builtChecksum) & 0xffffffff;
    long checksumAdjustment = header.checkSumAdjustment();
    assertEquals(headerAdjustment, header.checkSumAdjustment());
  }
}
