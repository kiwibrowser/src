package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cmap format 13 sub table.
 */
public final class CMapFormat13 extends CMap {
  private final int numberOfGroups;

  protected CMapFormat13(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format12.value, cmapId);
    this.numberOfGroups = this.data.readULongAsInt(Offset.format12nGroups.offset);
  }

  private int groupStartChar(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format13Groups.offset + groupIndex * Offset.format13Groups_structLength.offset
            + Offset.format13_startCharCode.offset);
  }

  private int groupEndChar(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format13Groups.offset + groupIndex * Offset.format13Groups_structLength.offset
            + Offset.format13_endCharCode.offset);
  }

  private int groupGlyph(int groupIndex) {
    return this.data.readULongAsInt(
        Offset.format13Groups.offset + groupIndex * Offset.format13Groups_structLength.offset
            + Offset.format13_glyphId.offset);
  }

  @Override
  public int glyphId(int character) {
    int group = this.data.searchULong(
        Offset.format13Groups.offset + Offset.format13_startCharCode.offset,
        Offset.format13Groups_structLength.offset,
        Offset.format13Groups.offset + Offset.format13_endCharCode.offset,
        Offset.format13Groups_structLength.offset,
        this.numberOfGroups,
        character);
    if (group == -1) {
      return CMapTable.NOTDEF;
    }
    return groupGlyph(group);
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

  public static class Builder extends CMap.Builder<CMapFormat13> {
    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format13Length.offset)),
          CMapFormat.Format13, cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readULongAsInt(offset + Offset.format13Length.offset)),
          CMapFormat.Format13, cmapId);
    }

    @Override
    protected CMapFormat13 subBuildTable(ReadableFontData data) {
      return new CMapFormat13(data, this.cmapId());
    }
  }
}