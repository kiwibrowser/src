package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

public final class NumRecord implements Record {
  static final int RECORD_SIZE = 2;
  private static final int TAG_POS = 0;
  final int value;

  NumRecord(ReadableFontData data, int base) {
    this.value = data.readUShort(base + TAG_POS);
  }

  public NumRecord(int num) {
    this.value = num;
  }

  @Override
  public int writeTo(WritableFontData newData, int base) {
    newData.writeUShort(base + TAG_POS, value);
    return RECORD_SIZE;
  }
}
