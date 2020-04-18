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
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.truetype.ControlValueTable;

import junit.framework.TestCase;

/**
 * @author Raph Levien
 */
public class CvtEncoderTest extends TestCase {
  public void testCvtEncoder() {
    CvtEncoder e = new CvtEncoder();
    FontFactory factory = FontFactory.getInstance();
    // Font.Builder fontBuilder = Font.Builder.getOTFBuilder(factory);
    Font.Builder fontBuilder = factory.newFontBuilder();
    byte[] cvtData = {(byte)0x00, (byte)0x01, (byte)0x12, (byte)0x34};
    WritableFontData data = WritableFontData.createWritableFontData(cvtData);
    Table.Builder<ControlValueTable> builder =
        (Table.Builder<ControlValueTable>) fontBuilder.newTableBuilder(Tag.cvt);
    // TODO: actually create cvt table, convert it, and test expected result
  }
}
