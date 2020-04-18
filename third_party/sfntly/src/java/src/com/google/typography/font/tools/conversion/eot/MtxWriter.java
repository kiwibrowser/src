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
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalDeviceMetricsTable;
import com.google.typography.font.sfntly.table.truetype.ControlValueTable;

import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * @author Raph Levien
 */
public class MtxWriter {
  
  private static final Set<Integer> REMOVE_TABLES = createRemoveTables();
  
  private static Set<Integer> createRemoveTables() {
    Set<Integer> result = new HashSet<Integer>();
    result.add(Tag.VDMX);
    result.add(Tag.glyf);
    result.add(Tag.cvt);
    result.add(Tag.loca);
    result.add(Tag.hdmx);
    result.add(Tag.head);
    return Collections.unmodifiableSet(result);
  }

  public byte[] compress(Font sfntlyFont) {
    MtxFontBuilder fontBuilder = new MtxFontBuilder();
    for (Map.Entry<Integer, ? extends Table> entry : sfntlyFont.tableMap().entrySet()) {
      Integer tag = entry.getKey();
      if (!REMOVE_TABLES.contains(tag)) {
        fontBuilder.addTable(tag, entry.getValue().readFontData());
      }
    }
    FontHeaderTable srcHead = sfntlyFont.getTable(Tag.head);
    fontBuilder.getHeadBuilder().initFrom(srcHead);

    GlyfEncoder glyfEncoder = new GlyfEncoder();
    glyfEncoder.encode(sfntlyFont);
    fontBuilder.addTableBytes(Tag.glyf, glyfEncoder.getGlyfBytes());
    fontBuilder.addTable(Tag.loca, null);

    ControlValueTable cvtTable = sfntlyFont.getTable(Tag.cvt);
    if (cvtTable != null) {
      CvtEncoder cvtEncoder = new CvtEncoder();
      cvtEncoder.encode(cvtTable);
      fontBuilder.addTableBytes(Tag.cvt, cvtEncoder.toByteArray());
    }

    HorizontalDeviceMetricsTable hdmxTable = sfntlyFont.getTable(Tag.hdmx);
    if (hdmxTable != null) {
      fontBuilder.addTable(Tag.hdmx, new HdmxEncoder().encode(sfntlyFont));
    }
    
    byte[] block1 = fontBuilder.build();
    byte[] block2 = glyfEncoder.getPushBytes();
    byte[] block3 = glyfEncoder.getCodeBytes();
    return packMtx(block1, block2, block3);
  }

  private static void writeBE24(byte[] data, int value, int off) {
    data[off] = (byte) ((value >> 16) & 0xff);
    data[off + 1] = (byte) ((value >> 8) & 0xff);
    data[off + 2] = (byte) (value & 0xff);
  }

  /**
   * Compress the blocks and pack them into the final container, as per section 2 of the spec.
   */
  private static byte[] packMtx(byte[] block1, byte[] block2, byte[] block3) {
    int copyDist = Math.max(block1.length, Math.max(block2.length, block3.length)) +
        LzcompCompress.getPreloadSize();
    byte[] compressed1 = LzcompCompress.compress(block1);
    byte[] compressed2 = LzcompCompress.compress(block2);
    byte[] compressed3 = LzcompCompress.compress(block3);
    int resultSize = 10 + compressed1.length + compressed2.length + compressed3.length;
    byte[] result = new byte[resultSize];
    result[0] = 3;
    writeBE24(result, copyDist, 1);
    int offset2 = 10 + compressed1.length;
    int offset3 = offset2 + compressed2.length;
    writeBE24(result, offset2, 4);
    writeBE24(result, offset3, 7);
    System.arraycopy(compressed1, 0, result, 10, compressed1.length);
    System.arraycopy(compressed2, 0, result, offset2, compressed2.length);
    System.arraycopy(compressed3, 0, result, offset3, compressed3.length);
    return result;
  }
}
