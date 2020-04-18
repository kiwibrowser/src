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
 * An abstract base to be used building tables in which the builder can use the
 * table itself to build from.
 *
 * @author Stuart Gill
 *
 * @param <T> the type of table to be built
 */
public abstract class TableBasedTableBuilder<T extends Table> extends Table.Builder<T> {
  private T table;

  /**
   * Constructor.
   *
   * @param header
   * @param data
   */
  protected TableBasedTableBuilder(Header header, WritableFontData data) {
    super(header, data);
  }

  /**
   * Constructor.
   *
   * @param header
   * @param data
   */
  protected TableBasedTableBuilder(Header header, ReadableFontData data) {
    super(header, data);
  }

  protected TableBasedTableBuilder(Header header) {
    super(header);
  }

  protected T table() {
    if (this.table == null) {
      this.table = this.subBuildTable(this.internalReadData());
    }
    return this.table;
  }

  @Override
  protected void subDataSet() {
    this.table = null;
  }

  @Override
  protected int subDataSizeToSerialize() {
    return 0;
  }

  @Override
  protected boolean subReadyToSerialize() {
    return true;
  }

  @Override
  protected int subSerialize(WritableFontData newData) {
    return 0;
  }

  @Override
  public T build() {
    if (!this.subReadyToSerialize()) {
      return null;
    }
    T table = this.table();
    this.notifyPostTableBuild(table);
    return table;
  }
}