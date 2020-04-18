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

import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.SubTableContainerTable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * @author Stuart Gill
 *
 */
public final class EbdtTable extends SubTableContainerTable {
  protected static enum Offset {
    // header
    version(0), headerLength(FontData.DataSize.Fixed.size());


    protected final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  protected EbdtTable(Header header, ReadableFontData data) {
    super(header, data);
  }

  public int version() {
    return this.data.readFixed(Offset.version.offset);
  }

  public BitmapGlyph glyph(int offset, int length, int format) {
    ReadableFontData glyphData = this.data.slice(offset, length);
    return BitmapGlyph.createGlyph(glyphData, format);
  }

  public static class Builder extends SubTableContainerTable.Builder<EbdtTable> {
    private final int version = 0x00020000; // TODO(stuartg) need a constant/enum
    private List<Map<Integer, BitmapGlyphInfo>> glyphLoca;
    private List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> glyphBuilders;

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
     * Create a new builder using the header information and data provided.
     *
     * @param header the header information
     * @param data the data holding the table
     * @return a new builder
     */
    public static Builder createBuilder(Header header, ReadableFontData data) {
      return new Builder(header, data);
    }

    protected Builder(Header header, WritableFontData data) {
      super(header, data);
    }

    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
    }

    public void setLoca(List<Map<Integer, BitmapGlyphInfo>> locaList) {
      this.revert();
      this.glyphLoca = locaList;
    }

    public List<Map<Integer, BitmapGlyphInfo>> generateLocaList() {
      if (this.glyphBuilders == null) {
        if (this.glyphLoca == null) {
          return new ArrayList<Map<Integer, BitmapGlyphInfo>>(0);
        }
        return this.glyphLoca;
      }

      List<Map<Integer, BitmapGlyphInfo>> newLocaList =
          new ArrayList<Map<Integer, BitmapGlyphInfo>>(this.glyphBuilders.size());

      int startOffset = Offset.headerLength.offset;
      for (Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> builderMap :
          this.glyphBuilders) {
        Map<Integer, BitmapGlyphInfo> newLocaMap = new TreeMap<Integer, BitmapGlyphInfo>();
        int glyphOffset = 0;
        for (Map.Entry<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> glyphEntry :
            builderMap.entrySet()) {
          BitmapGlyph.Builder<? extends BitmapGlyph> builder = glyphEntry.getValue();
          int size = builder.subDataSizeToSerialize();
          BitmapGlyphInfo info = new BitmapGlyphInfo(
              glyphEntry.getKey(), startOffset + glyphOffset, size, builder.format());
          newLocaMap.put(glyphEntry.getKey(), info);
          glyphOffset += size;
        }
        startOffset += glyphOffset;
        newLocaList.add(Collections.unmodifiableMap(newLocaMap));
      }
      return Collections.unmodifiableList(newLocaList);
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
    public List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> glyphBuilders() {
      return this.getGlyphBuilders();
    }

    /**
     * Replace the internal glyph builders with the one provided. The provided
     * list and all contained objects belong to this builder.
     *
     *  This call is only required if the entire set of glyphs in the glyph
     * table builder are being replaced. If the glyph builder list provided from
     * the {@link EbdtTable.Builder#glyphBuilders()} is being used and modified
     * then those changes will already be reflected in the glyph table builder.
     *
     * @param glyphBuilders the new glyph builders
     */
    public void setGlyphBuilders(
        List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> glyphBuilders) {
      this.glyphBuilders = glyphBuilders;
      this.setModelChanged();
    }

    private List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> getGlyphBuilders() {
      if (this.glyphBuilders == null) {
        if (this.glyphLoca == null) {
          throw new IllegalStateException("Loca values not set - unable to parse glyph data.");
        }
        this.glyphBuilders = Builder.initialize(this.internalReadData(), this.glyphLoca);
        this.setModelChanged();
      }
      return this.glyphBuilders;
    }

    public void revert() {
      this.glyphLoca = null;
      this.glyphBuilders = null;
      this.setModelChanged(false);
    }

    private static List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> initialize(
        ReadableFontData data, List<Map<Integer, BitmapGlyphInfo>> locaList) {

      List<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>> glyphBuilderList =
          new ArrayList<Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>>(locaList.size());
      if (data != null) {
        for (Map<Integer, BitmapGlyphInfo> locaMap : locaList) {
          Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> glyphBuilderMap =
              new TreeMap<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>>();
          for (Map.Entry<Integer, BitmapGlyphInfo> entry : locaMap.entrySet()) {
            BitmapGlyphInfo info = entry.getValue();
            BitmapGlyph.Builder<? extends BitmapGlyph> glyphBuilder =
                BitmapGlyph.Builder.createGlyphBuilder(
                    data.slice(info.offset(), info.length()), info.format());
            glyphBuilderMap.put(entry.getKey(), glyphBuilder);
          }
          glyphBuilderList.add(glyphBuilderMap);
        }
      }
      return glyphBuilderList;
    }

    @Override
    protected EbdtTable subBuildTable(ReadableFontData data) {
      return new EbdtTable(this.header(), data);
    }

    @Override
    protected void subDataSet() {
      this.revert();
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (this.glyphBuilders == null || this.glyphBuilders.size() == 0) {
        return 0;
      }

      boolean fixed = true;
      int size = Offset.headerLength.offset;
      for (Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> builderMap :
          this.glyphBuilders) {
        Map<Integer, BitmapGlyphInfo> newLocaMap = new TreeMap<Integer, BitmapGlyphInfo>();
        for (Map.Entry<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> glyphEntry :
            builderMap.entrySet()) {
          BitmapGlyph.Builder<? extends BitmapGlyph> builder = glyphEntry.getValue();
          int glyphSize = builder.subDataSizeToSerialize();
          size += Math.abs(glyphSize);
          fixed = (glyphSize <= 0) ? false : fixed;
        }
      }
      return (fixed ? 1 : -1) * size;
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.glyphBuilders == null) {
        return false;
      }
      return true;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      int size = 0;
      size += newData.writeFixed(Offset.version.offset, this.version);

      for (Map<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> builderMap :
          this.glyphBuilders) {
        Map<Integer, BitmapGlyphInfo> newLocaMap = new TreeMap<Integer, BitmapGlyphInfo>();
        for (Map.Entry<Integer, BitmapGlyph.Builder<? extends BitmapGlyph>> glyphEntry :
            builderMap.entrySet()) {
          BitmapGlyph.Builder<? extends BitmapGlyph> builder = glyphEntry.getValue();
          size += builder.subSerialize(newData.slice(size));
        }
      }
      return size;
    }
  }
}
