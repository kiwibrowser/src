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
 * An abstract base class for subtables. Subtables are smaller tables nested
 * within other tables and don't have an entry in the main font index. Examples
 * of these are the CMap subtables within CMap table (cmap) or a glyph within
 * the glyph table (glyf).
 *
 * @author Stuart Gill
 *
 */
public abstract class SubTable extends FontDataTable {
  /**
   * The data for the whole table in which this subtable is contained.
   */
  private final ReadableFontData masterData;

  private int padding = 0;

  /**
   * Constructor.
   *
   * @param data the data representing the subtable
   * @param masterData the data representing the full table containing this
   *        subtable
   */
  protected SubTable(ReadableFontData data, ReadableFontData masterData) {
    super(data);
    this.masterData = masterData;
  }

  /**
   * Constructor.
   *
   * @param data the data representing the subtable
   */
  protected SubTable(ReadableFontData data) {
    this(data, null);
  }

  /**
   * Constructor.
   *
   * @param data the data object that contains the subtable
   * @param offset the offset within the data where the subtable starts
   * @param length the length of the subtable data within the data object
   */
  protected SubTable(ReadableFontData data, int offset, int length) {
    this(data.slice(offset, length));
  }

  protected ReadableFontData masterReadData() {
    return this.masterData;
  }

  /**
   * An abstract base class for subtable builders.
   *
   * @param <T> the type of the subtable
   */
  protected abstract static class Builder<T extends SubTable> extends FontDataTable.Builder<T> {
    private ReadableFontData masterData;

    /**
     * Constructor.
     *
     * @param data the data for the subtable being built
     * @param masterData the data for the full table
     */
    protected Builder(WritableFontData data, ReadableFontData masterData) {
      super(data);
      this.masterData = masterData;
    }

    /**
     * Constructor.
     *
     * @param data the data for the subtable being built
     * @param masterData the data for the full table
     */
    protected Builder(ReadableFontData data, ReadableFontData masterData) {
      super(data);
      this.masterData = masterData;
    }

    /**
     * Constructor.
     *
     * @param data the data for the subtable being built
     */
    protected Builder(WritableFontData data) {
      super(data);
    }

    /**
     * Constructor.
     *
     * @param data the data for the subtable being built
     */
    protected Builder(ReadableFontData data) {
      super(data);
    }

    /**
     * Constructor.
     *
     * Creates a new empty sub-table.
     *
     * @param dataSize the initial size for the data; if it is positive then the
     *        size is fixed; if it is negative then it is variable sized
     */
    protected Builder(int dataSize) {
      super(dataSize);
    }

    protected ReadableFontData masterReadData() {
      return this.masterData;
    }
  }

  /**
   * Get the number of bytes of padding used in the table. The padding bytes are
   * used to align the table length to a 4 byte boundary.
   *
   * @return the number of padding bytes
   */
  public int padding() {
    return this.padding;
  }

  /**
   * Sets the amount of padding that is part of the data being used by this
   * subtable.
   *
   * @param padding
   */
  // TODO(stuartg): move to constructor
  protected void setPadding(int padding) {
    this.padding = padding;
  }
}