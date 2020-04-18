package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

final class RangeRecord implements Record {
  static final int RECORD_SIZE = 6;
  private static final int START_OFFSET = 0;
  private static final int END_OFFSET = 2;
  private static final int PROPERTY_OFFSET = 4;
  final int start;
  final int end;
  final int property;

  RangeRecord(ReadableFontData data, int base) {
    this.start = data.readUShort(base + START_OFFSET);
    this.end = data.readUShort(base + END_OFFSET);
    this.property = data.readUShort(base + PROPERTY_OFFSET);
  }

  @Override
  public int writeTo(WritableFontData newData, int base) {
    newData.writeUShort(base + START_OFFSET, start);
    newData.writeUShort(base + END_OFFSET, end);
    return RECORD_SIZE;
  }
}
