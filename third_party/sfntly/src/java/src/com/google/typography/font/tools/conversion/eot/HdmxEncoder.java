/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.google.typography.font.tools.conversion.eot;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalDeviceMetricsTable;
import com.google.typography.font.sfntly.table.core.HorizontalMetricsTable;
import com.google.typography.font.sfntly.table.core.MaximumProfileTable;

/**
 * Implementation of compression of CTF horizontal device metrics data, as per
 * section 5.4 of the MicroType Express spec.
 * 
 * @author Raph Levien
 */
public class HdmxEncoder {
  private static int HEADER_SIZE = 8;
  private static int RECORD_SIZE = 2;

  public WritableFontData encode(Font sourceFont) {
    HorizontalDeviceMetricsTable hdmx = sourceFont.getTable(Tag.hdmx);
    HorizontalMetricsTable hmtx = sourceFont.getTable(Tag.hmtx);
    MaximumProfileTable maxp = sourceFont.getTable(Tag.maxp);
    FontHeaderTable head = sourceFont.<FontHeaderTable>getTable(Tag.head);
    int unitsPerEm = head.unitsPerEm();
    int numRecords = hdmx.numRecords();
    int numGlyphs = maxp.numGlyphs();
    MagnitudeDependentWriter magWriter = new MagnitudeDependentWriter();
    for (int i = 0; i < numRecords; i++) {
      int ppem = hdmx.pixelSize(i);
      for (int j = 0; j < numGlyphs; j++) {
        int roundedTtAw =
            ((64 * ppem * hmtx.advanceWidth(j) + unitsPerEm / 2) / unitsPerEm + 32) / 64;
        int surprise = hdmx.width(i, j) - roundedTtAw;
        magWriter.writeValue(surprise);
      }
    }
    magWriter.flush();
    byte[] magBytes = magWriter.toByteArray();
    int resultSize = magBytes.length + HEADER_SIZE + RECORD_SIZE * numRecords;
    WritableFontData result = WritableFontData.createWritableFontData(resultSize);
    result.writeUShort(0, 0);
    result.writeUShort(2, numRecords);
    result.writeLong(4, hdmx.recordSize());
    for (int i = 0; i < numRecords; i++) {
      result.writeByte(HEADER_SIZE + RECORD_SIZE * i, (byte) hdmx.pixelSize(i));
      result.writeByte(HEADER_SIZE + RECORD_SIZE * i + 1, (byte) hdmx.maxWidth(i));
    }
    result.writeBytes(HEADER_SIZE + RECORD_SIZE * numRecords, magBytes);
    return result;
  }
}
