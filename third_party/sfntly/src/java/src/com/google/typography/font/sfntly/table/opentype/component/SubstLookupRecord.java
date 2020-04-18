package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

final class SubstLookupRecord implements Record {
  static final int RECORD_SIZE = 4;
  private static final int SEQUENCE_INDEX_OFFSET = 0;
  private static final int LOOKUP_LIST_INDEX_OFFSET = 2;
  final int sequenceIndex;
  final int lookupListIndex;

  SubstLookupRecord(ReadableFontData data, int base) {
    this.sequenceIndex = data.readUShort(base + SEQUENCE_INDEX_OFFSET);
    this.lookupListIndex = data.readUShort(base + LOOKUP_LIST_INDEX_OFFSET);
  }

  @Override
  public int writeTo(WritableFontData newData, int base) {
    newData.writeUShort(base + SEQUENCE_INDEX_OFFSET, sequenceIndex);
    newData.writeUShort(base + LOOKUP_LIST_INDEX_OFFSET, lookupListIndex);
    return RECORD_SIZE;
  }
}
