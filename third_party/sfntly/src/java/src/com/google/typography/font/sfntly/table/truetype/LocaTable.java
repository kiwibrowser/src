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

import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.core.FontHeaderTable.IndexToLocFormat;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * A Loca table - 'loca'.
 *
 * @author Stuart Gill
 */
public final class LocaTable extends Table {

  private IndexToLocFormat version;
  private int numGlyphs;

  private LocaTable(Header header, ReadableFontData data, IndexToLocFormat version, int numGlyphs) {
    super(header, data);
    this.version = version;
    this.numGlyphs = numGlyphs;
  }

  /**
   * Get the table version.
   *
   * @return the table version
   */
  public IndexToLocFormat formatVersion() {
    return this.version;
  }

  public int numGlyphs() {
    return this.numGlyphs;
  }

  /**
   * Return the offset for the given glyph id. Valid glyph ids are from 0 to the
   * one less than the number of glyphs. The zero entry is the special entry for
   * the notdef glyph. The final entry beyond the last glyph id is used to
   * calculate the size of the last glyph.
   *
   * @param glyphId the glyph id to get the offset for; must be less than or
   *        equal to one more than the number of glyph ids
   * @return the offset in the glyph table to the specified glyph id
   */
  public int glyphOffset(int glyphId) {
    if (glyphId < 0 || glyphId >= this.numGlyphs) {
      throw new IndexOutOfBoundsException("Glyph ID is out of bounds.");
    }
    return this.loca(glyphId);
  }

  /**
   * Get the length of the data in the glyph table for the specified glyph id.
   * @param glyphId the glyph id to get the offset for; must be greater than or
   *        equal to 0 and less than the number of glyphs in the font
   * @return the length of the data in the glyph table for the specified glyph id
   */
  public int glyphLength(int glyphId) {
    if (glyphId < 0 || glyphId >= this.numGlyphs) {
      throw new IndexOutOfBoundsException("Glyph ID is out of bounds.");
    }
    return this.loca(glyphId + 1) - this.loca(glyphId);
  }

  /**
   * Get the number of locations or locas. This will be one more than the number
   * of glyphs for this table since the last loca position is used to indicate
   * the size of the final glyph.
   *
   * @return the number of locas
   */
  public int numLocas() {
    return this.numGlyphs + 1;
  }

  /**
   * Get the value from the loca table for the index specified. These are the
   * raw values from the table that are used to compute the offset and size of a
   * glyph in the glyph table. Valid index values run from 0 to the number of
   * glyphs in the font.
   *
   * @param index the loca table index
   * @return the loca table value
   */
  public int loca(int index) {
    if (index > this.numGlyphs) {
      throw new IndexOutOfBoundsException();
    }
    if (this.version == IndexToLocFormat.shortOffset) {
      return 2 * this.data.readUShort(index * FontData.DataSize.USHORT.size());
    }
    return this.data.readULongAsInt(index * FontData.DataSize.ULONG.size());
  }

  /**
   * Get an iterator over the loca values for the table. The iterator returned
   * does not support the delete operation.
   *
   * @return loca iterator
   * @see #loca
   */
  Iterator<Integer> iterator() {
    return new LocaIterator();
  }

  /**
   * Iterator over the raw loca values.
   *
   */
  private final class LocaIterator implements Iterator<Integer> {
    int index;

    private LocaIterator() {
    }

    @Override
    public boolean hasNext() {
      if (this.index <= numGlyphs) {
        return true;
      }
      return false;
    }

    @Override
    public Integer next() {
      return loca(index++);
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException();
    }
  }

  /**
   * Builder for a loca table.
   *
   */
  public static class Builder extends Table.Builder<LocaTable> {

    // values that need to be set to properly passe an existing loca table
    private IndexToLocFormat formatVersion = IndexToLocFormat.longOffset;
    private int numGlyphs = -1;
    
    // parsed loca table
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
    
    private Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    private Builder(Header header, ReadableFontData data) {
      super(header, data);
    }

    /**
     * Initialize the internal state from the data. Done lazily since in many
     * cases the builder will be just creating a table object with no parsing
     * required.
     *
     * @param data the data to initialize from
     */
    private void initialize(ReadableFontData data) {
      this.clearLoca(false);
      if (this.loca == null) {
        this.loca = new ArrayList<Integer>();
      }
      if (data != null) {
        if (this.numGlyphs < 0) {
          throw new IllegalStateException("numglyphs not set on LocaTable Builder.");
        }

        LocaTable table = new LocaTable(this.header(), data, this.formatVersion, this.numGlyphs);
        Iterator<Integer> locaIter = table.iterator();
        while (locaIter.hasNext()) {
          this.loca.add(locaIter.next());
        }
      }
    }

    /**
     * Checks that the glyph id is within the correct range.
     *
     * @param glyphId
     * @return the glyphId
     * @throws IndexOutOfBoundsException if the glyph id is not within the
     *         correct range
     */
    private int checkGlyphRange(int glyphId) {
      if (glyphId < 0 || glyphId > this.lastGlyphIndex()) {
        throw new IndexOutOfBoundsException("Glyph ID is outside of the allowed range.");
      }
      return glyphId;
    }

    private int lastGlyphIndex() {
      return this.loca != null ? this.loca.size() - 2 : this.numGlyphs - 1;
    }

    /**
     * Internal method to get the loca list if already generated and if not to
     * initialize the state of the builder.
     *
     * @return the loca list
     */
    private List<Integer> getLocaList() {
      if (this.loca == null) {
        this.initialize(this.internalReadData());
        this.setModelChanged();
      }
      return this.loca;
    }

    private void clearLoca(boolean nullify) {
      if (this.loca != null) {
        this.loca.clear();
      }
      if (nullify) {
        this.loca = null;
      }
      this.setModelChanged(false);
    }
    
    /**
     * Get the format version that will be used when the loca table is
     * generated.
     *
     * @return the loca table format version
     */
    public IndexToLocFormat formatVersion() {
      return this.formatVersion;
    }

    /**
     * Set the format version to be used when generating the loca table.
     *
     * @param formatVersion
     */
    public void setFormatVersion(IndexToLocFormat formatVersion) {
      this.formatVersion = formatVersion;
    }

    /**
     * Gets the List of locas for loca table builder. These may be manipulated
     * in any way by the caller and the changes will be reflected in the final
     * loca table produced as long as no subsequent call is made to the
     * {@link #setLocaList(List)} method.
     *
     *  If there is no current data for the loca table builder or the loca list
     * have not been previously set then this will return an empty List.
     *
     * @return the list of glyph builders
     * @see #setLocaList(List)
     */
    public List<Integer> locaList() {
      return this.getLocaList();
    }

    /**
     * Set the list of locas to be used for building this table. If any existing
     * list was already retrieved with the {@link #locaList()} method then the
     * connection of that previous list to this builder will be broken.
     *
     * @param list
     * @see #locaList()
     */
    public void setLocaList(List<Integer> list) {
      this.loca = list;
      this.setModelChanged();
    }
    
    /**
     * Return the offset for the given glyph id. Valid glyph ids are from 0 to
     * one more than the number of glyphs. The zero entry is the special entry
     * for the notdef glyph. The final entry beyond the last glyph id is used to
     * calculate the size of the last glyph.
     *
     * @param glyphId the glyph id to get the offset for; must be less than or
     *        equal to one more than the number of glyph ids
     * @return the offset in the glyph table to the specified glyph id
     */
    public int glyphOffset(int glyphId) {
      this.checkGlyphRange(glyphId);
      return this.getLocaList().get(glyphId);
    }

    /**
     * Get the length of the data in the glyph table for the specified glyph id.
     * This is a convenience method that uses the specified glyph id
     *
     * @param glyphId the glyph id to get the offset for; must be less than or
     *        equal to the number of glyphs
     * @return the length of the data in the glyph table for the specified glyph
     *         id
     */
    public int glyphLength(int glyphId) {
      this.checkGlyphRange(glyphId);
      return this.getLocaList().get(glyphId + 1) - this.getLocaList().get(glyphId);
    }

    /**
     * Set the number of glyphs.
     *
     *  This method sets the number of glyphs that the builder will attempt to
     * parse location data for from the raw binary data. This method only needs
     * to be called (and <b>must</b> be) when the raw data for this builder has
     * been changed. It does not by itself reset the data or clear any set loca
     * list.
     *
     * @param numGlyphs the number of glyphs represented by the data
     */
    public void setNumGlyphs(int numGlyphs) {
      this.numGlyphs = numGlyphs;
    }

    /**
     * Get the number of glyphs that this builder has support for.
     *
     * @return the number of glyphs.
     */
    public int numGlyphs() {
      return this.lastGlyphIndex() + 1;
    }
    
    /**
     * Revert the loca table builder to the state contained in the last raw data
     * set on the builder. That raw data may be that read from a font file when
     * the font builder was created, that set by a user of the loca table
     * builder, or null data if this builder was created as a new empty builder.
     */
    public void revert() {
      this.loca = null;
      this.setModelChanged(false);
    }

    /**
     * Get the number of locations or locas. This will be one more than the
     * number of glyphs for this table since the last loca position is used to
     * indicate the size of the final glyph.
     *
     * @return the number of locas
     */
    public int numLocas() {
      return this.getLocaList().size();
    }
    
    /**
     * Get the value from the loca table for the index specified. These are the
     * raw values from the table that are used to compute the offset and size of
     * a glyph in the glyph table. Valid index values run from 0 to the number
     * of glyphs in the font.
     *
     * @param index the loca table index
     * @return the loca table value
     */
    public int loca(int index) {
      return this.getLocaList().get(index);
    }

    @Override
    protected LocaTable subBuildTable(ReadableFontData data) {
      return new LocaTable(this.header(), data, this.formatVersion, this.numGlyphs);
    }

    @Override
    protected void subDataSet() {
      this.initialize(this.internalReadData());
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.loca == null) {
        return 0;
      }
      if (this.formatVersion == IndexToLocFormat.longOffset) {
        return this.loca.size() * FontData.DataSize.ULONG.size();
      }
      return this.loca.size() * FontData.DataSize.USHORT.size();
    }

    @Override
    protected boolean subReadyToSerialize() {
      return this.loca != null;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = 0;
      for (int l : this.loca) {
        if (this.formatVersion == IndexToLocFormat.longOffset) {
          size += newData.writeULong(size, l);
        } else {
          size += newData.writeUShort(size, l / 2);
        }
      }
      this.numGlyphs = this.loca.size() - 1;
      return size;
    }
  }
}
