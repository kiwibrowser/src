package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;

/**
 * A cmap format 14 sub table.
 */
// TODO(stuartg): completely unsupported yet
public final class CMapFormat14 extends CMap {

  protected CMapFormat14(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format14.value, cmapId);
  }

  @Override
  public int glyphId(int character) {
    return CMapTable.NOTDEF;
  }

  @Override
  public int language() {
    return 0;
  }

  @Override
  public Iterator<Integer> iterator() {
    return null;
  }

  public static class Builder extends CMap.Builder<CMapFormat14> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format14Length.offset)),
          CMapFormat.Format14, cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format14Length.offset)),
          CMapFormat.Format14, cmapId);
    }

    @Override
    protected CMapFormat14 subBuildTable(ReadableFontData data) {
      return new CMapFormat14(data, this.cmapId());
    }
  }
}