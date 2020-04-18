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

import java.util.Comparator;

/**
 * An immutable class holding bitmap glyph information.
 *
 * @author Stuart Gill
 */
public final class BitmapGlyphInfo {
  private final int glyphId;
  private final boolean relative;
  private final int blockOffset;
  private final int startOffset;
  private final int length;
  private final int format;

  /**
   * Constructor for a relative located glyph. The glyph's position in the EBDT
   * table is a combination of it's block offset and it's own start offset.
   *
   * @param glyphId the glyph id
   * @param blockOffset the offset of the block to which the glyph belongs
   * @param startOffset the offset of the glyph within the block
   * @param length the byte length
   * @param format the glyph image format
   */
  public BitmapGlyphInfo(int glyphId, int blockOffset, int startOffset, int length, int format) {
    this.glyphId = glyphId;
    this.relative = true;
    this.blockOffset = blockOffset;
    this.startOffset = startOffset;
    this.length = length;
    this.format = format;
  }

  /**
   * Constructor for an absolute located glyph. The glyph's position in the EBDT
   * table is only given by it's own start offset.
   *
   * @param glyphId the glyph id
   * @param startOffset the offset of the glyph within the block
   * @param length the byte length
   * @param format the glyph image format
   */
  public BitmapGlyphInfo(int glyphId, int startOffset, int length, int format) {
    this.glyphId = glyphId;
    this.relative = false;
    this.blockOffset = 0;
    this.startOffset = startOffset;
    this.length = length;
    this.format = format;
  }

  public int glyphId() {
    return this.glyphId;
  }
  
  public boolean relative() {
    return this.relative;
  }
  
  public int blockOffset() {
    return this.blockOffset;
  }

  public int offset() {
    return this.blockOffset() + this.startOffset();
  }
  
  public int startOffset() {
    return this.startOffset;
  }
  
  public int length() {
    return this.length;
  }
  
  public int format() {
    return this.format;
  }
  
  @Override
  public int hashCode() {
    final int prime = 31;
    int result = 1;
    result = prime * result + blockOffset;
    result = prime * result + format;
    result = prime * result + glyphId;
    result = prime * result + length;
    result = prime * result + startOffset;
    return result;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if (obj == null) {
      return false;
    }
    if (!(obj instanceof BitmapGlyphInfo)) {
      return false;
    }
    BitmapGlyphInfo other = (BitmapGlyphInfo) obj;
    if (this.format != other.format) {
      return false;
    }
    if (this.glyphId != other.glyphId) {
      return false;
    }
    if (this.length != other.length) {
      return false;
    }
    if (this.offset() != other.offset()) {
      return false;
    }
    return true;
  }

  public static final Comparator<BitmapGlyphInfo> StartOffsetComparator =
      new StartOffsetComparatorClass();

  private static final class StartOffsetComparatorClass implements Comparator<BitmapGlyphInfo> {
    @Override
    public int compare(BitmapGlyphInfo o1, BitmapGlyphInfo o2) {
      return (o1.startOffset - o2.startOffset);
    }
  }
}
