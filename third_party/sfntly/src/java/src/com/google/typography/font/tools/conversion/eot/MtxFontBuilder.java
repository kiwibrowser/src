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

import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;

import java.util.HashMap;
import java.util.Map;
import java.util.TreeSet;

/**
 * @author Raph Levien
 */
public class MtxFontBuilder {
  private static final int OPENTYPE_VERSION_1_0 = 0x10000;
  private static final int FONT_HEADER_BASE_SIZE = 12;
  private static final int FONT_HEADER_PER_TABLE_SIZE = 16;
  private static final int TABLE_ALIGN = 4;
  private Map<Integer, ReadableFontData> tables;
  private MtxHeadBuilder headBuilder;

  public MtxFontBuilder() {
    tables = new HashMap<Integer, ReadableFontData>();
    headBuilder = new MtxHeadBuilder();
  }

  public MtxHeadBuilder getHeadBuilder() {
    return headBuilder;
  }

  /**
   * Add a table to the font being built.
   * 
   * @param tag 4-byte tag of the table, same format as sfntly.Tag
   * @param data ReadableFontData for table contents
   */
  public void addTable(int tag, ReadableFontData data) {
    tables.put(tag, data);
  }

  /**
   * Add a table to the font being built.
   * 
   * @param tag 4-byte tag of the table, same format as sfntly.Tag
   * @param data byte[] data for table contents
   */
  public void addTableBytes(int tag, byte[] data) {
    addTable(tag, ReadableFontData.createReadableFontData(data));
  }

  private static void putUshort(byte[] buf, int offset, int val) {
    buf[offset] = (byte)(val >> 8);
    buf[offset + 1] = (byte)val;
  }

  private static void putUlong(byte[] buf, int offset, int val) {
    buf[offset] = (byte)(val >> 24);
    buf[offset + 1] = (byte)(val >> 16);
    buf[offset + 2] = (byte)(val >> 8);
    buf[offset + 3] = (byte)val;
  }

  /**
   * Build the font, packing all tables into an OpenType (SFNT) structure.
   * 
   * @return the binary font data
   */
  public byte[] build() {
    addTable(Tag.head, headBuilder.build());

    TreeSet<Integer> tags = new TreeSet<Integer>(tables.keySet());
    int nTables = tables.size();
    int size = FONT_HEADER_BASE_SIZE + FONT_HEADER_PER_TABLE_SIZE * nTables;
    for (Map.Entry<Integer, ReadableFontData> entry : tables.entrySet()) {
      ReadableFontData data = entry.getValue();
      if (data != null) {
        size += (entry.getValue().length() + TABLE_ALIGN - 1) & -TABLE_ALIGN;
      }
    }
    // Note: we use raw byte[] array for building the final font, because the WritableFontData
    // implementation currently has performance issues with unnecessary copying.
    // TODO(raph): Either refactor this code to use the sfntly font builder, or, failing that,
    // when WritableFontData gets better performance, use that.
    byte[] buf = new byte[size];
    putUlong(buf, 0, OPENTYPE_VERSION_1_0);
    putUshort(buf, 4, nTables);
    int entrySelector = 0;
    int searchRange = searchRange(nTables);
    putUshort(buf, 6, searchRange * FONT_HEADER_PER_TABLE_SIZE);
    putUshort(buf, 8, log2(searchRange));
    putUshort(buf, 10, (nTables - searchRange) * FONT_HEADER_PER_TABLE_SIZE);
    int headerOffset = FONT_HEADER_BASE_SIZE;
    int offset = FONT_HEADER_BASE_SIZE + FONT_HEADER_PER_TABLE_SIZE * nTables;
    for (Integer tag : tags) {
      ReadableFontData data = tables.get(tag);
      putUlong(buf, headerOffset, tag.intValue());
      int checksum = 0;  // TODO(raph): compute checksum
      putUlong(buf, headerOffset + 4, checksum);
      if (data == null) {
        putUlong(buf, headerOffset + 8, 0);
        putUlong(buf, headerOffset + 12, 0);
      } else {
        putUlong(buf, headerOffset + 8, offset);
        int length = data.length();
        putUlong(buf, headerOffset + 12, length);
        data.readBytes(0, buf, offset, length);
        offset += (length + TABLE_ALIGN - 1) & -TABLE_ALIGN;
      }
      headerOffset += FONT_HEADER_PER_TABLE_SIZE;
    }
    return buf;
  }

  // visible for testing
  static int searchRange(int x) {
    return Integer.highestOneBit(x);
  }
  
  // visible for testing
  static int log2(int x) {
    return 31 - Integer.numberOfLeadingZeros(x);
  }
}
