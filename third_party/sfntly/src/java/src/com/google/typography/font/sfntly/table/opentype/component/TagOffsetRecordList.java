package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

import java.util.Iterator;

final class TagOffsetRecordList extends RecordList<TagOffsetRecord> {
  TagOffsetRecordList(WritableFontData data) {
    super(data);
  }

  TagOffsetRecordList(ReadableFontData data) {
    super(data);
  }

  static int sizeOfListOfCount(int count) {
    return RecordList.DATA_OFFSET + count * TagOffsetRecord.RECORD_SIZE;
  }

  TagOffsetRecord getRecordForTag(int tag) {
    Iterator<TagOffsetRecord> iterator = iterator();
    while (iterator.hasNext()) {
      TagOffsetRecord record = iterator.next();
      if (record.tag == tag) {
        return record;
      }
    }
    return null;
  }

  @Override
  protected TagOffsetRecord getRecordAt(ReadableFontData data, int offset) {
    return new TagOffsetRecord(data, offset);
  }

  @Override
  protected int recordSize() {
    return TagOffsetRecord.RECORD_SIZE;
  }
}
