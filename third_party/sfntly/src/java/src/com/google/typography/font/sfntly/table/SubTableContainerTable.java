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

package com.google.typography.font.sfntly.table;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

/**
 * Abstract base class for tables that have contained subtables.
 * 
 * @author Stuart Gill
 *
 */
public abstract class SubTableContainerTable extends Table {

  /**
   * Constructor.
   * @param header the header for the table
   * @param data the data that contains the table
   */
  protected SubTableContainerTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  public abstract static class Builder<T extends SubTableContainerTable>
  extends Table.Builder<T> {

    protected Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
    }
  }
}