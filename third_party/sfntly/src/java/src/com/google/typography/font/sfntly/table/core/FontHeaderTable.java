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
import com.google.typography.font.sfntly.table.truetype.LocaTable;

import java.util.EnumSet;

/**
 * A Font Header table.
 * 
 * @author Stuart Gill
 */
public final class FontHeaderTable extends Table {
  
  /**
   * Checksum adjustment base value. To compute the checksum adjustment: 
   * 1) set it to 0; 2) sum the entire font as ULONG, 3) then store 0xB1B0AFBA - sum.
   */
  public static final long CHECKSUM_ADJUSTMENT_BASE = 0xB1B0AFBAL;
  
  /**
   * Magic number value stored in the magic number field.
   */
  public static final long MAGIC_NUMBER = 0x5F0F3CF5L;

  /**
   * The ranges to use for checksum calculation.
   */
  private static final int[] CHECKSUM_RANGES = 
    new int[] {0, Offset.checkSumAdjustment.offset, Offset.magicNumber.offset};
  
  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
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

  /**
   * Constructor.
   *
   * @param header the table header
   * @param data the readable data for the table
   */
  private FontHeaderTable(Header header, ReadableFontData data) {
    super(header, data);
    data.setCheckSumRanges(0, Offset.checkSumAdjustment.offset, Offset.magicNumber.offset);
  }

  /**
   * Get the table version.
   *
   * @return the table version
   */
  public int tableVersion() {
    return this.data.readFixed(Offset.tableVersion.offset);
  }

  /**
   * Get the font revision.
   *
   * @return the font revision
   */
  public int fontRevision() {
    return this.data.readFixed(Offset.fontRevision.offset);
  }

  /**
   * Get the checksum adjustment. To compute: set it to 0, sum the entire font
   * as ULONG, then store 0xB1B0AFBA - sum.
   *
   * @return checksum adjustment
   */
  public long checkSumAdjustment() {
    return this.data.readULong(Offset.checkSumAdjustment.offset);
  }

  /**
   * Get the magic number. Set to 0x5F0F3CF5.
   *
   * @return the magic number
   */
  public long magicNumber() {
    return this.data.readULong(Offset.magicNumber.offset);
  }

  /**
   * Flag values in the font header table.
   *
   */
  public enum Flags {
    BaselineAtY0,
    LeftSidebearingAtX0,
    InstructionsDependOnPointSize,
    ForcePPEMToInteger,
    InstructionsAlterAdvanceWidth,
    //Apple Flags
    Apple_Vertical,
    Apple_Zero,
    Apple_RequiresLayout,
    Apple_GXMetamorphosis,
    Apple_StrongRTL,
    Apple_IndicRearrangement,

    FontDataLossless,
    FontConverted,
    OptimizedForClearType,
    Reserved14,
    Reserved15;

    public int mask() {
      return 1 << this.ordinal();
    }

    public static EnumSet<Flags> asSet(int value) {
      EnumSet<Flags> set = EnumSet.noneOf(Flags.class);
      for (Flags flag : Flags.values()) {
        if ((value & flag.mask()) == flag.mask()) {
          set.add(flag);
        }
      }
      return set;
    }

    static public int value(EnumSet<Flags> set) {
      int value = 0;
      for (Flags flag : set) {
        value |= flag.mask();
      }
      return value;
    }

    static public int cleanValue(EnumSet<Flags> set) {
      EnumSet<Flags> clean = EnumSet.copyOf(set);
      clean.remove(Flags.Reserved14);
      clean.remove(Flags.Reserved15);
      return value(clean);
    }
  }

  /**
   * Get the flags as an int value.
   *
   * @return the flags
   */
  public int flagsAsInt() {
    return this.data.readUShort(Offset.flags.offset);
  }

  /**
   * Get the flags as an enum set.
   *
   * @return the enum set of the flags
   */
  public EnumSet<Flags> flags() {
    return Flags.asSet(this.flagsAsInt());
  }

  /**
   * Get the units per em.
   *
   * @return the units per em
   */
  public int unitsPerEm() {
    return this.data.readUShort(Offset.unitsPerEm.offset);
  }

  /**
   * Get the created date. Number of seconds since 12:00 midnight, January 1,
   * 1904. 64-bit integer.
   *
   * @return created date
   */
  public long created() {
    return this.data.readDateTimeAsLong(Offset.created.offset);
  }

  /**
   * Get the modified date. Number of seconds since 12:00 midnight, January 1,
   * 1904. 64-bit integer.
   *
   * @return created date
   */
  public long modified() {
    return this.data.readDateTimeAsLong(Offset.modified.offset);
  }

  /**
   * Get the x min. For all glyph bounding boxes.
   *
   * @return the x min
   */
  public int xMin() {
    return this.data.readShort(Offset.xMin.offset);
  }

  /**
   * Get the y min. For all glyph bounding boxes.
   *
   * @return the y min
   */
  public int yMin() {
    return this.data.readShort(Offset.yMin.offset);
  }

  /**
   * Get the x max. For all glyph bounding boxes.
   *
   * @return the xmax
   */
  public int xMax() {
    return this.data.readShort(Offset.xMax.offset);
  }

  /**
   * Get the y max. For all glyph bounding boxes.
   *
   * @return the ymax
   */
  public int yMax() {
    return this.data.readShort(Offset.yMax.offset);
  }

  /**
   * Mac style bits set in the font header table.
   *
   */
  public enum MacStyle {
    Bold,
    Italic,
    Underline,
    Outline,
    Shadow,
    Condensed,
    Extended,
    Reserved7,
    Reserved8,
    Reserved9,
    Reserved10,
    Reserved11,
    Reserved12,
    Reserved13,
    Reserved14,
    Reserved15;

    public int mask() {
      return 1 << this.ordinal();
    }

    public static EnumSet<MacStyle> asSet(int value) {
      EnumSet<MacStyle> set = EnumSet.noneOf(MacStyle.class);
      for (MacStyle style : MacStyle.values()) {
        if ((value & style.mask()) == style.mask()) {
          set.add(style);
        }
      }
      return set;
    }

    public static int value(EnumSet<MacStyle> set) {
      int value = 0;
      for (MacStyle style : set) {
        value |= style.mask();
      }
      return value;
    }

    public static int cleanValue(EnumSet<MacStyle> set) {
      EnumSet<MacStyle> clean = EnumSet.copyOf(set);
      clean.removeAll(reserved);
      return value(clean);
    }

    private static final EnumSet<MacStyle> reserved = 
      EnumSet.range(MacStyle.Reserved7, MacStyle.Reserved15);
  }

  /**
   * Get the Mac style bits as an int.
   *
   * @return the Mac style bits
   */
  public int macStyleAsInt() {
    return this.data.readUShort(Offset.macStyle.offset);
  }

  /**
   * Get the Mac style bits as an enum set.
   *
   * @return the Mac style bits
   */
  public EnumSet<MacStyle> macStyle() {
    return MacStyle.asSet(this.macStyleAsInt());
  }

  public int lowestRecPPEM() {
    return this.data.readUShort(Offset.lowestRecPPEM.offset);
  }

  /**
   * Font direction hint values in the font header table.
   *
   */
  public enum FontDirectionHint {
    FullyMixed(0),
    OnlyStrongLTR(1),
    StrongLTRAndNeutral(2),
    OnlyStrongRTL(-1),
    StrongRTLAndNeutral(-2);

    private final int value;

    private FontDirectionHint(int value) {
      this.value = value;
    }

    public int value() {
      return this.value;
    }

    public boolean equals(int value) {
      return value == this.value;
    }

    public static FontDirectionHint valueOf(int value) {
      for (FontDirectionHint hint : FontDirectionHint.values()) {
        if (hint.equals(value)) {
          return hint;
        }
      }
      return null;
    }
  }

  public int fontDirectionHintAsInt() {
    return this.data.readShort(Offset.fontDirectionHint.offset);
  }

  public FontDirectionHint fontDirectionHint() {
    return FontDirectionHint.valueOf(this.fontDirectionHintAsInt());
  }

  /**
   * The index to location format used in the LocaTable.
   *
   * @see LocaTable
   */
  public enum IndexToLocFormat {
    shortOffset(0),
    longOffset(1);

    private final int value;
    
    private IndexToLocFormat(int value) {
      this.value = value;
    }

    public int value() {
      return this.value;
    }

    public boolean equals(int value) {
      return value == this.value;
    }

    public static IndexToLocFormat valueOf(int value) {
      for (IndexToLocFormat format : IndexToLocFormat.values()) {
        if (format.equals(value)) {
          return format;
        }
      }
      return null;
    }
  }

  public int indexToLocFormatAsInt() {
    return this.data.readShort(Offset.indexToLocFormat.offset);
  }

  public IndexToLocFormat indexToLocFormat() {
    return IndexToLocFormat.valueOf(this.indexToLocFormatAsInt());
  }

  public int glyphdataFormat() {
    return this.data.readShort(Offset.glyphDataFormat.offset);
  }

  public static class Builder extends TableBasedTableBuilder<FontHeaderTable> {
    private boolean fontChecksumSet = false;
    private long fontChecksum = 0;
    
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
      data.setCheckSumRanges(0, Offset.checkSumAdjustment.offset, Offset.magicNumber.offset);
    }

    protected Builder(Header header, ReadableFontData data) {
      super(header, data);
      data.setCheckSumRanges(FontHeaderTable.CHECKSUM_RANGES);
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (this.dataChanged()) {
        ReadableFontData data = this.internalReadData();
        data.setCheckSumRanges(FontHeaderTable.CHECKSUM_RANGES);
      }
      if (this.fontChecksumSet) {
        ReadableFontData data = this.internalReadData();
        data.setCheckSumRanges(FontHeaderTable.CHECKSUM_RANGES);
        long checksumAdjustment = 
          FontHeaderTable.CHECKSUM_ADJUSTMENT_BASE - (this.fontChecksum + data.checksum());
        this.setCheckSumAdjustment(checksumAdjustment);
      }
      return super.subReadyToSerialize();
    }

    @Override
    protected FontHeaderTable subBuildTable(ReadableFontData data) {
      return new FontHeaderTable(this.header(), data);
    }

    /**
     * Sets the font checksum to be used when calculating the the checksum
     * adjustment for the header table during build time.
     * 
     * The font checksum is the sum value of all tables but the font header
     * table. If the font checksum has been set then further setting will be
     * ignored until the font check sum has been cleared with
     * {@link #clearFontChecksum()}. Most users will never need to set this. It
     * is used when the font is being built. If set by a client it can interfere
     * with that process.
     * 
     * @param checksum
     *          the font checksum
     */
    public void setFontChecksum(long checksum) {
      if (this.fontChecksumSet) {
        return;
      }
      this.fontChecksumSet = true;
      this.fontChecksum = checksum;
    }
    
    /**
     * Clears the font checksum to be used when calculating the the checksum
     * adjustment for the header table during build time.
     * 
     * The font checksum is the sum value of all tables but the font header
     * table. If the font checksum has been set then further setting will be
     * ignored until the font check sum has been cleared.
     * 
     */
    public void clearFontChecksum() {
      this.fontChecksumSet = false;
    }

    public int tableVersion() {
      return this.table().tableVersion();
    }

    public void setTableVersion(int version) {
      this.internalWriteData().writeFixed(Offset.tableVersion.offset, version);
    }

    public int fontRevision() {
      return this.table().fontRevision();
    }

    public void setFontRevision(int revision) {
      this.internalWriteData().writeFixed(Offset.fontRevision.offset, revision);
    }

    public long checkSumAdjustment() {
      return this.table().checkSumAdjustment();
    }

    public void setCheckSumAdjustment(long adjustment) {
      this.internalWriteData().writeULong(Offset.checkSumAdjustment.offset, adjustment);
    }

    public long magicNumber() {
      return this.table().magicNumber();
    }

    public void setMagicNumber(long magicNumber) {
      this.internalWriteData().writeULong(Offset.magicNumber.offset, magicNumber);
    }

    public int flagsAsInt() {
      return this.table().flagsAsInt();
    }

    public EnumSet<Flags> flags() {
      return this.table().flags();
    }

    public void setFlagsAsInt(int flags) {
      this.internalWriteData().writeUShort(Offset.flags.offset, flags);
    }
    
    public void setFlags(EnumSet<Flags> flags) {
      setFlagsAsInt(Flags.cleanValue(flags));
    }

    public int unitsPerEm() {
      return this.table().unitsPerEm();
    }

    public void setUnitsPerEm(int units) {
      this.internalWriteData().writeUShort(Offset.unitsPerEm.offset, units);
    }

    public long created() {
      return this.table().created();
    }

    public void setCreated(long date) {
      this.internalWriteData().writeDateTime(Offset.created.offset, date);
    }

    public long modified() {
      return this.table().modified();
    }

    public void setModified(long date) {
      this.internalWriteData().writeDateTime(Offset.modified.offset, date);
    }

    public int xMin() {
      return this.table().xMin();
    }

    public void setXMin(int xmin) {
      this.internalWriteData().writeShort(Offset.xMin.offset, xmin);
    }

    public int yMin() {
      return this.table().yMin();
    }

    public void setYMin(int ymin) {
      this.internalWriteData().writeShort(Offset.yMin.offset, ymin);
    }

    public int xMax() {
      return this.table().xMax();
    }

    public void setXMax(int xmax) {
      this.internalWriteData().writeShort(Offset.xMax.offset, xmax);
    }

    public int yMax() {
      return this.table().yMax();
    }

    public void setYMax(int ymax) {
      this.internalWriteData().writeShort(Offset.yMax.offset, ymax);
    }

    public int macStyleAsInt() {
      return this.table().macStyleAsInt();
    }

    public void setMacStyleAsInt(int style) {
      this.internalWriteData().writeUShort(Offset.macStyle.offset, style);
    }

    public EnumSet<MacStyle> macStyle() {
      return this.table().macStyle();
    }

    public void macStyle(EnumSet<MacStyle> style) {
      this.setMacStyleAsInt(MacStyle.cleanValue(style));
    }

    public int lowestRecPPEM() {
      return this.table().lowestRecPPEM();
    }

    public void setLowestRecPPEM(int size) {
      this.internalWriteData().writeUShort(Offset.lowestRecPPEM.offset, size);
    }

    public int fontDirectionHintAsInt() {
      return this.table().fontDirectionHintAsInt();
    }

    public void setFontDirectionHintAsInt(int hint) {
      this.internalWriteData().writeShort(Offset.fontDirectionHint.offset, hint);
    }

    public FontDirectionHint fontDirectionHint() {
      return this.table().fontDirectionHint();
    }

    public void setFontDirectionHint(FontDirectionHint hint) {
      this.setFontDirectionHintAsInt(hint.value());
    }

    public int indexToLocFormatAsInt() {
      return this.table().indexToLocFormatAsInt();
    }

    public void setIndexToLocFormatAsInt(int format) {
      this.internalWriteData().writeShort(Offset.indexToLocFormat.offset, format);
    }

    public IndexToLocFormat indexToLocFormat() {
      return this.table().indexToLocFormat();
    }

    public void setIndexToLocFormat(IndexToLocFormat format) {
      this.setIndexToLocFormatAsInt(format.value());
    }

    public int glyphdataFormat() {
      return this.table().glyphdataFormat();
    }

    public void setGlyphdataFormat(int format) {
      this.internalWriteData().writeShort(Offset.glyphDataFormat.offset, format);
    }
  }
}
