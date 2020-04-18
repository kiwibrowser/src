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

package com.google.typography.font.sfntly.table.truetype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.SubTableContainerTable;

import java.util.ArrayList;
import java.util.List;

/**
 * A Glyph table.
 *
 * @author Stuart Gill
 */
public final class GlyphTable extends SubTableContainerTable {

  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
   */
  public enum Offset {
    // header
    numberOfContours(0),
    xMin(2),
    yMin(4),
    xMax(6),
    yMax(8),

    // Simple Glyph Description
    simpleEndPtsOfCountours(10),
    // offset from the end of the contours array
    simpleInstructionLength(0),
    simpleInstructions(2),
    // flags
    // xCoordinates
    // yCoordinates

    // Composite Glyph Description
    compositeFlags(0),
    compositeGyphIndexWithoutFlag(0),
    compositeGlyphIndexWithFlag(2);

    final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  private GlyphTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  public Glyph glyph(int offset, int length) {
    return Glyph.getGlyph(this, this.data, offset, length);
  }

  public static class Builder extends SubTableContainerTable.Builder<GlyphTable> {

    private List<Glyph.Builder<? extends Glyph>> glyphBuilders;
    private List<Integer> loca;

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

    /**
     * Constructor.
     *
     * @param header the table header
     * @param data the data for the table
     */
    protected Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    /**
     * Constructor.
     *
     * @param header the table header
     * @param data the data for the table
     */
    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
    }

    // glyph table level building

    public void setLoca(List<Integer> loca) {
      this.loca = new ArrayList<Integer>(loca);
      this.setModelChanged(false);
      this.glyphBuilders = null;
    }

    /**
     * Generate a loca table list from the current state of the glyph table
     * builder.
     *
     * @return a list of loca information for the glyphs
     */
    public List<Integer> generateLocaList() {
      List<Integer> locas = new ArrayList<Integer>(this.getGlyphBuilders().size());
      locas.add(0);
      if (this.getGlyphBuilders().size() == 0) {
        locas.add(0);
      } else {
        int total = 0;
        for (Glyph.Builder<? extends Glyph> b : this.getGlyphBuilders()) {
          int size = b.subDataSizeToSerialize();
          locas.add(total + size);
          total += size;
        }
      }
      return locas;
    }

    private void initialize(ReadableFontData data, List<Integer> loca) {
      this.glyphBuilders = new ArrayList<Glyph.Builder<? extends Glyph>>();

      if (data != null) {
        int locaValue;
        int lastLocaValue = loca.get(0);
        for (int i = 1; i < loca.size(); i++) {
          locaValue = loca.get(i);
          this.glyphBuilders.add(Glyph.Builder.getBuilder(this, data, lastLocaValue /* offset */,
              locaValue - lastLocaValue /* length */));
          lastLocaValue = locaValue;
        }
      }
    }

    private List<Glyph.Builder<? extends Glyph>> getGlyphBuilders() {
      if (this.glyphBuilders == null) {
        if (this.internalReadData() != null && this.loca == null) {
          throw new IllegalStateException("Loca values not set - unable to parse glyph data.");
        }
        this.initialize(this.internalReadData(), this.loca);
        this.setModelChanged();
      }
      return this.glyphBuilders;
    }

    public void revert() {
      this.glyphBuilders = null;
      this.setModelChanged(false);
    }

    /**
     * Gets the List of glyph builders for the glyph table builder. These may be
     * manipulated in any way by the caller and the changes will be reflected in
     * the final glyph table produced.
     *
     *  If there is no current data for the glyph builder or the glyph builders
     * have not been previously set then this will return an empty glyph builder
     * List. If there is current data (i.e. data read from an existing font) and
     * the <code>loca</code> list has not been set or is null, empty, or
     * invalid, then an empty glyph builder List will be returned.
     *
     * @return the list of glyph builders
     */
    public List<Glyph.Builder<? extends Glyph>> glyphBuilders() {
      return this.getGlyphBuilders();
    }

    /**
     * Replace the internal glyph builders with the one provided. The provided
     * list and all contained objects belong to this builder.
     *
     *  This call is only required if the entire set of glyphs in the glyph
     * table builder are being replaced. If the glyph builder list provided from
     * the {@link GlyphTable.Builder#glyphBuilders()} is being used and modified
     * then those changes will already be reflected in the glyph table builder.
     *
     * @param glyphBuilders the new glyph builders
     */
    public void setGlyphBuilders(List<Glyph.Builder<? extends Glyph>> glyphBuilders) {
      this.glyphBuilders = glyphBuilders;
      this.setModelChanged();
    }

    // glyph builder factories

    public Glyph.Builder<? extends Glyph> glyphBuilder(ReadableFontData data) {
      Glyph.Builder<? extends Glyph> glyphBuilder = Glyph.Builder.getBuilder(this, data);
      return glyphBuilder;
    }


    // internal API for building

    @Override
    protected GlyphTable subBuildTable(ReadableFontData data) {
      return new GlyphTable(this.header(), data);
    }

    @Override
    protected void subDataSet() {
      this.glyphBuilders = null;
      super.setModelChanged(false);
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.glyphBuilders == null || this.glyphBuilders.size() == 0) {
        return 0;
      }

      boolean variable = false;
      int size = 0;

      // calculate size of each table
      for (Glyph.Builder<? extends Glyph> b : this.glyphBuilders) {
        int glyphSize = b.subDataSizeToSerialize();
        size += Math.abs(glyphSize);
        variable |= glyphSize <= 0;
      }
      return variable ? -size : size;
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.glyphBuilders == null) {
        return false;
      }
      // TODO(stuartg): check glyphs for ready to build?
      return true;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = 0;
      for (Glyph.Builder<? extends Glyph> b : this.glyphBuilders) {
        size += b.subSerialize(newData.slice(size));
      }
      return size;
    }
  }
}
