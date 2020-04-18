package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cmap format 0 sub table.
 *
 */
public final class CMapFormat0 extends CMap {
  protected CMapFormat0(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format0.value, cmapId);
  }

  @Override
  public int glyphId(int character) {
    if (character < 0 || character > 255) {
      return CMapTable.NOTDEF;
    }
    return this.data.readUByte(character + Offset.format0GlyphIdArray.offset);
  }

  @Override
  public int language() {
    return this.data.readUShort(Offset.format0Language.offset);
  }

  @Override
  public Iterator<Integer> iterator() {
    return new CharacterIterator();
  }

  private class CharacterIterator implements Iterator<Integer> {
    int character = 0;
    protected static final int MAX_CHARACTER = 0xff;

    private CharacterIterator() {
    }

    @Override
    public boolean hasNext() {
      if (character <= MAX_CHARACTER) {
        return true;
      }
      return false;
    }

    @Override
    public Integer next() {
      if (!hasNext()) {
        throw new NoSuchElementException("No more characters to iterate.");
      }
      return this.character++;
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException("Unable to remove a character from cmap.");
    }
  }

  public static class Builder extends CMap.Builder<CMapFormat0> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format0Length.offset)), CMapFormat.Format0,
          cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format0Length.offset)), CMapFormat.Format0,
          cmapId);
    }

    @Override
    protected CMapFormat0 subBuildTable(ReadableFontData data) {
      return new CMapFormat0(data, this.cmapId());
    }
  }
}