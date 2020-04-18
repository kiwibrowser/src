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

package com.google.typography.font.tools.conversion.eot;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.testutils.TestFont.TestFontNames;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;

/**
 * @author Raph Levien
 */
public class EOTWriterTest extends TestCase {
  private static final File fontFile = TestFontNames.OPENSANS.getFile();
  private static final long EOT_VERSION = 0x00020002;

  public void testBasicEot() throws IOException {
    Font srcFont = TestFontUtils.loadFont(fontFile)[0];

    EOTWriter eotWriter = new EOTWriter();
    WritableFontData eotData = eotWriter.convert(srcFont);
    assertEquals(eotData.length(), eotData.readULongLE(0));
    assertEquals(EOT_VERSION, eotData.readULongLE(8));
    // TODO: more sanity-checking and validation
  }
}
