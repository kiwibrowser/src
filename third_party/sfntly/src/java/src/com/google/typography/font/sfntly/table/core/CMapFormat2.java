package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;

/**
 * A cmap format 2 sub table.
 *
 * The format 2 cmap is used for multi-byte encodings such as SJIS,
 * EUC-JP/KR/CN, Big5, etc.
 */
public final class CMapFormat2 extends CMap {

  protected CMapFormat2(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format2.value, cmapId);
  }

  private int subHeaderOffset(int subHeaderIndex) {
    int subHeaderOffset = this.data.readUShort(
        Offset.format2SubHeaderKeys.offset + subHeaderIndex * FontData.DataSize.USHORT.size());
    return subHeaderOffset;
  }

  private int firstCode(int subHeaderIndex) {
    int subHeaderOffset = subHeaderOffset(subHeaderIndex);
    int firstCode =
        this.data.readUShort(subHeaderOffset + Offset.format2SubHeaderKeys.offset
            + Offset.format2SubHeader_firstCode.offset);
    return firstCode;
  }

  private int entryCount(int subHeaderIndex) {
    int subHeaderOffset = subHeaderOffset(subHeaderIndex);
    int entryCount =
        this.data.readUShort(subHeaderOffset + Offset.format2SubHeaderKeys.offset
            + Offset.format2SubHeader_entryCount.offset);
    return entryCount;
  }

  private int idRangeOffset(int subHeaderIndex) {
    int subHeaderOffset = subHeaderOffset(subHeaderIndex);
    int idRangeOffset = this.data.readUShort(subHeaderOffset + Offset.format2SubHeaderKeys.offset
        + Offset.format2SubHeader_idRangeOffset.offset);
    return idRangeOffset;
  }

  private int idDelta(int subHeaderIndex) {
    int subHeaderOffset = subHeaderOffset(subHeaderIndex);
    int idDelta =
        this.data.readShort(subHeaderOffset + Offset.format2SubHeaderKeys.offset
            + Offset.format2SubHeader_idDelta.offset);
    return idDelta;
  }

  /**
   * Returns how many bytes would be consumed by a lookup of this character
   * with this cmap. This comes about because the cmap format 2 table is
   * designed around multi-byte encodings such as SJIS, EUC-JP, Big5, etc.
   *
   * @param character
   * @return the number of bytes consumed from this "character" - either 1 or
   *         2
   */
  public int bytesConsumed(int character) {
    int highByte = (character >> 8) & 0xff;
    int offset = subHeaderOffset(highByte);

    if (offset == 0) {
      return 1;
    }
    return 2;
  }

  @Override
  public int glyphId(int character) {
    if (character > 0xffff) {
      return CMapTable.NOTDEF;
    }

    int highByte = (character >> 8) & 0xff;
    int lowByte = character & 0xff;
    int offset = subHeaderOffset(highByte);

    // only consume one byte
    if (offset == 0) {
      lowByte = highByte;
      highByte = 0;
    }

    int firstCode = firstCode(highByte);
    int entryCount = entryCount(highByte);

    if (lowByte < firstCode || lowByte >= firstCode + entryCount) {
      return CMapTable.NOTDEF;
    }

    int idRangeOffset = idRangeOffset(highByte);

    // position of idRangeOffset + value of idRangeOffset + index for low byte
    // = firstcode
    int pLocation = (offset + Offset.format2SubHeader_idRangeOffset.offset) + idRangeOffset
        + (lowByte - firstCode) * FontData.DataSize.USHORT.size();
    int p = this.data.readUShort(pLocation);
    if (p == 0) {
      return CMapTable.NOTDEF;
    }

    if (offset == 0) {
      return p;
    }
    int idDelta = idDelta(highByte);
    return (p + idDelta) % 65536;
  }

  @Override
  public int language() {
    return this.data.readUShort(Offset.format2Language.offset);
  }

  @Override
  public Iterator<Integer> iterator() {
    return new CharacterIterator(0, 0xffff);
  }

  public static class Builder extends CMap.Builder<CMapFormat2> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format2Length.offset)), CMapFormat.Format2,
          cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format2Length.offset)), CMapFormat.Format2,
          cmapId);
    }

    @Override
    protected CMapFormat2 subBuildTable(ReadableFontData data) {
      return new CMapFormat2(data, this.cmapId());
    }
  }
}