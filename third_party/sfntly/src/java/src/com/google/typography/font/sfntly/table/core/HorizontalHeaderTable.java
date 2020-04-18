/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.TableBasedTableBuilder;


/**
 * A Horizontal Header table - 'hhea'.
 *
 * @author Stuart Gill
 */
public final class HorizontalHeaderTable extends Table {

  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
   */
  private enum Offset {
    version(0),
    Ascender(4),
    Descender(6),
    LineGap(8),
    advanceWidthMax(10),
    minLeftSideBearing(12),
    minRightSideBearing(14),
    xMaxExtent(16),
    caretSlopeRise(18),
    caretSlopeRun(20),
    caretOffset(22),
    metricDataFormat(32),
    numberOfHMetrics(34);

    private final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  private HorizontalHeaderTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  public int tableVersion() {
    return this.data.readFixed(Offset.version.offset);
  }

  public int ascender() {
    return this.data.readShort(Offset.Ascender.offset);
  }

  public int descender() {
    return this.data.readShort(Offset.Descender.offset);
  }

  public int lineGap() {
    return this.data.readShort(Offset.LineGap.offset);
  }

  public int advanceWidthMax() {
    return this.data.readUShort(Offset.advanceWidthMax.offset);
  }

  public int minLeftSideBearing() {
    return this.data.readShort(Offset.minLeftSideBearing.offset);
  }

  public int minRightSideBearing() {
    return this.data.readShort(Offset.minRightSideBearing.offset);
  }

  public int xMaxExtent() {
    return this.data.readShort(Offset.xMaxExtent.offset);
  }

  public int caretSlopeRise() {
    return this.data.readShort(Offset.caretSlopeRise.offset);
  }

  public int caretSlopeRun() {
    return this.data.readShort(Offset.caretSlopeRun.offset);
  }

  public int caretOffset() {
    return this.data.readShort(Offset.caretOffset.offset);
  }

  // TODO(stuartg): an enum?
  public int metricDataFormat() {
    return this.data.readShort(Offset.metricDataFormat.offset);
  }

  public int numberOfHMetrics() {
    return this.data.readUShort(Offset.numberOfHMetrics.offset);
  }

  /**
   * Builder for a Horizontal Header table - 'hhea'.
   *
   */
  public static class Builder
  extends TableBasedTableBuilder<HorizontalHeaderTable> {

    /**
     * Create a new builder using the header information and data provided.
     *
     * @param header the header information
     * @param data the data holding the table
     * @return a new builder
     */
    public static Builder createBuilder(Header header, WritableFontData data) {
      return new Builder(header, data);
    }

    protected Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
    }

    @Override
    protected HorizontalHeaderTable subBuildTable(ReadableFontData data) {
      return new HorizontalHeaderTable(this.header(), data);
    }

    public int tableVersion() {
      return this.internalReadData().readFixed(Offset.version.offset);
    }

    public void setTableVersion(int version) {
      this.internalWriteData().writeFixed(Offset.version.offset, version);
    }

    public int ascender() {
      return this.internalReadData().readShort(Offset.Ascender.offset);
    }

    public void setAscender(int version) {
      this.internalWriteData().writeShort(Offset.Ascender.offset, version);
    }

    public int descender() {
      return this.internalReadData().readShort(Offset.Descender.offset);
    }

    public void setDescender(int version) {
      this.internalWriteData().writeShort(Offset.Descender.offset, version);
    }

    public int lineGap() {
      return this.internalReadData().readShort(Offset.LineGap.offset);
    }

    public void setLineGap(int version) {
      this.internalWriteData().writeShort(Offset.LineGap.offset, version);
    }

    public int advanceWidthMax() {
      return this.internalReadData().readUShort(Offset.advanceWidthMax.offset);
    }

    public void setAdvanceWidthMax(int version) {
      this.internalWriteData().writeUShort(Offset.advanceWidthMax.offset, version);
    }

    public int minLeftSideBearing() {
      return this.internalReadData().readShort(Offset.minLeftSideBearing.offset);
    }

    public void setMinLeftSideBearing(int version) {
      this.internalWriteData().writeShort(Offset.minLeftSideBearing.offset, version);
    }

    public int minRightSideBearing() {
      return this.internalReadData().readShort(Offset.minRightSideBearing.offset);
    }

    public void setMinRightSideBearing(int version) {
      this.internalWriteData().writeShort(Offset.minRightSideBearing.offset, version);
    }

    public int xMaxExtent() {
      return this.internalReadData().readShort(Offset.xMaxExtent.offset);
    }

    public void setXMaxExtent(int version) {
      this.internalWriteData().writeShort(Offset.xMaxExtent.offset, version);
    }

    public int caretSlopeRise() {
      return this.internalReadData().readUShort(Offset.caretSlopeRise.offset);
    }

    public void setCaretSlopeRise(int version) {
      this.internalWriteData().writeUShort(Offset.caretSlopeRise.offset, version);
    }

    public int caretSlopeRun() {
      return this.internalReadData().readUShort(Offset.caretSlopeRun.offset);
    }

    public void setCaretSlopeRun(int version) {
      this.internalWriteData().writeUShort(Offset.caretSlopeRun.offset, version);
    }

    public int caretOffset() {
      return this.internalReadData().readUShort(Offset.caretOffset.offset);
    }

    public void setCaretOffset(int version) {
      this.internalWriteData().writeUShort(Offset.caretOffset.offset, version);
    }

    // TODO(stuartg): an enum?
    public int metricDataFormat() {
      return this.internalReadData().readUShort(Offset.metricDataFormat.offset);
    }

    public void setMetricDataFormat(int version) {
      this.internalWriteData().writeUShort(Offset.metricDataFormat.offset, version);
    }

    public int numberOfHMetrics() {
      return this.internalReadData().readUShort(Offset.numberOfHMetrics.offset);
    }

    public void setNumberOfHMetrics(int version) {
      this.internalWriteData().writeUShort(Offset.numberOfHMetrics.offset, version);
    }
  }
}
