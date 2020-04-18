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

package com.google.typography.font.sfntly.table.bitmap;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

/**
 * @author Stuart Gill
 */
public class SmallGlyphMetrics extends GlyphMetrics {

  static enum Offset {
    metricsLength(5), height(0), width(1), BearingX(2), BearingY(3), Advance(4);

    final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }
  
  private SmallGlyphMetrics(ReadableFontData data) {
    super(data);
  }
  
  public int height() {
    return this.data.readByte(Offset.height.offset);
  }
  
  public int width() {
    return this.data.readByte(Offset.width.offset);
  }

  public int bearingX() {
    return this.data.readChar(Offset.BearingX.offset);
  }

  public int bearingY() {
    return this.data.readChar(Offset.BearingY.offset);
  }

  public int advance() {
    return this.data.readByte(Offset.Advance.offset);
  }

  public static class Builder extends GlyphMetrics.Builder<SmallGlyphMetrics> {

    /**
     * Constructor.
     *
     * @param data the data for the builder
     */
    protected Builder(WritableFontData data) {
      super(data);
    }

    /**
     * Constructor.
     *
     * @param data the data for the builder
     */
    protected Builder(ReadableFontData data) {
      super(data);
    }

    public int height() {
      return this.internalReadData().readByte(Offset.height.offset);
    }

    public void setHeight(byte height) {
      this.internalWriteData().writeByte(Offset.height.offset, height);
    }

    public int width() {
      return this.internalReadData().readByte(Offset.width.offset);
    }

    public void setWidth(byte width) {
      this.internalWriteData().writeByte(Offset.width.offset, width);
    }

    public int bearingX() {
      return this.internalReadData().readChar(Offset.BearingX.offset);
    }

    public void setBearingX(byte bearing) {
      this.internalWriteData().writeChar(Offset.BearingX.offset, bearing);
    }

    public int bearingY() {
      return this.internalReadData().readChar(Offset.BearingY.offset);
    }

    public void setBearingY(byte bearing) {
      this.internalWriteData().writeChar(Offset.BearingY.offset, bearing);
    }

    public int advance() {
      return this.internalReadData().readByte(Offset.Advance.offset);
    }

    public void setAdvance(byte advance) {
      this.internalWriteData().writeByte(Offset.Advance.offset, advance);
    }

    @Override
    protected SmallGlyphMetrics subBuildTable(ReadableFontData data) {
      return new SmallGlyphMetrics(data);
    }

    @Override
    protected void subDataSet() {
      // NOP
    }

    @Override
    protected int subDataSizeToSerialize() {
      return 0;
    }

    @Override
    protected boolean subReadyToSerialize() {
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      return this.data().copyTo(newData);
    }
  }
}
