// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.table.opentype;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.SubTable;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
abstract class LayoutCommonTable<T extends LookupTable> extends SubTable {
  private static int VERSION_OFFSET = 0;
  private static int SCRIPT_LIST_OFFSET = 4;
  private static int FEATURE_LIST_OFFSET = 6;
  private static int LOOKUP_LIST_OFFSET = 8;
  private static int HEADER_SIZE = 10;

  private static int VERSION_ID = 0x00010000;

  private final boolean dataIsCanonical;

  /**
   * @param data
   *          the GSUB or GPOS data
   */
  protected LayoutCommonTable(ReadableFontData data, boolean dataIsCanonical) {
    super(data);
    this.dataIsCanonical = dataIsCanonical;
  }

  private static int readScriptListOffset(ReadableFontData data) {
    return data.readUShort(SCRIPT_LIST_OFFSET);
  }

  private static ReadableFontData scriptListData(ReadableFontData commonData,
      boolean dataIsCanonical) {
    int start = readScriptListOffset(commonData);
    if (dataIsCanonical) {
      int limit = readFeatureListOffset(commonData);
      return commonData.slice(start, limit - start);
    }
    return commonData.slice(start);
  }

  ScriptListTable createScriptList() {
    return new ScriptListTable(scriptListData(data, dataIsCanonical), dataIsCanonical);
  }

  private static int readFeatureListOffset(ReadableFontData data) {
    return data.readUShort(FEATURE_LIST_OFFSET);
  }

  private static ReadableFontData featureListData(ReadableFontData commonData,
      boolean dataIsCanonical) {
    int start = readFeatureListOffset(commonData);
    if (dataIsCanonical) {
      int limit = readLookupListOffset(commonData);
      return commonData.slice(start, limit - start);
    }
    return commonData.slice(start);
  }

  FeatureListTable createFeatureList() {
    return new FeatureListTable(featureListData(data, dataIsCanonical), dataIsCanonical);
  }

  private static int readLookupListOffset(ReadableFontData data) {
    return data.readUShort(LOOKUP_LIST_OFFSET);
  }

  private static ReadableFontData lookupListData(ReadableFontData commonData,
      boolean dataIsCanonical) {
    int start = readLookupListOffset(commonData);
    if (dataIsCanonical) {
      int limit = commonData.length();
      return commonData.slice(start, limit - start);
    }
    return commonData.slice(start);
  }

  protected LookupListTable createLookupList() {
    return handleCreateLookupList(lookupListData(data, dataIsCanonical), dataIsCanonical);
  }

  protected abstract LookupListTable handleCreateLookupList(
      ReadableFontData data, boolean dataIsCanonical);

  static abstract class Builder<T extends LookupTable>
  extends SubTable.Builder<LayoutCommonTable<T>> {
    private int serializedLength;
    private ScriptListTable.Builder serializedScriptListBuilder;
    private FeatureListTable.Builder serializedFeatureListBuilder;
    private LookupListTable.Builder serializedLookupListBuilder;

    /**
     * @param data
     *          the GSUB or GPOS data
     */
    protected Builder(ReadableFontData data, boolean dataIsCanonical) {
      super(data);
    }

    protected Builder() {
      super(null);
    }

    protected abstract LookupListTable handleCreateLookupList(
        ReadableFontData data, boolean dataIsCanonical);

    protected abstract LookupListTable.Builder createLookupListBuilder();

    @Override
    protected int subSerialize(WritableFontData newData) {
      if (serializedLength == 0) {
        return 0;
      }
      newData.writeULong(VERSION_OFFSET, VERSION_ID);
      int pos = HEADER_SIZE;
      newData.writeUShort(SCRIPT_LIST_OFFSET, pos);
      pos += serializedScriptListBuilder.subSerialize(newData.slice(pos));
      newData.writeUShort(FEATURE_LIST_OFFSET, pos);
      pos += serializedFeatureListBuilder.subSerialize(newData.slice(pos));
      newData.writeUShort(LOOKUP_LIST_OFFSET, pos);
      pos += serializedLookupListBuilder.subSerialize(newData.slice(pos));
      return serializedLength;
    }

    @Override
    protected boolean subReadyToSerialize() {
      return true;
    }

    @Override
    protected abstract LayoutCommonTable<T> subBuildTable(ReadableFontData data);
  }
}
