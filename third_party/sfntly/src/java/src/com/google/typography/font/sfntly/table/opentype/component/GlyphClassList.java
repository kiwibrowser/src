package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

public class GlyphClassList extends NumRecordList {
  private GlyphClassList(WritableFontData data) {
    super(data);
  }

  private GlyphClassList(ReadableFontData data) {
    super(data);
  }

  private GlyphClassList(ReadableFontData data, int countDecrement) {
    super(data, countDecrement);
  }

  private GlyphClassList(
      ReadableFontData data, int countDecrement, int countOffset, int valuesOffset) {
    super(data, countDecrement, countOffset, valuesOffset);
  }

  public GlyphClassList(NumRecordList other) {
    super(other);
  }

  public static int sizeOfListOfCount(int count) {
    return RecordList.DATA_OFFSET + count * NumRecord.RECORD_SIZE;
  }
}
