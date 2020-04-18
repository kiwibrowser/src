package com.google.typography.font.sfntly.table.opentype.component;

public enum GsubLookupType implements LookupType {
  GSUB_SINGLE,
  GSUB_MULTIPLE,
  GSUB_ALTERNATE,
  GSUB_LIGATURE,
  GSUB_CONTEXTUAL,
  GSUB_CHAINING_CONTEXTUAL,
  GSUB_EXTENSION,
  GSUB_REVERSE_CHAINING_CONTEXTUAL_SINGLE;

  @Override
  public int typeNum() {
    return ordinal() + 1;
  }

  @Override
  public String toString() {
    return super.toString().toLowerCase();
  }

  public static GsubLookupType forTypeNum(int typeNum) {
    if (typeNum <= 0 || typeNum > values.length) {
      System.err.format("unknown gsub lookup typeNum: %d\n", typeNum);
      return null;
    }
    return values[typeNum - 1];
  }

  private static final GsubLookupType[] values = values();
}
