package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cmap format 12 sub table.
 *
 */
public final class CMapFormat12 extends CMap {
  private final int numberOfGroups;

  protected CMapFormat12(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format12.value, cmapId);
    this.numberOfGroups = this.data.readULongAsInt(Offset.format12nGroups.offset);
  }

  private int groupStartChar(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format12Groups.offset + groupIndex * Offset.format12Groups_structLength.offset
            + Offset.format12_startCharCode.offset);
  }

  private int groupEndChar(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format12Groups.offset + groupIndex * Offset.format12Groups_structLength.offset
            + Offset.format12_endCharCode.offset);
  }

  private int groupStartGlyph(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format12Groups.offset + groupIndex * Offset.format12Groups_structLength.offset
            + Offset.format12_startGlyphId.offset);
  }

  @Override
  public int glyphId(int character) {
    int group =
        this.data.searchULong(Offset.format12Groups.offset + Offset.format12_startCharCode.offset,
            Offset.format12Groups_structLength.offset,
            Offset.format12Groups.offset + Offset.format12_endCharCode.offset,
            Offset.format12Groups_structLength.offset,
            this.numberOfGroups,
            character);
    if (group == -1) {
      return CMapTable.NOTDEF;
    }
    return groupStartGlyph(group) + (character - groupStartChar(group));
  }

  @Override
  public int language() {
    return this.data.readULongAsInt(Offset.format12Language.offset);
  }

  @Override
  public Iterator<Integer> iterator() {
    return new CharacterIterator();
  }

  private final class CharacterIterator implements Iterator<Integer> {
    private int groupIndex = 0;
    private int groupEndChar;

    private boolean nextSet = false;
    private int nextChar;

    private CharacterIterator() {
      nextChar = groupStartChar(groupIndex);
      groupEndChar = groupEndChar(groupIndex);
      nextSet = true;
    }

    @Override
    public boolean hasNext() {
      if (nextSet) {
        return true;
      }
      if (groupIndex >= numberOfGroups) {
        return false;
      }
      if (nextChar < groupEndChar) {
        nextChar++;
        nextSet = true;
        return true;
      }
      groupIndex++;
      if (groupIndex < numberOfGroups) {
        nextSet = true;
        nextChar = groupStartChar(groupIndex);
        groupEndChar = groupEndChar(groupIndex);
        return true;
      }
      return false;
    }

    @Override
    public Integer next() {
      if (!this.nextSet) {
        if (!hasNext()) {
          throw new NoSuchElementException("No more characters to iterate.");
        }
      }
      this.nextSet = false;
      return nextChar;
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException("Unable to remove a character from cmap.");
    }
  }

  public static class Builder extends CMap.Builder<CMapFormat12> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format12Length.offset)),
          CMapFormat.Format12, cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format12Length.offset)),
          CMapFormat.Format12, cmapId);
    }

    @Override
    protected CMapFormat12 subBuildTable(ReadableFontData data) {
      return new CMapFormat12(data, this.cmapId());
    }
  }
}