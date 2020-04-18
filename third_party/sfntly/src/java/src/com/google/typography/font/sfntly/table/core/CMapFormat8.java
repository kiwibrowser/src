package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cmap format 8 sub table.
 *
 */
public final class CMapFormat8 extends CMap {
  private final int numberOfGroups;

  protected CMapFormat8(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format8.value, cmapId);
    this.numberOfGroups = this.data.readULongAsInt(Offset.format8nGroups.offset);
  }

  private int firstChar(int groupIndex) {
    return this.readFontData().readULongAsInt(
        Offset.format8Groups.offset + groupIndex * Offset.format8Group_structLength.offset
            + Offset.format8Group_startCharCode.offset);
  }

  private int endChar(int groupIndex) {
    return this.readFontData().readULongAsInt(
        Offset.format8Groups.offset + groupIndex * Offset.format8Group_structLength.offset
            + Offset.format8Group_endCharCode.offset);
  }

  @Override
  public int glyphId(int character) {
    return this.readFontData().searchULong(Offset.format8Groups.offset
        + Offset.format8Group_startCharCode.offset,
        Offset.format8Group_structLength.offset,
        Offset.format8Groups.offset + Offset.format8Group_endCharCode.offset,
        Offset.format8Group_structLength.offset,
        numberOfGroups,
        character);
  }

  @Override
  public int language() {
    return this.data.readULongAsInt(Offset.format8Language.offset);
  }

  @Override
  public Iterator<Integer> iterator() {
    return new CharacterIterator();
  }

  private class CharacterIterator implements Iterator<Integer> {
    private int groupIndex;
    private int firstCharInGroup;
    private int endCharInGroup;

    private int nextChar;
    private boolean nextCharSet;

    private CharacterIterator() {
      groupIndex = 0;
      firstCharInGroup = -1;
    }

    @Override
    public boolean hasNext() {
      if (nextCharSet == true) {
        return true;
      }
      while (groupIndex < numberOfGroups) {
        if (firstCharInGroup < 0) {
          firstCharInGroup = firstChar(groupIndex);
          endCharInGroup = endChar(groupIndex);
          nextChar = firstCharInGroup;
          nextCharSet = true;
          return true;
        }
        if (nextChar < endCharInGroup) {
          nextChar++;
          nextCharSet = true;
          return true;
        }
        groupIndex++;
        firstCharInGroup = -1;
      }
      return false;
    }

    @Override
    public Integer next() {
      if (!nextCharSet) {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
      }
      nextCharSet = false;
      return nextChar;
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException("Unable to remove a character from cmap.");
    }
  }

  public static class Builder extends CMap.Builder<CMapFormat8> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format8Length.offset)), CMapFormat.Format8,
          cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format8Length.offset)), CMapFormat.Format8,
          cmapId);
    }

    @Override
    protected CMapFormat8 subBuildTable(ReadableFontData data) {
      return new CMapFormat8(data, this.cmapId());
    }
  }
}