package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.data.WritableFontData;

interface Record {
  int writeTo(WritableFontData newData, int base);
}
