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
 * A Maximum Profile table - 'maxp'.
 *
 * @author Stuart Gill
 */
public final class MaximumProfileTable extends Table {

  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
   */
  private enum Offset {
    // version 0.5 and 1.0
    version(0),
    numGlyphs(4),

    // version 1.0
    maxPoints(6),
    maxContours(8),
    maxCompositePoints(10),
    maxCompositeContours(12),
    maxZones(14),
    maxTwilightPoints(16),
    maxStorage(18),
    maxFunctionDefs(20),
    maxInstructionDefs(22),
    maxStackElements(24),
    maxSizeOfInstructions(26),
    maxComponentElements(28),
    maxComponentDepth(30);

    private final int offset;
    private Offset(int offset) {
      this.offset = offset;
    }
  }

  private MaximumProfileTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  public int tableVersion() {
    return this.data.readFixed(Offset.version.offset);
  }

  public int numGlyphs() {
    return this.data.readUShort(Offset.numGlyphs.offset);
  }

  public int maxPoints() {
    return this.data.readUShort(Offset.maxPoints.offset);
  }

  public int maxContours() {
    return this.data.readUShort(Offset.maxContours.offset);
  }

  public int maxCompositePoints() {
    return this.data.readUShort(Offset.maxCompositePoints.offset);
  }
  
  public int maxCompositeContours() {
    return this.data.readUShort(Offset.maxCompositeContours.offset);
  }

  public int maxZones() {
    return this.data.readUShort(Offset.maxZones.offset);
  }

  public int maxTwilightPoints() {
    return this.data.readUShort(Offset.maxTwilightPoints.offset);
  }

  public int maxStorage() {
    return this.data.readUShort(Offset.maxStorage.offset);
  }

  public int maxFunctionDefs() {
    return this.data.readUShort(Offset.maxFunctionDefs.offset);
  }

  public int maxStackElements() {
    return this.data.readUShort(Offset.maxStackElements.offset);
  }

  public int maxSizeOfInstructions() {
    return this.data.readUShort(Offset.maxSizeOfInstructions.offset);
  }

  public int maxComponentElements() {
    return this.data.readUShort(Offset.maxComponentElements.offset);
  }

  public int maxComponentDepth() {
    return this.data.readUShort(Offset.maxComponentDepth.offset);
  }

  /**
   * Builder for a Maximum Profile table - 'maxp'.
   *
   */
  public static class 
  Builder extends TableBasedTableBuilder<MaximumProfileTable> {

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
    protected MaximumProfileTable subBuildTable(ReadableFontData data) {
      return new MaximumProfileTable(this.header(), data);
    }

    public int tableVersion() {
      return this.internalReadData().readUShort(Offset.version.offset);
    }

    public void setTableVersion(int version) {
      this.internalWriteData().writeUShort(Offset.version.offset, version);
    }

    public int numGlyphs() {
      return this.internalReadData().readUShort(Offset.numGlyphs.offset);
    }

    public void setNumGlyphs(int numGlyphs) {
      this.internalWriteData().writeUShort(Offset.numGlyphs.offset, numGlyphs);
    }

    public int maxPoints() {
      return this.internalReadData().readUShort(Offset.maxPoints.offset);
    }

    public void maxPoints(int maxPoints) {
      this.internalWriteData().writeUShort(Offset.maxPoints.offset, maxPoints);
    }

    public int maxContours() {
      return this.internalReadData().readUShort(Offset.maxContours.offset);
    }

    public void setMaxContours(int maxContours) {
      this.internalWriteData().writeUShort(Offset.maxContours.offset, maxContours);
    }

    public int maxCompositePoints() {
      return this.internalReadData().readUShort(Offset.maxCompositePoints.offset);
    }

    public void setMaxCompositePoints(int maxCompositePoints) {
      this.internalWriteData().writeUShort(Offset.maxCompositePoints.offset, maxCompositePoints);
    }
    
    public int maxCompositeContours() {
      return this.internalReadData().readUShort(Offset.maxCompositeContours.offset);
    }

    public void setMaxCompositeContours(int maxCompositeContours) {
      this.internalWriteData().writeUShort(
          Offset.maxCompositeContours.offset, maxCompositeContours);
    }

    public int maxZones() {
      return this.internalReadData().readUShort(Offset.maxZones.offset);
    }

    public void setMaxZones(int maxZones) {
      this.internalWriteData().writeUShort(Offset.maxZones.offset, maxZones);
    }

    public int maxTwilightPoints() {
      return this.internalReadData().readUShort(Offset.maxTwilightPoints.offset);
    }

    public void setMaxTwilightPoints(int maxTwilightPoints) {
      this.internalWriteData().writeUShort(Offset.maxTwilightPoints.offset, maxTwilightPoints);
    }

    public int maxStorage() {
      return this.internalReadData().readUShort(Offset.maxStorage.offset);
    }

    public void setMaxStorage(int maxStorage) {
      this.internalWriteData().writeUShort(Offset.maxStorage.offset, maxStorage);
    }

    public int maxFunctionDefs() {
      return this.internalReadData().readUShort(Offset.maxFunctionDefs.offset);
    }

    public void setMaxFunctionDefs(int maxFunctionDefs) {
      this.internalWriteData().writeUShort(Offset.maxFunctionDefs.offset, maxFunctionDefs);
    }

    public int maxStackElements() {
      return this.internalReadData().readUShort(Offset.maxStackElements.offset);
    }

    public void setMaxStackElements(int maxStackElements) {
      this.internalWriteData().writeUShort(Offset.maxStackElements.offset, maxStackElements);
    }

    public int maxSizeOfInstructions() {
      return this.internalReadData().readUShort(Offset.maxSizeOfInstructions.offset);
    }

    public void setMaxSizeOfInstructions(int maxSizeOfInstructions) {
      this.internalWriteData().writeUShort(
          Offset.maxSizeOfInstructions.offset, maxSizeOfInstructions);
    }

    public int maxComponentElements() {
      return this.internalReadData().readUShort(Offset.maxComponentElements.offset);
    }

    public void setMaxComponentElements(int maxComponentElements) {
      this.internalWriteData().writeUShort(
          Offset.maxComponentElements.offset, maxComponentElements);
    }

    public int maxComponentDepth() {
      return this.internalReadData().readUShort(Offset.maxComponentDepth.offset);
    }

    public void setMaxComponentDepth(int maxComponentDepth) {
      this.internalWriteData().writeUShort(Offset.maxComponentDepth.offset, maxComponentDepth);
    }
  }
}
