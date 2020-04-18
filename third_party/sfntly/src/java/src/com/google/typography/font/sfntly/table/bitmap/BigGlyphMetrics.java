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
public class BigGlyphMetrics extends GlyphMetrics {

  static enum Offset {
    metricsLength(8),

    height(0),
    width(1),
    horiBearingX(2),
    horiBearingY(3),
    horiAdvance(4),
    vertBearingX(5),
    vertBearingY(6),
    vertAdvance(7);

    final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  /**
   * @param data
   */
  BigGlyphMetrics(ReadableFontData data) {
    super(data);
  }

  public int height() {
    return this.data.readByte(Offset.height.offset);
  }

  public int width() {
    return this.data.readByte(Offset.width.offset);
  }

  public int horiBearingX() {
    return this.data.readChar(Offset.horiBearingX.offset);
  }

  public int horiBearingY() {
    return this.data.readChar(Offset.horiBearingY.offset);
  }

  public int horiAdvance() {
    return this.data.readByte(Offset.horiAdvance.offset);
  }

  public int vertBearingX() {
    return this.data.readChar(Offset.vertBearingX.offset);
  }

  public int vertBearingY() {
    return this.data.readChar(Offset.vertBearingY.offset);
  }

  public int vertAdvance() {
    return this.data.readByte(Offset.vertAdvance.offset);
  }

  public static class Builder extends GlyphMetrics.Builder<BigGlyphMetrics> {

    public static Builder createBuilder() {
      WritableFontData data = WritableFontData.createWritableFontData(Offset.metricsLength.offset);
      return new Builder(data);
    }

    /**
     * Constructor.
     *
     * @param data
     */
    protected Builder(WritableFontData data) {
      super(data);
    }

    /**
     * Constructor.
     *
     * @param data
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

    public int horiBearingX() {
      return this.internalReadData().readChar(Offset.horiBearingX.offset);
    }

    public void setHoriBearingX(byte bearing) {
      this.internalWriteData().writeChar(Offset.horiBearingX.offset, bearing);
    }

    public int horiBearingY() {
      return this.internalReadData().readChar(Offset.horiBearingY.offset);
    }

    public void setHoriBearingY(byte bearing) {
      this.internalWriteData().writeChar(Offset.horiBearingY.offset, bearing);
    }

    public int horiAdvance() {
      return this.internalReadData().readByte(Offset.horiAdvance.offset);
    }

    public void setHoriAdvance(byte advance) {
      this.internalWriteData().writeByte(Offset.horiAdvance.offset, advance);
    }

    public int vertBearingX() {
      return this.internalReadData().readChar(Offset.vertBearingX.offset);
    }

    public void setVertBearingX(byte bearing) {
      this.internalWriteData().writeChar(Offset.vertBearingX.offset, bearing);
    }

    public int vertBearingY() {
      return this.internalReadData().readChar(Offset.vertBearingY.offset);
    }

    public void setVertBearingY(byte bearing) {
      this.internalWriteData().writeChar(Offset.vertBearingY.offset, bearing);
    }

    public int vertAdvance() {
      return this.internalReadData().readByte(Offset.vertAdvance.offset);
    }

    public void setVertAdvance(byte advance) {
      this.internalWriteData().writeByte(Offset.vertAdvance.offset, advance);
    }

    @Override
    protected BigGlyphMetrics subBuildTable(ReadableFontData data) {
      return new BigGlyphMetrics(data);
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
