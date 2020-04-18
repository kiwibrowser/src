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

package com.google.typography.font.tools.conversion.eot;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;

/**
 * Builder for "head" table. Most of the fields will be initialized from the head table of an
 * existing font, and setters are provided for fields that will change in the subsetted font.
 * 
 * @author Raph Levien
 */
public class MtxHeadBuilder {
  private static final int HEAD_TABLE_SIZE = 54;

  /**
   * Offsets adapted from sfntly {@link FontHeaderTable}. Note that offsets are capitalized
   * according to the OpenType spec, rather than the usual Java constant convention.
   */
  private enum Offset {
    tableVersion(0),
    fontRevision(4),
    checkSumAdjustment(8),
    magicNumber(12),
    flags(16),
    unitsPerEm(18),
    created(20),
    modified(28),
    xMin(36),
    yMin(38),
    xMax(40),
    yMax(42),
    macStyle(44),
    lowestRecPPEM(46),
    fontDirectionHint(48),
    indexToLocFormat(50),
    glyphDataFormat(52);
    
    private final int offset;
    
    private Offset(int offset) {
      this.offset = offset;
    }
  }

  private final WritableFontData data;
  
  public MtxHeadBuilder() {
    data = WritableFontData.createWritableFontData(HEAD_TABLE_SIZE);
  }

  public void initFrom(FontHeaderTable src) {
    if (src == null) {
      throw new IllegalArgumentException("source table must not be null");
    }
    src.readFontData().slice(0, HEAD_TABLE_SIZE).copyTo(data);
  }

  public MtxHeadBuilder setIndexToLOCFormat(int fmt) {
    this.data.writeUShort(Offset.indexToLocFormat.offset, fmt);
    return this;
  }
  
  public ReadableFontData build() {
    return data;
  }
}
