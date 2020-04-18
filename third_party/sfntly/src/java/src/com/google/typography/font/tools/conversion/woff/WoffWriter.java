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

package com.google.typography.font.tools.conversion.woff;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.TreeSet;
import java.util.zip.Deflater;

/**
 * @author Jeremie Lenfant-Engelmann
 */
public class WoffWriter {

  protected boolean woff_compression_faster = false;

  private static final long SIGNATURE = 0x774F4646;
  private static final int WOFF_HEADER_SIZE =
      (9 * FontData.DataSize.ULONG.size()) + (4 * FontData.DataSize.USHORT.size());

  public WritableFontData convert(Font font) {
    List<TableDirectoryEntry> tableDirectoryEntries = createTableDirectoryEntries(font);
    int length =
        WOFF_HEADER_SIZE + computeTableDirectoryEntriesLength(tableDirectoryEntries)
            + computeTablesLength(tableDirectoryEntries);
    WritableFontData writableFontData = WritableFontData.createWritableFontData(length);
    int index = 0;

    index += writeWoffHeader(writableFontData,
        index,
        tableDirectoryEntries,
        font.sfntVersion(),
        length,
        extractMajorVersion(font),
        extractMinorVersion(font));
    index += writeTableDirectoryEntries(writableFontData, index, tableDirectoryEntries);
    index += writeTables(writableFontData, index, tableDirectoryEntries);
    return writableFontData;
  }

  private int extractMajorVersion(Font font) {
    FontHeaderTable head = (FontHeaderTable) font.getTable(Tag.head);
    return (head.fontRevision() >> 16) & 0xffff;
  }

  private int extractMinorVersion(Font sfntlyFont) {
    FontHeaderTable head = (FontHeaderTable) sfntlyFont.getTable(Tag.head);
    return head.fontRevision() & 0xffff;
  }

  private int align4(int value) {
    return (value + 3) & -4;
  }

  private int computeTableDirectoryEntriesLength(
      List<TableDirectoryEntry> tableDirectoryEntries) {
    return TableDirectoryEntry.ENTRY_SIZE * tableDirectoryEntries.size();
  }

  private int computeTablesLength(List<TableDirectoryEntry> tableDirectoryEntries) {
    int length = 0;
    for (TableDirectoryEntry entry : tableDirectoryEntries) {
      length += entry.getCompressedTableLength();
      length = align4(length);
    }
    return length;
  }

  private int writeWoffHeader(WritableFontData writableFontData,
      int start,
      List<TableDirectoryEntry> tableDirectoryEntries,
      int flavor,
      int length,
      int majorVersion,
      int minorVersion) {
    int index = start;
    index += writableFontData.writeULong(index, SIGNATURE); // signature
    index += writableFontData.writeULong(index, flavor); // flavor
    index += writableFontData.writeULong(index, length); // length
    index += writableFontData.writeUShort(index, tableDirectoryEntries.size()); // numTables
    index += writableFontData.writeUShort(index, 0); // reserved

    // totalSfntSize
    index +=
        writableFontData.writeULong(index, computeUncompressedTablesLength(tableDirectoryEntries)
            + computeTableSfntHeaderLength(tableDirectoryEntries));

    index += writableFontData.writeUShort(index, 1); // majorVersion
    index += writableFontData.writeUShort(index, 1); // minorVersion
    index += writableFontData.writeULong(index, 0); // metaOffset
    index += writableFontData.writeULong(index, 0); // metaLength
    index += writableFontData.writeULong(index, 0); // metaOrigLength
    index += writableFontData.writeULong(index, 0); // privOffset
    index += writableFontData.writeULong(index, 0); // privLength
    return WOFF_HEADER_SIZE;
  }

  private int computeTableSfntHeaderLength(List<TableDirectoryEntry> tableDirectoryEntries) {
    return FontData.DataSize.ULONG.size() + (4 * FontData.DataSize.USHORT.size())
        + ((4 * FontData.DataSize.ULONG.size()) * tableDirectoryEntries.size());
  }

  private int computeUncompressedTablesLength(List<TableDirectoryEntry> tableDirectoryEntries) {
    int length = 0;
    for (TableDirectoryEntry entry : tableDirectoryEntries) {
      length += entry.getUncompressedTableLength();
      length = align4(length);
    }
    return length;
  }

  private int writeTableDirectoryEntries(WritableFontData writableFontData, int start,
      List<TableDirectoryEntry> tableDirectoryEntries) {
    int index = start;
    int tableOffset = align4(start + computeTableDirectoryEntriesLength(tableDirectoryEntries));
    for (TableDirectoryEntry entry : tableDirectoryEntries) {
      index += entry.writeEntry(writableFontData, tableOffset, index);
      tableOffset += entry.getCompressedTableLength();
      tableOffset = align4(tableOffset);
    }
    return computeTableDirectoryEntriesLength(tableDirectoryEntries);
  }

  private int writeTables(WritableFontData writableFontData, int start,
      List<TableDirectoryEntry> tableDirectoryEntries) {
    int index = align4(start);
    for (TableDirectoryEntry entry : tableDirectoryEntries) {
      index += entry.writeTable(writableFontData, index);
      index = align4(index);
    }
    return index - start;
  }

  private List<TableDirectoryEntry> createTableDirectoryEntries(Font font) {
    List<TableDirectoryEntry> tableDirectoryEntries = new ArrayList<TableDirectoryEntry>();
    TreeSet<Integer> tags = new TreeSet<Integer>(font.tableMap().keySet());
    tags.remove(Tag.DSIG);

    for (int tag : tags) {
      Table table = font.getTable(tag);
      TableDirectoryEntry tableDirectoryEntry = new TableDirectoryEntry();
      tableDirectoryEntry.setTag(tag);
      tableDirectoryEntry.setOrigLength(table.dataLength());
      tableDirectoryEntry.setOrigChecksum(table.calculatedChecksum());
      setCompressedTableData(tableDirectoryEntry, table);
      tableDirectoryEntries.add(tableDirectoryEntry);
    }
    return tableDirectoryEntries;
  }

  private void setCompressedTableData(TableDirectoryEntry tableDirectoryEntry, Table table) {
    int length = table.dataLength();
    byte[] input = new byte[length];
    table.readFontData().readBytes(0, input, 0, length);
    if (woff_compression_faster && (length < 100 || table.headerTag() == Tag.loca)) {
      tableDirectoryEntry.setCompTable(input);
    } else {
      byte[] output = new byte[length];
      Deflater compresser = new Deflater();
      compresser.setInput(input);
      compresser.finish();
      int compLength = compresser.deflate(output);
      tableDirectoryEntry.setCompTable(
          compLength == length || !compresser.finished() ? input : Arrays.copyOfRange(
              output, 0, compLength));
    }
  }

  private static class TableDirectoryEntry {

    public static final int ENTRY_SIZE = 5 * FontData.DataSize.ULONG.size();

    private long tag;
    private long origLength;
    private long origChecksum;
    private byte[] compTable;

    public void setTag(int tag) {
      this.tag = tag;
    }

    public void setOrigLength(int origLength) {
      this.origLength = origLength;
    }

    public void setOrigChecksum(long origChecksum) {
      this.origChecksum = origChecksum;
    }

    public void setCompTable(byte[] compTable) {
      this.compTable = compTable;
    }

    public int getCompressedTableLength() {
      return compTable.length;
    }

    public long getUncompressedTableLength() {
      return origLength;
    }

    public int writeEntry(WritableFontData writableFontData, int tableOffset, int start) {
      int index = start;
      index += writableFontData.writeULong(index, tag);
      index += writableFontData.writeULong(index, tableOffset);
      index += writableFontData.writeULong(index, compTable.length);
      index += writableFontData.writeULong(index, origLength);
      index += writableFontData.writeULong(index, origChecksum);
      return ENTRY_SIZE;
    }

    public int writeTable(WritableFontData writableFontData, int index) {
      writableFontData.writeBytes(index, compTable, 0, compTable.length);
      return getCompressedTableLength();
    }
  }
}
