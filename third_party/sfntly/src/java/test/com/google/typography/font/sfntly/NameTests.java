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

import com.google.typography.font.sfntly.Font.PlatformId;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.table.core.NameTable.NameId;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFont.TestFontNames;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.File;

/**
 * @author Stuart Gill
 *
 */
public class NameTests extends TestCase {

  private static final boolean DEBUG = false;

  public NameTests() {
  }

  public NameTests(String name) {
    super(name);
  }

  private static final TestFontNames testFonts[] = {
    TestFont.TestFontNames.OPENSANS
  };
  // total, mac, win
  private static final int[][] nameTestResults = { {26, 13, 13} };

  public void testNameEntries() throws Exception {
    for (int i = 0; i < testFonts.length; i++) {
      File fontFile = testFonts[i].getFile();
      Font[] fonts = TestFontUtils.loadFont(fontFile);
      Font font = fonts[0];
      NameTable nameTable = font.getTable(Tag.name);

      NameId[] nameIds = NameId.values();
      int nameCount = 0;
      int winNameCount = 0;
      int macNameCount = 0;
      for (int nameIndex = 0; nameIndex < nameTable.nameCount(); nameIndex++) {
        NameTable.NameEntry entry = nameTable.nameEntry(nameIndex);
        nameCount++;
        if (entry.platformId() == PlatformId.Macintosh.value()) {
          macNameCount++;
        }

        if (entry.platformId() == PlatformId.Windows.value()) {
          winNameCount++;
        }
        if (DEBUG) {
          System.out.println(entry);
        }
      }
      if (DEBUG) {
        System.out.printf("total = %d, mac = %d, win = %d", nameCount, macNameCount, winNameCount);
      }
      assertEquals(nameTestResults[i][0], nameCount);
      assertEquals(nameTestResults[i][1], macNameCount);
      assertEquals(nameTestResults[i][2], winNameCount);
      assertEquals(nameTable.nameCount(), nameCount);
    }
  }
}
