package com.google.typography.font.sfntly.table.core;

import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.math.FontMath;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.CMapTable.Offset;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * A cmap format 4 sub table.
 */
public final class CMapFormat4 extends CMap {
  private final int segCount;
  private final int glyphIdArrayOffset;

  protected CMapFormat4(ReadableFontData data, CMapId cmapId) {
    super(data, CMapFormat.Format4.value, cmapId);

    this.segCount = this.data.readUShort(Offset.format4SegCountX2.offset) / 2;
    this.glyphIdArrayOffset = glyphIdArrayOffset(this.segCount);
  }

  @Override
  public int glyphId(int character) {
    int segment = this.data.searchUShort(CMapFormat4.startCodeOffset(this.segCount),
        FontData.DataSize.USHORT.size(),
        Offset.format4EndCount.offset,
        FontData.DataSize.USHORT.size(),
        this.segCount,
        character);
    if (segment == -1) {
      return CMapTable.NOTDEF;
    }
    int startCode = startCode(segment);

    return retrieveGlyphId(segment, startCode, character);
  }

  /**
   * Lower level glyph code retrieval that requires processing the Format 4 segments to use.
   *
   * @param segment the cmap segment
   * @param startCode the start code for the segment
   * @param character the character to be looked up
   * @return the glyph id for the character; CMapTable.NOTDEF if not found
   */
  public int retrieveGlyphId(int segment, int startCode, int character) {
    if (character < startCode) {
      return CMapTable.NOTDEF;
    }
    int idRangeOffset = this.idRangeOffset(segment);
    if (idRangeOffset == 0) {
      return (character + this.idDelta(segment)) % 65536;
    }
    int gid = this.data.readUShort(
        idRangeOffset + this.idRangeOffsetLocation(segment) + 2 * (character - startCode));
    if (gid != 0) {
      gid = (gid +  this.idDelta(segment)) % 65536;
    }
    return gid;
  }

  /**
   * Gets the count of the number of segments in this cmap.
   *
   * @return the number of segments
   */
  public int getSegCount() {
    return segCount;
  }

  /**
   * Gets the start code for a segment.
   *
   * @param segment the segment in the look up table
   * @return the start code for the segment
   */
  public int startCode(int segment) {
    isValidIndex(segment);
    return startCode(this.data, this.segCount, segment);
  }

  private static int length(ReadableFontData data) {
    int length = data.readUShort(Offset.format4Length.offset);
    return length;
  }

  private static int segCount(ReadableFontData data) {
    int segCount = data.readUShort(Offset.format4SegCountX2.offset) / 2;
    return segCount;
  }

  private static int startCode(ReadableFontData data, int segCount, int index) {
    int startCode =
        data.readUShort(startCodeOffset(segCount) + index * FontData.DataSize.USHORT.size());
    return startCode;
  }

  private static int startCodeOffset(int segCount) {
    int startCodeOffset =
        Offset.format4EndCount.offset + FontData.DataSize.USHORT.size() + segCount
            * FontData.DataSize.USHORT.size();
    return startCodeOffset;
  }

  private static int endCode(ReadableFontData data, int segCount, int index) {
    int endCode =
        data.readUShort(Offset.format4EndCount.offset + index * FontData.DataSize.USHORT.size());
    return endCode;
  }

  private static int idDelta(ReadableFontData data, int segCount, int index) {
    int idDelta =
        data.readShort(idDeltaOffset(segCount) + index * FontData.DataSize.SHORT.size());
    return idDelta;
  }

  private static int idDeltaOffset(int segCount) {
    int idDeltaOffset =
        Offset.format4EndCount.offset + ((2 * segCount) + 1) * FontData.DataSize.USHORT.size();
    return idDeltaOffset;
  }

  private static int idRangeOffset(ReadableFontData data, int segCount, int index) {
    int idRangeOffset =
        data.readUShort(idRangeOffsetOffset(segCount) + index * FontData.DataSize.USHORT.size());
    return idRangeOffset;
  }

  private static int idRangeOffsetOffset(int segCount) {
    int idRangeOffsetOffset =
        Offset.format4EndCount.offset + ((2 * segCount) + 1) * FontData.DataSize.USHORT.size()
            + segCount * FontData.DataSize.SHORT.size();
    return idRangeOffsetOffset;
  }

  private static int glyphIdArrayOffset(int segCount) {
    int glyphIdArrayOffset =
        Offset.format4EndCount.offset + ((3 * segCount) + 1) * FontData.DataSize.USHORT.size()
            + segCount * FontData.DataSize.SHORT.size();
    return glyphIdArrayOffset;
  }

  /**
   * Gets the end code for a segment.
   *
   * @param segment the segment in the look up table
   * @return the end code for the segment
   */
  public int endCode(int segment) {
    isValidIndex(segment);
    return endCode(this.data, this.segCount, segment);
  }

  private void isValidIndex(int segment) {
    if (segment < 0 || segment >= segCount) {
      throw new IllegalArgumentException();
    }
  }

  /**
   * Gets the id delta for a segment.
   *
   * @param segment the segment in the look up table
   * @return the id delta for the segment
   */
  public int idDelta(int segment) {
    isValidIndex(segment);
    return idDelta(this.data, this.segCount, segment);
  }

  /**
   * Gets the id range offset for a segment.
   *
   * @param segment the segment in the look up table
   * @return the id range offset for the segment
   */
  public int idRangeOffset(int segment) {
    isValidIndex(segment);
    return this.data.readUShort(this.idRangeOffsetLocation(segment));
  }

  /**
   * Get the location of the id range offset for a segment.
   * @param segment the segment in the look up table
   * @return the location of the id range offset for the segment
   */
  public int idRangeOffsetLocation(int segment) {
    isValidIndex(segment);
    return idRangeOffsetOffset(this.segCount) + segment * FontData.DataSize.USHORT.size();
  }

  @SuppressWarnings("unused")
  private int glyphIdArray(int index) {
    return this.data.readUShort(
        this.glyphIdArrayOffset + index * FontData.DataSize.USHORT.size());
  }

  @Override
  public int language() {
    return this.data.readUShort(Offset.format4Language.offset);
  }

  @Override
  public Iterator<Integer> iterator() {
    return new CharacterIterator();
  }

  private class CharacterIterator implements Iterator<Integer> {
    private int segmentIndex;
    private int firstCharInSegment;
    private int lastCharInSegment;

    private int nextChar;
    private boolean nextCharSet;

    private CharacterIterator() {
      segmentIndex = 0;
      firstCharInSegment = -1;
    }

    @Override
    public boolean hasNext() {
      if (nextCharSet == true) {
        return true;
      }
      while (segmentIndex < segCount) {
        if (firstCharInSegment < 0) {
          firstCharInSegment = startCode(segmentIndex);
          lastCharInSegment = endCode(segmentIndex);
          nextChar = firstCharInSegment;
          nextCharSet = true;
          return true;
        }
        if (nextChar < lastCharInSegment) {
          nextChar++;
          nextCharSet = true;
          return true;
        }
        segmentIndex++;
        firstCharInSegment = -1;
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

  public static class Builder extends CMap.Builder<CMapFormat4> {
    public static class Segment {
      public static List<Builder.Segment> deepCopy(List<Builder.Segment> original) {
        List<Builder.Segment> list = new ArrayList<Builder.Segment>(original.size());
        for (Builder.Segment segment : original) {
          list.add(new Segment(segment));
        }
        return list;
      }

      private int startCount;
      private int endCount;
      private int idDelta;
      private int idRangeOffset;

      public Segment() {
      }

      public Segment(Builder.Segment other) {
        this(other.startCount, other.endCount, other.idDelta, other.idRangeOffset);
      }

      public Segment(int startCount, int endCount, int idDelta, int idRangeOffset) {
        this.startCount = startCount;
        this.endCount = endCount;
        this.idDelta = idDelta;
        this.idRangeOffset = idRangeOffset;
      }

      /**
       * @return the startCount
       */
      public int getStartCount() {
        return startCount;
      }

      /**
       * @param startCount the startCount to set
       */
      public void setStartCount(int startCount) {
        this.startCount = startCount;
      }

      /**
       * @return the endCount
       */
      public int getEndCount() {
        return endCount;
      }

      /**
       * @param endCount the endCount to set
       */
      public void setEndCount(int endCount) {
        this.endCount = endCount;
      }

      /**
       * @return the idDelta
       */
      public int getIdDelta() {
        return idDelta;
      }

      /**
       * @param idDelta the idDelta to set
       */
      public void setIdDelta(int idDelta) {
        this.idDelta = idDelta;
      }

      /**
       * @return the idRangeOffset
       */
      public int getIdRangeOffset() {
        return idRangeOffset;
      }

      /**
       * @param idRangeOffset the idRangeOffset to set
       */
      public void setIdRangeOffset(int idRangeOffset) {
        this.idRangeOffset = idRangeOffset;
      }

      @Override
      public String toString() {
        return String.format("[0x%04x - 0x%04x, delta = 0x%04x, rangeOffset = 0x%04x]",
            this.startCount, this.endCount, this.idDelta, this.idRangeOffset);
      }
    }

    private List<Builder.Segment> segments;
    private List<Integer> glyphIdArray;

    protected Builder(WritableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format4Length.offset)), CMapFormat.Format4,
          cmapId);
    }

    protected Builder(ReadableFontData data, int offset, CMapId cmapId) {
      super(data == null ? null : data.slice(
          offset, data.readUShort(offset + Offset.format4Length.offset)), CMapFormat.Format4,
          cmapId);
    }

    private void initialize(ReadableFontData data) {
      this.segments = new ArrayList<Builder.Segment>();
      this.glyphIdArray = new ArrayList<Integer>();

      if (data == null || data.length() == 0) {
        return;
      }

      // build segments
      int segCount = CMapFormat4.segCount(data);
      for (int index = 0; index < segCount; index++) {
        Builder.Segment segment = new Segment();
        segment.setStartCount(CMapFormat4.startCode(data, segCount, index));
        segment.setEndCount(CMapFormat4.endCode(data, segCount, index));
        segment.setIdDelta(CMapFormat4.idDelta(data, segCount, index));
        segment.setIdRangeOffset(CMapFormat4.idRangeOffset(data, segCount, index));

        this.segments.add(segment);
      }

      // build glyph id array
      int glyphIdArrayLength =
          CMapFormat4.length(data) - CMapFormat4.glyphIdArrayOffset(segCount);
      for (int index = 0; index < glyphIdArrayLength; index += FontData.DataSize.USHORT.size()) {
        this.glyphIdArray.add(data.readUShort(index + CMapFormat4.glyphIdArrayOffset(segCount)));
      }
    }

    public List<Builder.Segment> getSegments() {
      if (this.segments == null) {
        this.initialize(this.internalReadData());
        this.setModelChanged();
      }
      return this.segments;
    }

    public void setSegments(List<Builder.Segment> segments) {
      this.segments = Segment.deepCopy(segments);
      this.setModelChanged();
    }

    public List<Integer> getGlyphIdArray() {
      if (this.glyphIdArray == null) {
        this.initialize(this.internalReadData());
        this.setModelChanged();
      }
      return this.glyphIdArray;
    }

    public void setGlyphIdArray(List<Integer> glyphIdArray) {
      this.glyphIdArray = new ArrayList<Integer>(glyphIdArray);
      this.setModelChanged();
    }

    @Override
    protected CMapFormat4 subBuildTable(ReadableFontData data) {
      return new CMapFormat4(data, this.cmapId());
    }

    @Override
    protected void subDataSet() {
      this.segments = null;
      this.glyphIdArray = null;
      super.setModelChanged(false);
    }

    @Override
    protected int subDataSizeToSerialize() {
      if (!this.modelChanged()) {
        return super.subDataSizeToSerialize();
      }

      int size = Offset.format4FixedSize.offset + this.segments.size()
          * (3 * FontData.DataSize.USHORT.size() + FontData.DataSize.SHORT.size())
          + this.glyphIdArray.size() * FontData.DataSize.USHORT.size();
      return size;
    }

    @Override
    protected boolean subReadyToSerialize() {
      if (!this.modelChanged()) {
        return super.subReadyToSerialize();
      }

      if (this.segments != null) {
        return true;
      }
      return false;
    }

    @Override
    protected int subSerialize(WritableFontData newData) {
      if (!this.modelChanged()) {
        return super.subSerialize(newData);
      }

      int index = 0;
      index += newData.writeUShort(index, CMapFormat.Format4.value());
      index += FontData.DataSize.USHORT.size(); // length - write this at the
                                                // end
      index += newData.writeUShort(index, this.language());
      int segCount = this.segments.size();
      index += newData.writeUShort(index, segCount * 2);
      int log2SegCount = FontMath.log2(this.segments.size());
      int searchRange = 1 << (log2SegCount + 1);
      index += newData.writeUShort(index, searchRange);
      int entrySelector = log2SegCount;
      index += newData.writeUShort(index, entrySelector);
      int rangeShift = 2 * segCount - searchRange;
      index += newData.writeUShort(index, rangeShift);

      for (int i = 0; i < segCount; i++) {
        index += newData.writeUShort(index, this.segments.get(i).getEndCount());
      }
      index += FontData.DataSize.USHORT.size(); // reserved UShort
      for (int i = 0; i < segCount; i++) {
        index += newData.writeUShort(index, this.segments.get(i).getStartCount());
      }
      for (int i = 0; i < segCount; i++) {
        index += newData.writeShort(index, this.segments.get(i).getIdDelta());
      }
      for (int i = 0; i < segCount; i++) {
        index += newData.writeUShort(index, this.segments.get(i).getIdRangeOffset());
      }

      for (int i = 0; i < this.glyphIdArray.size(); i++) {
        index += newData.writeUShort(index, this.glyphIdArray.get(i));
      }

      newData.writeUShort(Offset.format4Length.offset, index);

      return index;
    }
  }
}
