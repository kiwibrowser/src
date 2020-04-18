// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
abstract class GsubLookupTable extends LookupTable {

  protected GsubLookupTable(ReadableFontData data, int base, boolean dataIsCanonical) {
    super(data, base, dataIsCanonical);
  }

  static abstract class Builder<T extends GsubLookupTable> extends LookupTable.Builder {

    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data, dataIsCanonical);
    }

    protected Builder() {
    }

    protected Builder(T table) {
      super(table);
    }
  }
}
